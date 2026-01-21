// container-data/main.c
//
// CBOR sample publisher for Edge Impulse motion classifier.
//
// Closed-loop version:
//  - Scans a directory for Edge Impulse *.cbor* files
//  - For each file, decodes the CBOR (see ei_cbor_decoder.c / ei_cbor_decoder.h)
//    into a structured sample.
//  - Takes the payload.values array (float samples) and slices each long
//    sample into classifier-sized windows using either random or deterministic
//    (evenly spaced) start positions.
//  - For each window:
//        * publish to the internal OCRE bus on EI_BUS_TOPIC
//        * wait for a result message on EI_RESULT_TOPIC
//        * compare predicted label with expected label for the sample
//  - Uses classifier results to trigger the next window (no fixed time delay
//    between windows, aside from a small polling delay while waiting).

#include <stdio.h>
#include <stdlib.h>
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

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

// Directory containing CBOR samples. You can override this at runtime via argv[1].
#define DEFAULT_SAMPLE_DIR          "testing"

// Number of classifier-sized windows to generate per CBOR file.
// If CHUNKS_PER_SAMPLE > number of available windows, it will be clamped.
#define CHUNKS_PER_SAMPLE           3

// Number of axes in the raw data (e.g. accelerometer X/Y/Z = 3).
// This should match the impulse's sensor configuration.
#define N_AXES                      3

// Topic + content-type used on the internal bus (must match classifier container).
#define EI_BUS_TOPIC                "ei/sample/raw"
#define EI_BUS_CONTENT_TYPE         "application/ei-bus-f32"

// Classifier result topic / content-type (must match classifier container).
#define EI_RESULT_TOPIC             "ei/result"
#define EI_RESULT_CONTENT_TYPE      "text/plain"

// Window selection mode:
//   1 = Randomized start positions (non-deterministic, seeded with time())
//   2 = Deterministic, evenly spaced start positions over all valid windows
#define WINDOW_MODE_RANDOM          1
#define WINDOW_MODE_DETERMINISTIC   2
#define WINDOW_SELECTION_MODE       WINDOW_MODE_RANDOM

// Timeout waiting for a classifier result (ms)
#define RESULT_TIMEOUT_MS           5000
#define RESULT_POLL_INTERVAL_MS     10

// -----------------------------------------------------------------------------
// Global state for closed-loop result handling
// -----------------------------------------------------------------------------

static volatile bool g_waiting_for_result = false;
static volatile bool g_result_received    = false;

static char  g_last_result_label[64];
static float g_last_result_score = 0.0f;

// Simple stats
static size_t g_total_windows   = 0;
static size_t g_correct_windows = 0;

// -----------------------------------------------------------------------------
// Result parsing and callback
// -----------------------------------------------------------------------------

/**
 * Extract the expected label from the CBOR filename.
 *
 * Assumes filenames like:
 *   testing/idle.1.cbor.XXXX.cbor
 *   testing/snake.1.cbor.XXXX.cbor
 *
 * We take the basename (after last '/') and then everything up to the first
 * '.' as the label ("idle", "snake", etc.).
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
 * Callback for classifier results published on EI_RESULT_TOPIC.
 *
 * Expects payload of the form:
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

// -----------------------------------------------------------------------------
// OCRE messaging bus publishing helpers
// -----------------------------------------------------------------------------

static void publish_window(const char *sample_name,
                           size_t window_index,
                           const float *window_data,
                           size_t window_len)
{
    // Debug logging
    // printf(LOG_PREFIX "Publishing sample '%s' window %u (len=%u floats) to %s\n",
    //        sample_name,
    //        (unsigned)window_index,
    //        (unsigned)window_len,
    //        EI_BUS_TOPIC);

    // Print sample name prior to publishing (per earlier requirement)
    printf(LOG_PREFIX "Publish window %u of sample \"%s\"\n",
           (unsigned)window_index, sample_name);

    // Debug logging
    // printf(LOG_PREFIX "Data: [");
    // for (int i = 0; i < (int)window_len; i++) {
    //     printf("%.5f", window_data[i]);
    //     if (i != (int)(window_len - 1)) {
    //         printf(", ");
    //     }
    // }
    // printf("]\n");

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
 * Returns true if we got a result and the predicted label matches expected_label.
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

// -----------------------------------------------------------------------------
// Window / chunk generation
// -----------------------------------------------------------------------------

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

    // Debug logging
    // printf(LOG_PREFIX
    //        "Sample %s: %u frames, window_frames=%u -> %u possible windows, emitting %u\n",
    //        sample_name,
    //        (unsigned)n_frames,
    //        (unsigned)window_frames,
    //        (unsigned)available_windows,
    //        (unsigned)chunks_to_emit);
    // printf(LOG_PREFIX "  expected label: '%s'\n", expected_label);

    if (chunks_to_emit == 0) {
        return;
    }

    size_t *start_frames = (size_t *)malloc(chunks_to_emit * sizeof(size_t));
    if (!start_frames) {
        printf(LOG_PREFIX "Failed to allocate start_frames array\n");
        return;
    }

#if WINDOW_SELECTION_MODE == WINDOW_MODE_RANDOM
    // ------------------------ Random selection -------------------------------
    //
    // Pick CHUNKS_PER_SAMPLE distinct start positions uniformly over the
    // available windows via a partial Fisher–Yates shuffle.

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
    // ------------------------ Deterministic selection -----------------------
    //
    // Even spacing over [0..max_start]. This is reproducible and provides
    // good coverage regardless of sample length.
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

        // Debug logging
        // printf(LOG_PREFIX "  Window %u: start_frame=%u\n",
        //        (unsigned)w, (unsigned)start_frame);

        // Copy one full window: window_frames * n_axes floats.
        memcpy(window_buf,
               raw + start_index,
               window_floats * sizeof(float));

        // Closed-loop: send window and wait for classifier result
        (void)send_window_and_wait_for_result(sample_name,
                                              expected_label,
                                              w,
                                              window_buf,
                                              window_floats);
    }

    free(start_frames);
}

// -----------------------------------------------------------------------------
// Directory scanning
// -----------------------------------------------------------------------------

static bool has_cbor_substring(const char *name)
{
    return strstr(name, ".cbor") != NULL;
}

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
 * Scan a directory for files whose names contain ".cbor".
 *
 * On success:
 *   - *out_files receives a heap-allocated array of char* paths (relative to dir)
 *   - *out_count is the number of entries
 *   - Caller must free each element of out_files[i] and then free(out_files).
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

    // Iterate over all discovered files and publish windows in closed-loop.
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
