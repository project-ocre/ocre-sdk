// container-data/main.c
//
// CBOR sample publisher for Edge Impulse motion classifier.
//
// - Scans a directory for *.cbor* files (ingestion-format CBOR).
// - For each file, decodes the CBOR using a reusable QCBOR-based decoder
//   (see cbor_decoder.c / cbor_decoder.h) into a structured sample.
// - Takes the payload.values array (float samples) and slices each long
//   sample into classifier-sized windows using either random or deterministic
//   (evenly spaced) start positions.
// - Publishes each window to the internal OCRE bus with a configurable
//   delay between windows.

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

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

// Directory containing CBOR samples. You can override this at runtime via argv[1].
#define DEFAULT_SAMPLE_DIR          "testing"

// Number of classifier-sized windows to generate per CBOR file.
// If CHUNKS_PER_SAMPLE > number of available windows, it will be clamped.
#define CHUNKS_PER_SAMPLE           3

// Delay between sending each window over the bus (in milliseconds).
#define SEND_DELAY_MS               50

// Number of axes in the raw data (e.g. accelerometer X/Y/Z = 3).
// This should match the impulse's sensor configuration.
#define N_AXES                      3

// Topic + content-type used on the internal bus (must match classifier container).
#define EI_BUS_TOPIC                "ei/sample/raw"
#define EI_BUS_CONTENT_TYPE         "application/ei-bus-f32"

// Window selection mode:
//   1 = Randomized start positions (non-deterministic, seeded with time())
//   2 = Deterministic, evenly spaced start positions over all valid windows
#define WINDOW_MODE_RANDOM          1
#define WINDOW_MODE_DETERMINISTIC   2
#define WINDOW_SELECTION_MODE       WINDOW_MODE_RANDOM

// -----------------------------------------------------------------------------
// OCRE bus publishing helpers
// -----------------------------------------------------------------------------

static void publish_window(const char *sample_name,
                           size_t window_index,
                           const float *window_data,
                           size_t window_len)
{
    // Log for debug / traceability
    printf("Publishing sample '%s' window %u (len=%u floats) to %s\n",
           sample_name,
           (unsigned)window_index,
           (unsigned)window_len,
           EI_BUS_TOPIC);

    // Print sample name prior to publishing (per earlier requirement)
    printf("Sample: %s (window %u)\n", sample_name, (unsigned)window_index);

    uint32_t payload_bytes = (uint32_t)(window_len * sizeof(float));

    int rc = ocre_publish_message(EI_BUS_TOPIC,
                                  EI_BUS_CONTENT_TYPE,
                                  (const void *)window_data,
                                  payload_bytes);
    if (rc != 0) {
        fprintf(stderr,
                "Failed to publish window %u for sample '%s' (rc=%d)\n",
                (unsigned)window_index, sample_name, rc);
    }
}

// -----------------------------------------------------------------------------
// Window / chunk generation
// -----------------------------------------------------------------------------

static void publish_windows_for_sample(const char *sample_name,
                                       const float *raw,
                                       size_t total_floats)
{
    const size_t window_frames = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    const size_t window_floats = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    const size_t n_axes        = N_AXES;

    if (total_floats % n_axes != 0) {
        fprintf(stderr,
                "Sample %s: total_floats=%u not divisible by %u axes, skipping\n",
                sample_name, (unsigned)total_floats, (unsigned)n_axes);
        return;
    }

    const size_t n_frames = total_floats / n_axes;

    if (n_frames < window_frames) {
        fprintf(stderr,
                "Sample %s: only %u frames (< %u), skipping\n",
                sample_name, (unsigned)n_frames, (unsigned)window_frames);
        return;
    }

    // Sanity check: the impulse's DSP input size should match frames * axes.
    if (window_floats != window_frames * n_axes) {
        fprintf(stderr,
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

    printf("Sample %s: %u frames, window_frames=%u -> %u possible windows, emitting %u\n",
           sample_name,
           (unsigned)n_frames,
           (unsigned)window_frames,
           (unsigned)available_windows,
           (unsigned)chunks_to_emit);

    if (chunks_to_emit == 0) {
        return;
    }

    size_t *start_frames = (size_t *)malloc(chunks_to_emit * sizeof(size_t));
    if (!start_frames) {
        fprintf(stderr, "Failed to allocate start_frames array\n");
        return;
    }

#if WINDOW_SELECTION_MODE == WINDOW_MODE_RANDOM
    // ------------------------ Random selection -------------------------------
    //
    // Pick CHUNKS_PER_SAMPLE distinct start positions uniformly over the
    // available windows via a partial Fisher–Yates shuffle.

    size_t *all_starts = (size_t *)malloc(available_windows * sizeof(size_t));
    if (!all_starts) {
        fprintf(stderr, "Failed to allocate all_starts array\n");
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
            fprintf(stderr,
                    "Sample %s: computed out-of-range window (%u), skipping\n",
                    sample_name, (unsigned)w);
            continue;
        }

        printf("  Window %u: start_frame=%u\n",
               (unsigned)w, (unsigned)start_frame);

        // Copy one full window: window_frames * n_axes floats.
        memcpy(window_buf,
               raw + start_index,
               window_floats * sizeof(float));

        publish_window(sample_name, w, window_buf, window_floats);

        // Configurable delay between chunks
        ocre_sleep(SEND_DELAY_MS);
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
        fprintf(stderr, "Failed to open directory '%s': %s\n", dir, strerror(errno));
        return false;
    }

    size_t capacity = 16;
    char **files = (char **)malloc(capacity * sizeof(char *));
    if (!files) {
        fprintf(stderr, "Failed to allocate file list\n");
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
            fprintf(stderr, "Path too long, skipping: %s/%s\n", dir, name);
            continue;
        }

        if (num_files == capacity) {
            capacity *= 2;
            char **tmp = (char **)realloc(files, capacity * sizeof(char *));
            if (!tmp) {
                fprintf(stderr, "Failed to grow file list\n");
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
            fprintf(stderr, "Failed to duplicate path string\n");
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
        fprintf(stderr, "No .cbor files found in '%s'\n", dir);
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

    printf("Processing file: %s\n", path);

    if (!ei_cbor_decode_file(path, &sample)) {
        fprintf(stderr, "Failed to decode CBOR file %s\n", path);
        return;
    }

    // Optional: log some metadata for debugging / reuse
    if (sample.device_type[0] != '\0') {
        printf("  device_type : %s\n", sample.device_type);
    }
    if (sample.device_name[0] != '\0') {
        printf("  device_name : %s\n", sample.device_name);
    }
    if (sample.has_interval_ms) {
        printf("  interval_ms : %.3f\n", sample.interval_ms);
    }
    if (sample.n_sensors > 0) {
        printf("  sensors (%u):\n", sample.n_sensors);
        for (uint32_t i = 0; i < sample.n_sensors; i++) {
            printf("    [%u] %s (%s)\n",
                   i,
                   sample.sensors[i].name,
                   sample.sensors[i].units);
        }
    }
    printf("  frames: %zu, axes: %zu, total_floats: %zu\n",
           sample.n_frames, sample.n_axes, sample.n_values);

    if (sample.n_axes != N_AXES) {
        fprintf(stderr,
                "Sample %s: decoder reported %zu axes, expected %u, skipping\n",
                path, sample.n_axes, (unsigned)N_AXES);
        ei_cbor_free_sample(&sample);
        return;
    }

    // From the classifier’s point of view we just need the raw feature vector.
    publish_windows_for_sample(path, sample.values, sample.n_values);

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

    printf("Using sample directory: %s\n", sample_dir);

    char **files = NULL;
    size_t num_files = 0;

    if (!scan_cbor_files(sample_dir, &files, &num_files)) {
        fprintf(stderr, "No CBOR files to process. Exiting.\n");
        return 1;
    }

    printf("Found %u CBOR files\n", (unsigned)num_files);

    // Iterate over all discovered files and publish windows.
    for (size_t i = 0; i < num_files; i++) {
        process_file(files[i]);
        free(files[i]);
    }
    free(files);

    // If you want continuous streaming, you could wrap the above in a `while (1)`
    // loop, but for now we run through the dataset once and exit.
    return 0;
}
