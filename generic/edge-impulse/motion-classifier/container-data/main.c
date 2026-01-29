/*
 * CBOR Sample Publisher for Edge Impulse Motion Classifier
 *
 * This module implements a closed-loop test data publisher that:
 *  1. Scans a directory for Edge Impulse CBOR-encoded samples
 *  2. Decodes each CBOR file into a structured sample
 *  3. Slices samples into classifier-sized windows using configurable selection
 *     modes (random or evenly-spaced deterministic)
 *  4. Publishes each window to the OCRE messaging bus
 *  5. Waits for and validates classifier results against expected labels
 *  6. Reports overall accuracy statistics
 *
 * See ei_cbor_decoder.h for CBOR decoding details and model_metadata.h for
 * classifier configuration parameters.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <inttypes.h>

#include "ocre_api.h"
#include "../model-parameters/model_metadata.h"
#include "ei_cbor_decoder.h"

#ifndef LOG_PREFIX
#define LOG_PREFIX "[DATA] "
#endif

/* ============================================================================
 * Configuration Constants
 * ============================================================================ */

/**
 * Directory containing CBOR sample files.
 */
#define DEFAULT_SAMPLE_DIR          "testing"

/**
 * Number of classifier-sized windows to generate per CBOR file.
 * If CHUNKS_PER_SAMPLE exceeds the number of available windows, it is clamped
 * to the maximum available.
 */
#define CHUNKS_PER_SAMPLE           3

/**
 * Number of axes in raw sensor data (e.g., accelerometer: X/Y/Z = 3).
 * Must match the impulse's sensor configuration.
 */
#define N_AXES                      3

/**
 * OCRE messaging bus topic and content type for raw samples.
 * Must match the classifier container.
 */
#define EI_BUS_TOPIC                "ei/sample/raw"
#define EI_BUS_CONTENT_TYPE         "application/ei-bus-f32"

/**
 * OCRE messaging bus topic and content type for inference results.
 * Must match the classifier container.
 */
#define EI_RESULT_TOPIC             "ei/result"
#define EI_RESULT_CONTENT_TYPE      "text/plain"

/**
 * Window selection mode options:
 *   WINDOW_MODE_RANDOM        - Randomized start positions (non-deterministic)
 *   WINDOW_MODE_DETERMINISTIC - Evenly-spaced positions (reproducible)
 *
 * Set WINDOW_SELECTION_MODE to one of the above values.
 */
#define WINDOW_MODE_RANDOM          1
#define WINDOW_MODE_DETERMINISTIC   2
#define WINDOW_SELECTION_MODE       WINDOW_MODE_RANDOM

/**
 * Result message polling configuration.
 * RESULT_TIMEOUT_MS: Maximum time to wait for a classifier result (milliseconds)
 * RESULT_POLL_INTERVAL_MS: Interval between poll attempts (milliseconds)
 */
#define RESULT_TIMEOUT_MS           5000
#define RESULT_POLL_INTERVAL_MS     10

/* ============================================================================
 * Global State for Closed-Loop Result Handling
 * ============================================================================ */

/** Flag indicating that we are waiting for a classifier result. */
static volatile bool g_waiting_for_result = false;

/** Flag indicating that a classifier result has been received. */
static volatile bool g_result_received    = false;

/** Last received classifier result label. */
static char  g_last_result_label[64];

/** Last received classifier result confidence score. */
static float g_last_result_score = 0.0f;

/** Total number of windows processed. */
static size_t g_total_windows   = 0;

/** Number of windows with correct predictions. */
static size_t g_correct_windows = 0;

/* ============================================================================
 * Result Parsing and Callback Handling
 * ============================================================================ */

/**
 * Extract the expected label from the CBOR filename.
 *
 * Filename format expected:
 *   <directory>/<label>.<index>.cbor[.<id>].cbor
 *
 * The label is extracted as everything from the start of the filename up to
 * the first '.' character.
 *
 * @param path        Full or relative path to the CBOR file
 * @param label_out   Output buffer for the extracted label
 * @param max_len     Maximum length of label_out (including null terminator)
 */
static void extract_expected_label_from_path(const char *path,
                                             char *label_out,
                                             size_t max_len)
{
    if (!path || !label_out || max_len == 0) {
        return;
    }

    const char *base = strrchr(path, '/');
    if (base) {
        base++; // skip '/'
    }
    else {
        base = path;
    }

    const char *dot = strchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);

    if (len >= max_len) {
        len = max_len - 1;
    }

    memcpy(label_out, base, len);
    label_out[len] = '\0';
}

/**
 * Callback for classifier result messages.
 *
 * Processes result messages published on EI_RESULT_TOPIC and extracts the
 * predicted label and confidence score. Updates global state and signals
 * completion of the result wait.
 *
 * Expected payload format:
 *   "label=<name> score=<float>"
 */
static void result_message_handler(const char *topic,
                                   const char *content_type,
                                   const void *payload,
                                   uint32_t payload_len)
{
    if (!topic || !content_type || !payload) {
        return;
    }

    if (strcmp(topic, EI_RESULT_TOPIC) != 0) {
        // Not for us
        return;
    }

    if (strcmp(content_type, EI_RESULT_CONTENT_TYPE) != 0) {
        printf(LOG_PREFIX
               "Data app: ignoring result with unexpected content_type '%s'\n",
               content_type);
        return;
    }

    // Copy into a local buffer and NUL-terminate for parsing
    char buf[128];
    size_t copy_len = (payload_len < sizeof(buf) - 1) ? payload_len : (sizeof(buf) - 1);
    memcpy(buf, payload, copy_len);
    buf[copy_len] = '\0';

    char label[64] = { 0 };
    float score = 0.0f;

    if (sscanf(buf, "label=%63s score=%f", label, &score) != 2) {
        printf(LOG_PREFIX "Data app: failed to parse result payload: '%s'\n", buf);
        return;
    }

    strncpy(g_last_result_label, label, sizeof(g_last_result_label));
    g_last_result_label[sizeof(g_last_result_label) - 1] = '\0';
    g_last_result_score = score;

    g_result_received    = true;
    g_waiting_for_result = false;

    printf(LOG_PREFIX "Data app: received result: label='%s' score=%.5f\n",
           g_last_result_label, g_last_result_score);
}

/* ============================================================================
 * OCRE Messaging Bus Publishing Helpers
 * ============================================================================ */

/**
 * Publish a single classifier-sized window to the OCRE messaging bus.
 *
 * @param sample_name   Name/path of the source sample file
 * @param window_index  Index of the window within the sample
 * @param window_data   Pointer to raw float data (window_len floats)
 * @param window_len    Number of floats in window_data
 */
static void publish_window(const char *sample_name,
                           size_t window_index,
                           const float *window_data,
                           size_t window_len)
{
    printf(LOG_PREFIX "Publish window %u of sample \"%s\"\n",
           (unsigned)window_index, sample_name);

    uint32_t payload_bytes = (uint32_t)(window_len * sizeof(float));

    int rc = ocre_publish_message(EI_BUS_TOPIC,
                                  EI_BUS_CONTENT_TYPE,
                                  (const void *)window_data,
                                  payload_bytes);
    if (rc != 0) {
        printf(LOG_PREFIX
               "Failed to publish window %u for sample '%s' (rc=%d)\n",
               (unsigned)window_index, sample_name, rc);
    }
}

/**
 * Publish one window and wait for the classifier to respond.
 *
 * This function implements the closed-loop testing: it sends a sample window
 * to the classifier, waits for a result, and validates the predicted label
 * against the expected label. Statistics are updated for tracking overall
 * accuracy.
 *
 * @param sample_name    Name/path of the source sample
 * @param expected_label Expected class label from the sample filename
 * @param window_index   Index of this window within the sample
 * @param window_data    Pointer to raw float data
 * @param window_len     Number of floats in window_data
 *
 * @return true if the prediction matched the expected label, false otherwise
 */
static bool send_window_and_wait_for_result(const char *sample_name,
                                            const char *expected_label,
                                            size_t window_index,
                                            const float *window_data,
                                            size_t window_len)
{
    g_waiting_for_result = true;
    g_result_received    = false;
    g_last_result_label[0] = '\0';
    g_last_result_score = 0.0f;

    publish_window(sample_name, window_index, window_data, window_len);

    uint32_t waited_ms = 0;
    while (g_waiting_for_result && waited_ms < RESULT_TIMEOUT_MS) {
        ocre_process_events();
        ocre_sleep(RESULT_POLL_INTERVAL_MS);
        waited_ms += RESULT_POLL_INTERVAL_MS;
    }

    if (!g_result_received) {
        printf(LOG_PREFIX
               "Timed out waiting for result for sample '%s' window %u\n",
               sample_name, (unsigned)window_index);
        return false;
    }

    bool match = (strcmp(expected_label, g_last_result_label) == 0);

    printf(LOG_PREFIX "Comparison for sample '%s' window %u:\n",
           sample_name, (unsigned)window_index);
    printf(LOG_PREFIX "  expected='%s' predicted='%s' score=%.5f -> %s\n",
           expected_label,
           g_last_result_label,
           g_last_result_score,
           match ? "MATCH" : "MISMATCH");

    g_total_windows++;
    if (match) {
        g_correct_windows++;
    }

    return match;
}

/* ============================================================================
 * Window/Chunk Generation
 * ============================================================================ */

/**
 * Generate and publish classifier-sized windows from a single CBOR sample.
 *
 * This function:
 *  1. Validates the sample has the expected number of axes
 *  2. Determines available windows based on sample length and classifier window size
 *  3. Selects windows using the configured mode (random or deterministic)
 *  4. Publishes each window and waits for classifier result
 *
 * @param sample_name     Name/path of the source sample file
 * @param expected_label  Expected class label from the sample filename
 * @param raw             Pointer to raw float sample data
 * @param total_floats    Total number of floats in the raw data
 */
static void publish_windows_for_sample(const char *sample_name,
                                       const char *expected_label,
                                       const float *raw,
                                       size_t total_floats)
{
    const size_t window_frames = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    const size_t window_floats = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    const size_t n_axes        = N_AXES;

    if (total_floats % n_axes != 0) {
        printf(LOG_PREFIX
               "Sample %s: total_floats=%u not divisible by %u axes, skipping\n",
               sample_name, (unsigned)total_floats, (unsigned)n_axes);
        return;
    }

    const size_t n_frames = total_floats / n_axes;

    if (n_frames < window_frames) {
        printf(LOG_PREFIX
               "Sample %s: only %u frames (< %u), skipping\n",
               sample_name, (unsigned)n_frames, (unsigned)window_frames);
        return;
    }

    // Sanity check: the impulse's DSP input size should match frames * axes.
    if (window_floats != window_frames * n_axes) {
        printf(LOG_PREFIX
               "Configuration error: window_floats=%u, expected %u (frames=%u * axes=%u)\n",
               (unsigned)window_floats,
               (unsigned)(window_frames * n_axes),
               (unsigned)window_frames,
               (unsigned)n_axes);
        return;
    }

    const size_t max_start         = n_frames - window_frames;
    const size_t available_windows = max_start + 1;
    const size_t chunks_to_emit    =
        (CHUNKS_PER_SAMPLE < available_windows) ? CHUNKS_PER_SAMPLE : available_windows;

    if (chunks_to_emit == 0) {
        return;
    }

    size_t *start_frames = (size_t *)malloc(chunks_to_emit * sizeof(size_t));
    if (!start_frames) {
        printf(LOG_PREFIX "Failed to allocate start_frames array\n");
        return;
    }

#if WINDOW_SELECTION_MODE == WINDOW_MODE_RANDOM
    /*
     * Random Selection: Pick CHUNKS_PER_SAMPLE distinct start positions
     * uniformly over available windows via a partial Fisher–Yates shuffle.
     */

    size_t *all_starts = (size_t *)malloc(available_windows * sizeof(size_t));
    if (!all_starts) {
        printf(LOG_PREFIX "Failed to allocate all_starts array\n");
        free(start_frames);
        return;
    }

    for (size_t i = 0; i < available_windows; i++) {
        all_starts[i] = i;
    }

    // Partial Fisher–Yates shuffle for first chunks_to_emit elements
    for (size_t i = 0; i < chunks_to_emit; i++) {
        size_t j = i + (size_t)(rand() % (available_windows - i));
        size_t tmp    = all_starts[i];
        all_starts[i] = all_starts[j];
        all_starts[j] = tmp;
    }

    for (size_t i = 0; i < chunks_to_emit; i++) {
        start_frames[i] = all_starts[i];
    }

    free(all_starts);
#elif WINDOW_SELECTION_MODE == WINDOW_MODE_DETERMINISTIC
    /*
     * Deterministic Selection: Even spacing over [0..max_start].
     * This is reproducible and provides good coverage regardless of sample length.
     */
    if (chunks_to_emit == 1) {
        start_frames[0] = max_start / 2;
    } else {
        size_t step = max_start / (chunks_to_emit - 1);
        if (step == 0) {
            step = 1;
        }
        for (size_t i = 0; i < chunks_to_emit; i++) {
            size_t start = i * step;
            if (start > max_start) {
                start = max_start;
            }
            start_frames[i] = start;
        }
    }
#else
#error "Invalid WINDOW_SELECTION_MODE"
#endif

    float window_buf[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

    for (size_t w = 0; w < chunks_to_emit; w++) {
        size_t start_frame = start_frames[w];
        size_t start_index = start_frame * n_axes;

        if (start_index + window_floats > total_floats) {
            printf(LOG_PREFIX
                   "Sample %s: computed out-of-range window (%u), skipping\n",
                   sample_name, (unsigned)w);
            continue;
        }

        /* Copy one full window: window_frames * n_axes floats. */
        memcpy(window_buf,
               raw + start_index,
               window_floats * sizeof(float));

        /* Send window to classifier and wait for result. */
        (void)send_window_and_wait_for_result(sample_name,
                                              expected_label,
                                              w,
                                              window_buf,
                                              window_floats);
    }

    free(start_frames);
}

/* ============================================================================
 * Directory Scanning
 * ============================================================================ */

/**
 * Check if filename contains the ".cbor" substring.
 *
 * @param name  Filename to check
 * @return true if ".cbor" is found in the filename, false otherwise
 */
static bool has_cbor_substring(const char *name)
{
    return strstr(name, ".cbor") != NULL;
}

/**
 * Check if a path refers to a regular file (not a directory or special file).
 *
 * @param dir   Directory path containing the file
 * @param name  Filename to check
 * @return true if path is a regular file, false otherwise
 */
static bool is_regular_file(const char *dir, const char *name)
{
    char path[512];
    int n = snprintf(path, sizeof(path), "%s/%s", dir, name);
    if (n <= 0 || (size_t)n >= sizeof(path)) {
        return false;
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }

    return S_ISREG(st.st_mode);
}

/**
 * Scan a directory for files matching the CBOR naming pattern.
 *
 * Discovers all regular files in the specified directory whose names contain
 * the ".cbor" substring. Results are allocated on the heap and must be freed
 * by the caller.
 *
 * @param dir        Directory path to scan
 * @param out_files  Output: pointer to heap-allocated array of file paths.
 *                   Each path is relative to dir. Caller must free each element
 *                   and then free the array itself.
 * @param out_count  Output: number of files discovered
 *
 * @return true on success with valid files found, false if directory cannot
 *         be opened or no CBOR files are found
 */
static bool scan_cbor_files(const char *dir,
                            char ***out_files,
                            size_t *out_count)
{
    *out_files = NULL;
    *out_count = 0;

    DIR *d = opendir(dir);
    if (!d) {
        printf(LOG_PREFIX "Failed to open directory '%s': %s\n", dir, strerror(errno));
        return false;
    }

    size_t capacity = 16;
    char **files = (char **)malloc(capacity * sizeof(char *));
    if (!files) {
        printf(LOG_PREFIX "Failed to allocate file list\n");
        closedir(d);
        return false;
    }

    size_t num_files = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        const char *name = ent->d_name;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        if (!has_cbor_substring(name)) {
            continue;
        }

        if (!is_regular_file(dir, name)) {
            continue;
        }

        char full_path[512];
        int n = snprintf(full_path, sizeof(full_path), "%s/%s", dir, name);
        if (n <= 0 || (size_t)n >= sizeof(full_path)) {
            printf(LOG_PREFIX "Path too long, skipping: %s/%s\n", dir, name);
            continue;
        }

        if (num_files == capacity) {
            capacity *= 2;
            char **tmp = (char **)realloc(files, capacity * sizeof(char *));
            if (!tmp) {
                printf(LOG_PREFIX "Failed to grow file list\n");
                // Clean up and return
                for (size_t i = 0; i < num_files; i++) {
                    free(files[i]);
                }
                free(files);
                closedir(d);
                return false;
            }
            files = tmp;
        }

        files[num_files] = strdup(full_path);
        if (!files[num_files]) {
            printf(LOG_PREFIX "Failed to duplicate path string\n");
            // Clean up and return
            for (size_t i = 0; i < num_files; i++) {
                free(files[i]);
            }
            free(files);
            closedir(d);
            return false;
        }

        num_files++;
    }

    closedir(d);

    if (num_files == 0) {
        printf(LOG_PREFIX "No .cbor files found in '%s'\n", dir);
        free(files);
        return false;
    }

    *out_files = files;
    *out_count = num_files;
    return true;
}

// -----------------------------------------------------------------------------
// Per-file processing (CBOR decoding + window publication)
// -----------------------------------------------------------------------------

static void process_file(const char *path)
{
    ei_cbor_sample_t sample;

    printf(LOG_PREFIX "Processing file: %s\n", path);

    if (!ei_cbor_decode_file(path, &sample)) {
        printf(LOG_PREFIX "Failed to decode CBOR file %s\n", path);
        return;
    }

    // Debug logging
    // if (sample.device_type[0] != '\0') {
    //     printf(LOG_PREFIX "  device_type : %s\n", sample.device_type);
    // }
    // if (sample.device_name[0] != '\0') {
    //     printf(LOG_PREFIX "  device_name : %s\n", sample.device_name);
    // }
    // if (sample.has_interval_ms) {
    //     printf(LOG_PREFIX "  interval_ms : %.3f\n", sample.interval_ms);
    // }
    // if (sample.n_sensors > 0) {
    //     printf(LOG_PREFIX "  sensors (%u):\n", sample.n_sensors);
    //     for (uint32_t i = 0; i < sample.n_sensors; i++) {
    //         printf(LOG_PREFIX "    [%u] %s (%s)\n",
    //                i,
    //                sample.sensors[i].name,
    //                sample.sensors[i].units);
    //     }
    // }
    // printf(LOG_PREFIX "  frames: %zu, axes: %zu, total_floats: %zu\n",
    //        sample.n_frames, sample.n_axes, sample.n_values);

    if (sample.n_axes != N_AXES) {
        printf(LOG_PREFIX
               "Sample %s: decoder reported %zu axes, expected %u, skipping\n",
               path, sample.n_axes, (unsigned)N_AXES);
        ei_cbor_free_sample(&sample);
        return;
    }

    // Derive expected label from filename (see extract_expected_label_from_path)
    char expected_label[64];
    extract_expected_label_from_path(path, expected_label, sizeof(expected_label));

    // From the classifier’s point of view we just need the raw feature vector.
    publish_windows_for_sample(path, expected_label, sample.values, sample.n_values);

    ei_cbor_free_sample(&sample);
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    const char *sample_dir = (argc > 1) ? argv[1] : DEFAULT_SAMPLE_DIR;

#if WINDOW_SELECTION_MODE == WINDOW_MODE_RANDOM
    // Seed the RNG for random window selection.
    srand((unsigned)time(NULL));
#endif

    setvbuf(stdout, NULL, _IONBF, 0);

    printf(LOG_PREFIX "Data publisher start\n");
    printf(LOG_PREFIX "Using sample directory: %s\n", sample_dir);

    // Register and subscribe for classifier results
    int rc = ocre_register_message_callback(EI_RESULT_TOPIC, result_message_handler);
    if (rc != OCRE_SUCCESS) {
        printf(LOG_PREFIX "Error: Failed to register result callback for %s (ret=%d)\n",
               EI_RESULT_TOPIC, rc);
    }

    rc = ocre_subscribe_message(EI_RESULT_TOPIC);
    if (rc != OCRE_SUCCESS) {
        printf(LOG_PREFIX "Error: Failed to subscribe to result topic %s (ret=%d)\n",
               EI_RESULT_TOPIC, rc);
    }

    char **files = NULL;
    size_t num_files = 0;

    if (!scan_cbor_files(sample_dir, &files, &num_files)) {
        printf(LOG_PREFIX "No CBOR files to process. Exiting.\n");
        return 1;
    }

    printf(LOG_PREFIX "Found %u CBOR files\n", (unsigned)num_files);

    /* Process each file in closed-loop fashion. */
    for (size_t i = 0; i < num_files; i++) {
        process_file(files[i]);
        free(files[i]);
    }
    free(files);

    printf(LOG_PREFIX "Test results:\n");
    printf(LOG_PREFIX "  Total windows:   %zu\n", g_total_windows);
    printf(LOG_PREFIX "  Correct windows: %zu\n", g_correct_windows);
    if (g_total_windows > 0) {
        float acc = (float)g_correct_windows * 100.0f / (float)g_total_windows;
        printf(LOG_PREFIX "  Window accuracy: %.2f %%\n", acc);
    }

    return 0;
}
