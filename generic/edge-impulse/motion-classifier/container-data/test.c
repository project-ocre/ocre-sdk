#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>

#include "ei_cbor_decoder.h"

#ifndef TEST_LOG_PREFIX
#define TEST_LOG_PREFIX "[CBOR-TEST] "
#endif

/* Default directory if no arguments are provided */
#ifndef EI_CBOR_TEST_DIR
#define EI_CBOR_TEST_DIR "testing"
#endif

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

static bool is_directory(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

static bool ends_with(const char *s, const char *suffix)
{
    size_t slen = strlen(s);
    size_t tlen = strlen(suffix);
    if (slen < tlen) {
        return false;
    }
    return (strcmp(s + slen - tlen, suffix) == 0);
}

/* Small helper to build "dir/file" paths safely */
static void join_path(char *out, size_t out_size,
                      const char *dir, const char *name)
{
    size_t len_dir = strlen(dir);
    bool has_sep = (len_dir > 0) &&
                   (dir[len_dir - 1] == '/' || dir[len_dir - 1] == '\\');

    if (has_sep) {
        snprintf(out, out_size, "%s%s", dir, name);
    } else {
        snprintf(out, out_size, "%s/%s", dir, name);
    }
}

/* -------------------------------------------------------------------------
 * Sample analysis / validation
 * ------------------------------------------------------------------------- */

static void analyze_sample(const char *path, const ei_cbor_sample_t *s)
{
    printf(TEST_LOG_PREFIX "File: %s\n", path);

    /* Basic metadata */
    printf(TEST_LOG_PREFIX "  device_type : %s\n",
           (s->device_type[0] != '\0') ? s->device_type : "<none>");
    printf(TEST_LOG_PREFIX "  device_name : %s\n",
           (s->device_name[0] != '\0') ? s->device_name : "<none>");

    if (s->has_interval_ms) {
        printf(TEST_LOG_PREFIX "  interval_ms : %.3f\n", s->interval_ms);
    } else {
        printf(TEST_LOG_PREFIX "  interval_ms : <not present>\n");
    }

    printf(TEST_LOG_PREFIX "  sensors (%u):\n", s->n_sensors);
    for (uint32_t i = 0; i < s->n_sensors; i++) {
        const ei_cbor_sensor_t *sen = &s->sensors[i];
        printf(TEST_LOG_PREFIX "    [%u] name='%s' units='%s'\n",
               i,
               sen->name[0]  ? sen->name  : "<none>",
               sen->units[0] ? sen->units : "<none>");
    }

    printf(TEST_LOG_PREFIX "  frames: %zu, axes: %zu, total_floats: %zu\n",
           s->n_frames, s->n_axes, s->n_values);

    /* Dimensional consistency check */
    size_t expected_values = s->n_frames * s->n_axes;
    if (expected_values != s->n_values) {
        printf(TEST_LOG_PREFIX "  ERROR: n_values (%zu) != n_frames (%zu) * n_axes (%zu) = %zu\n",
               s->n_values, s->n_frames, s->n_axes, expected_values);
    } else {
        printf(TEST_LOG_PREFIX "  OK: n_values matches n_frames * n_axes\n");
    }

    /* Values sanity check */
    if (s->values == NULL || s->n_values == 0) {
        printf(TEST_LOG_PREFIX "  ERROR: values[] is NULL or n_values == 0\n");
        return;
    }

    double min_v = s->values[0];
    double max_v = s->values[0];
    double sum_v = 0.0;
    size_t zero_count = 0;

    for (size_t i = 0; i < s->n_values; i++) {
        float v = s->values[i];
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
        sum_v += (double)v;
        if (v == 0.0f) {
            zero_count++;
        }
    }

    double mean_v = sum_v / (double)s->n_values;
    double zero_pct = (s->n_values > 0)
                        ? (100.0 * (double)zero_count / (double)s->n_values)
                        : 0.0;

    printf(TEST_LOG_PREFIX "  value stats: min=%f max=%f mean=%f zeros=%zu (%.2f %%)\n",
           min_v, max_v, mean_v, zero_count, zero_pct);

    if (zero_count == s->n_values) {
        printf(TEST_LOG_PREFIX "  WARNING: all decoded values are zero\n");
    }

    /* Preview a few frames/axes so we can eyeball correctness */
    size_t preview_frames = (s->n_frames < 3) ? s->n_frames : 3;
    size_t preview_axes   = (s->n_axes   < 3) ? s->n_axes   : 3;

    printf(TEST_LOG_PREFIX "  preview of first %zu frame(s), %zu axe(s):\n",
           preview_frames, preview_axes);

    for (size_t f = 0; f < preview_frames; f++) {
        printf(TEST_LOG_PREFIX "    frame %zu: [", f);
        for (size_t a = 0; a < preview_axes; a++) {
            size_t idx = f * s->n_axes + a;
            printf("%f", s->values[idx]);
            if (a + 1 < preview_axes) {
                printf(", ");
            }
        }
        if (preview_axes < s->n_axes) {
            printf(", ...");
        }
        printf("]\n");
    }

    if (preview_frames < s->n_frames) {
        printf(TEST_LOG_PREFIX "    ... (%zu more frame(s) not shown)\n",
               s->n_frames - preview_frames);
    }

    printf(TEST_LOG_PREFIX "  decode/validation complete for %s\n\n", path);
}

/* -------------------------------------------------------------------------
 * Per-file & directory traversal
 * ------------------------------------------------------------------------- */

static void test_single_file(const char *path)
{
    ei_cbor_sample_t sample;
    memset(&sample, 0, sizeof(sample));

    printf(TEST_LOG_PREFIX "Decoding file: %s\n", path);

    if (!ei_cbor_decode_file(path, &sample)) {
        printf(TEST_LOG_PREFIX "  ERROR: ei_cbor_decode_file() failed for %s\n\n", path);
        return;
    }

    analyze_sample(path, &sample);

    /* Release heap allocations from the decoder */
    ei_cbor_free_sample(&sample);
}

/* Walk a directory and test every *.cbor* file inside */
static void test_directory(const char *dir_path)
{
    DIR *dir = opendir(dir_path);
    if (!dir) {
        printf(TEST_LOG_PREFIX "ERROR: Failed to open directory '%s'\n", dir_path);
        return;
    }

    printf(TEST_LOG_PREFIX "Scanning directory: %s\n", dir_path);

    struct dirent *ent;
    char full_path[1024];

    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;

        /* Skip . and .. */
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        /* Only consider files that look like EI CBOR ingestion files */
        if (!ends_with(name, ".cbor") &&
            !strstr(name, ".cbor.")) {
            continue;
        }

        join_path(full_path, sizeof(full_path), dir_path, name);
        test_single_file(full_path);
    }

    closedir(dir);
}

/* -------------------------------------------------------------------------
 * main()
 * ------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    if (argc <= 1) {
        /* Default behavior: look at the "testing" directory */
        printf(TEST_LOG_PREFIX "No paths specified, defaulting to '%s'\n",
               EI_CBOR_TEST_DIR);

        if (is_directory(EI_CBOR_TEST_DIR)) {
            test_directory(EI_CBOR_TEST_DIR);
        } else {
            printf(TEST_LOG_PREFIX "ERROR: '%s' is not a directory\n",
                   EI_CBOR_TEST_DIR);
            return 1;
        }

        return 0;
    }

    /* If paths are provided, treat each as either a file or directory */
    for (int i = 1; i < argc; i++) {
        const char *path = argv[i];

        if (is_directory(path)) {
            test_directory(path);
        } else {
            test_single_file(path);
        }
    }

    return 0;
}
