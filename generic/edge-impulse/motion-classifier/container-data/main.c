// container-data/main.c
//
// CBOR sample publisher for Edge Impulse motion classifier.
//
// - Scans a directory for *.cbor* files (ingestion-format CBOR).
// - For each file, decodes only the payload.values array into a flat
//   float buffer laid out as [ax0, ay0, az0,  ax1, ay1, az1, ...].
// - Slices each long sample into classifier-sized windows using either
//   random or deterministic (evenly spaced) start positions.
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

// Maximum length of CBOR text strings used for keys ("payload", "values", etc.).
#define CBOR_MAX_TSTR_LEN           64

// -----------------------------------------------------------------------------
// Small CBOR decoder for Edge Impulse ingestion files
// -----------------------------------------------------------------------------

typedef struct {
    const uint8_t *buf;
    size_t len;
    size_t pos;
} cbor_buf_t;

static bool cbor_read_u8(cbor_buf_t *c, uint8_t *out)
{
    if (c->pos >= c->len) {
        return false;
    }
    *out = c->buf[c->pos++];
    return true;
}

static bool cbor_read_bytes(cbor_buf_t *c, void *dst, size_t n)
{
    if (c->pos + n > c->len) {
        return false;
    }
    memcpy(dst, c->buf + c->pos, n);
    c->pos += n;
    return true;
}

static bool cbor_read_uint_n(cbor_buf_t *c, uint8_t addl, uint64_t *value)
{
    if (addl < 24) {
        *value = addl;
        return true;
    }
    uint8_t buf[8];
    switch (addl) {
        case 24:
            if (!cbor_read_u8(c, buf)) return false;
            *value = buf[0];
            return true;
        case 25:
            if (!cbor_read_bytes(c, buf, 2)) return false;
            *value = ((uint64_t)buf[0] << 8) | buf[1];
            return true;
        case 26:
            if (!cbor_read_bytes(c, buf, 4)) return false;
            *value = ((uint64_t)buf[0] << 24) |
                     ((uint64_t)buf[1] << 16) |
                     ((uint64_t)buf[2] << 8)  |
                     ((uint64_t)buf[3]);
            return true;
        case 27:
            if (!cbor_read_bytes(c, buf, 8)) return false;
            *value = ((uint64_t)buf[0] << 56) |
                     ((uint64_t)buf[1] << 48) |
                     ((uint64_t)buf[2] << 40) |
                     ((uint64_t)buf[3] << 32) |
                     ((uint64_t)buf[4] << 24) |
                     ((uint64_t)buf[5] << 16) |
                     ((uint64_t)buf[6] << 8)  |
                     ((uint64_t)buf[7]);
            return true;
        default:
            // 28, 29, 30 are reserved; 31 is "indefinite" and handled elsewhere.
            return false;
    }
}

// Parse CBOR initial byte and retrieve major type and length / value.
// For arrays & maps, `len` is the number of elements / pairs.
// For byte/text strings, `len` is the number of bytes.
// For integers, `len` is the numeric value.
// For simple/float, `len` is not meaningful (we only use major type and addl).
static bool cbor_read_type_len(cbor_buf_t *c,
                               uint8_t *major,
                               uint8_t *addl,
                               uint64_t *len,
                               bool *indef)
{
    uint8_t ib;
    if (!cbor_read_u8(c, &ib)) {
        return false;
    }

    *major = (ib >> 5) & 0x07;
    *addl  = ib & 0x1f;
    *indef = false;
    *len   = 0;

    if (*addl == 31) {
        // Indefinite length (we don't support this here).
        *indef = true;
        return true;
    }

    uint64_t v;
    if (!cbor_read_uint_n(c, *addl, &v)) {
        return false;
    }
    *len = v;
    return true;
}

static bool cbor_read_text(cbor_buf_t *c, char *out, size_t max_len)
{
    uint8_t major, addl;
    uint64_t len;
    bool indef;

    if (!cbor_read_type_len(c, &major, &addl, &len, &indef)) return false;
    if (indef || major != 3) {
        return false;
    }
    if (len + 1 > max_len) {
        // Key too long for our buffer; skip bytes and fail.
        if (c->pos + len > c->len) return false;
        c->pos += len;
        return false;
    }
    if (c->pos + len > c->len) return false;
    memcpy(out, c->buf + c->pos, len);
    c->pos += len;
    out[len] = '\0';
    return true;
}

// Decode half-precision (binary16) into float.
static float cbor_half_to_float(uint16_t h)
{
    uint16_t h_exp = (h & 0x7C00u);
    uint16_t h_sig = (h & 0x03FFu);
    uint32_t sign  = (uint32_t)(h & 0x8000u) << 16;
    uint32_t f;

    if (h_exp == 0) {
        // Subnormal or zero
        if (h_sig == 0) {
            f = sign;
        } else {
            // Normalize
            int shift = 0;
            while ((h_sig & 0x0400u) == 0) {
                h_sig <<= 1;
                shift++;
            }
            h_sig &= 0x03FFu;
            int32_t exp = 127 - 15 - shift;
            f = sign | ((uint32_t)(exp + 127) << 23) | ((uint32_t)h_sig << 13);
        }
    } else if (h_exp == 0x7C00u) {
        // Inf/NaN
        f = sign | 0x7F800000u | ((uint32_t)h_sig << 13);
    } else {
        // Normalized
        int32_t exp = (int32_t)(h_exp >> 10) - 15 + 127;
        f = sign | ((uint32_t)exp << 23) | ((uint32_t)h_sig << 13);
    }

    float result;
    memcpy(&result, &f, sizeof(result));
    return result;
}

// Generic numeric reader: accepts ints and floats, returns as float.
static bool cbor_read_number(cbor_buf_t *c, float *out)
{
    uint8_t ib;
    if (!cbor_read_u8(c, &ib)) {
        return false;
    }

    uint8_t major = (ib >> 5) & 0x07;
    uint8_t addl  = ib & 0x1f;

    uint8_t buf[8];
    uint64_t u;
    int64_t  s;

    switch (major) {
        case 0: // unsigned int
            if (!cbor_read_uint_n(c, addl, &u)) {
                return false;
            }
            *out = (float)u;
            return true;

        case 1: // negative int (value = -1 - n)
            if (!cbor_read_uint_n(c, addl, &u)) {
                return false;
            }
            s = -1 - (int64_t)u;
            *out = (float)s;
            return true;

        case 7: // float / simple
            if (addl == 25) {
                // Half-precision
                if (!cbor_read_bytes(c, buf, 2)) return false;
                {
                    uint16_t h = ((uint16_t)buf[0] << 8) | buf[1];
                    *out = cbor_half_to_float(h);
                }
                return true;
            }
            else if (addl == 26) {
                // Single-precision
                if (!cbor_read_bytes(c, buf, 4)) return false;
                uint32_t v = ((uint32_t)buf[0] << 24) |
                             ((uint32_t)buf[1] << 16) |
                             ((uint32_t)buf[2] << 8)  |
                             ((uint32_t)buf[3]);
                float f;
                memcpy(&f, &v, sizeof(f));
                *out = f;
                return true;
            }
            else if (addl == 27) {
                // Double-precision
                if (!cbor_read_bytes(c, buf, 8)) return false;
                uint64_t v = ((uint64_t)buf[0] << 56) |
                             ((uint64_t)buf[1] << 48) |
                             ((uint64_t)buf[2] << 40) |
                             ((uint64_t)buf[3] << 32) |
                             ((uint64_t)buf[4] << 24) |
                             ((uint64_t)buf[5] << 16) |
                             ((uint64_t)buf[6] << 8)  |
                             ((uint64_t)buf[7]);
                double d;
                memcpy(&d, &v, sizeof(d));
                *out = (float)d;
                return true;
            }
            else {
                // Simple values (false, true, null, etc.) not expected here.
                return false;
            }

        default:
            // We don't expect other major types (arrays, maps, tags, strings) as scalars.
            return false;
    }
}

// Old cbor_read_float is left here but no longer used directly by the values reader.
// Keeping it for completeness; could be removed if desired.
static bool cbor_read_float(cbor_buf_t *c, float *out)
{
    return cbor_read_number(c, out);
}

// Forward declaration
static bool cbor_skip_item(cbor_buf_t *c);

static bool cbor_skip_n_items(cbor_buf_t *c, uint64_t count)
{
    for (uint64_t i = 0; i < count; i++) {
        if (!cbor_skip_item(c)) return false;
    }
    return true;
}

// Skip a single CBOR data item recursively.
static bool cbor_skip_item(cbor_buf_t *c)
{
    uint8_t ib;
    if (!cbor_read_u8(c, &ib)) return false;

    uint8_t major = (ib >> 5) & 0x07;
    uint8_t addl  = ib & 0x1f;

    if (addl == 31) {
        // Indefinite lengths are not expected in ingestion CBOR for JSON-like payloads.
        return false;
    }

    uint64_t len;
    if (!cbor_read_uint_n(c, addl, &len)) return false;

    switch (major) {
        case 0: // unsigned int
        case 1: // negative int
            // Already consumed full integer (header + extra bytes).
            return true;

        case 2: // byte string
        case 3: // text string
            if (c->pos + len > c->len) return false;
            c->pos += len;
            return true;

        case 4: // array
            return cbor_skip_n_items(c, len);

        case 5: // map
            // map of length `len` => 2*len items (key + value)
            return cbor_skip_n_items(c, len * 2);

        case 6: // tag
            // Tag number already read as 'len'; now skip the tagged item itself.
            return cbor_skip_item(c);

        case 7: // float/simple
            // Header + extra bytes already consumed.
            return true;

        default:
            return false;
    }
}

// Parse values[] from a payload map into a flat float buffer.
//
// EI ingestion for motion often uses frames like:
//   [timestamp_ms, ax, ay, az]
// or just:
//   [ax, ay, az]
//
// We want only the last N_AXES elements per frame (ax, ay, az), and we must
// handle both integer and float encodings.
static bool cbor_read_values_array(cbor_buf_t *c, float **out_raw, size_t *out_total_floats)
{
    uint8_t major, addl;
    uint64_t n_frames;
    bool indef;

    if (!cbor_read_type_len(c, &major, &addl, &n_frames, &indef)) {
        return false;
    }
    if (indef || major != 4) {
        fprintf(stderr, "Expected values[] to be a definite-length array\n");
        return false;
    }

    if (n_frames == 0) {
        *out_raw = NULL;
        *out_total_floats = 0;
        return true;
    }

    size_t axis_count = 0;          // actual axes we keep per frame (should be N_AXES)
    size_t frame_items = 0;         // number of numeric items per frame in CBOR
    size_t skip_leading = 0;        // how many leading items to skip (e.g. timestamp)
    float *values = NULL;

    for (uint64_t frame = 0; frame < n_frames; frame++) {
        uint8_t m2, a2;
        uint64_t n_items64;
        bool indef2;

        if (!cbor_read_type_len(c, &m2, &a2, &n_items64, &indef2)) {
            free(values);
            return false;
        }
        if (indef2 || m2 != 4) {
            fprintf(stderr, "Expected frame %" PRIu64 " to be an array of numeric items\n", frame);
            free(values);
            return false;
        }

        if (n_items64 == 0) {
            fprintf(stderr, "Frame %" PRIu64 " has zero items\n", frame);
            free(values);
            return false;
        }

        size_t n_items = (size_t)n_items64;

        if (frame == 0) {
            // Determine layout based on first frame.
            frame_items = n_items;

            if (frame_items == N_AXES) {
                // [ax, ay, az]
                skip_leading = 0;
                axis_count   = N_AXES;
            }
            else if (frame_items == N_AXES + 1) {
                // [timestamp, ax, ay, az]
                skip_leading = 1;
                axis_count   = N_AXES;
            }
            else {
                fprintf(stderr,
                        "Unsupported frame layout: %" PRIu64 " items, expected %u or %u (with timestamp)\n",
                        n_items64, (unsigned)N_AXES, (unsigned)(N_AXES + 1));
                return false;
            }

            size_t total = (size_t)n_frames * axis_count;
            values = (float *)malloc(total * sizeof(float));
            if (!values) {
                fprintf(stderr, "Failed to allocate float buffer for %u frames x %u axes\n",
                        (unsigned)n_frames, (unsigned)axis_count);
                return false;
            }
        }
        else {
            // Subsequent frames must have the same item count as the first.
            if (n_items != frame_items) {
                fprintf(stderr,
                        "Frame %" PRIu64 " item count mismatch (%u vs %u in frame 0)\n",
                        frame, (unsigned)n_items, (unsigned)frame_items);
                free(values);
                return false;
            }
        }

        // Read all items in this frame, but only keep the last N_AXES after
        // skipping the optional timestamp.
        for (size_t idx = 0; idx < frame_items; idx++) {
            float v;
            if (!cbor_read_number(c, &v)) {
                fprintf(stderr, "Failed to read numeric value at frame=%" PRIu64 ", index=%u\n",
                        frame, (unsigned)idx);
                free(values);
                return false;
            }

            // Store only the axes (skip leading timestamp if present).
            if (idx >= skip_leading && (idx - skip_leading) < axis_count) {
                size_t axis_idx = idx - skip_leading;
                values[frame * axis_count + axis_idx] = v;
            }
        }
    }

    // At this point axis_count should always be N_AXES if we reached here.
    if (axis_count != N_AXES) {
        fprintf(stderr,
                "Internal error: axis_count=%u but N_AXES=%u\n",
                (unsigned)axis_count, (unsigned)N_AXES);
        free(values);
        return false;
    }

    *out_raw = values;
    *out_total_floats = (size_t)n_frames * axis_count;
    return true;
}

// Decode CBOR ingestion file and extract payload.values as floats.
static bool decode_cbor_file(const char *path, float **out_raw, size_t *out_total_floats)
{
    *out_raw = NULL;
    *out_total_floats = 0;

    // Read entire file into memory
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return false;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "Failed to seek %s\n", path);
        fclose(f);
        return false;
    }

    long fsize = ftell(f);
    if (fsize < 0) {
        fprintf(stderr, "Failed to tell size of %s\n", path);
        fclose(f);
        return false;
    }
    rewind(f);

    uint8_t *buf = (uint8_t *)malloc((size_t)fsize);
    if (!buf) {
        fprintf(stderr, "Failed to allocate %ld bytes for %s\n", fsize, path);
        fclose(f);
        return false;
    }

    size_t nread = fread(buf, 1, (size_t)fsize, f);
    fclose(f);
    if (nread != (size_t)fsize) {
        fprintf(stderr, "Short read on %s (%zu vs %ld)\n", path, nread, fsize);
        free(buf);
        return false;
    }

    cbor_buf_t c = { buf, (size_t)fsize, 0 };

    uint8_t major, addl;
    uint64_t n_pairs;
    bool indef;

    // Root object must be a map.
    if (!cbor_read_type_len(&c, &major, &addl, &n_pairs, &indef)) {
        fprintf(stderr, "CBOR decode error: cannot read root header in %s\n", path);
        free(buf);
        return false;
    }
    if (indef || major != 5) {
        fprintf(stderr, "CBOR root is not a definite-length map in %s\n", path);
        free(buf);
        return false;
    }

    bool found_payload = false;
    bool found_values  = false;
    float *values      = NULL;
    size_t total_floats = 0;

    for (uint64_t i = 0; i < n_pairs; i++) {
        char key[CBOR_MAX_TSTR_LEN];

        if (!cbor_read_text(&c, key, sizeof(key))) {
            fprintf(stderr, "Failed to read root key in %s\n", path);
            free(buf);
            return false;
        }

        if (strcmp(key, "payload") == 0) {
            found_payload = true;

            // Payload is a map
            uint8_t m2, a2;
            uint64_t n_pairs2;
            bool indef2;
            if (!cbor_read_type_len(&c, &m2, &a2, &n_pairs2, &indef2)) {
                fprintf(stderr, "Failed to read payload header in %s\n", path);
                free(buf);
                return false;
            }
            if (indef2 || m2 != 5) {
                fprintf(stderr, "Payload is not a definite-length map in %s\n", path);
                free(buf);
                return false;
            }

            for (uint64_t j = 0; j < n_pairs2; j++) {
                char pkey[CBOR_MAX_TSTR_LEN];
                if (!cbor_read_text(&c, pkey, sizeof(pkey))) {
                    fprintf(stderr, "Failed to read payload key in %s\n", path);
                    free(values);
                    free(buf);
                    return false;
                }

                if (strcmp(pkey, "values") == 0) {
                    if (!cbor_read_values_array(&c, &values, &total_floats)) {
                        fprintf(stderr, "Failed to read payload.values in %s\n", path);
                        free(values);
                        free(buf);
                        return false;
                    }
                    found_values = true;
                }
                else {
                    // Skip other payload fields
                    if (!cbor_skip_item(&c)) {
                        fprintf(stderr, "Failed to skip payload field '%s' in %s\n", pkey, path);
                        free(values);
                        free(buf);
                        return false;
                    }
                }
            }
        }
        else {
            // Skip non-payload top-level fields ("protected", "signature", etc.)
            if (!cbor_skip_item(&c)) {
                fprintf(stderr, "Failed to skip root field '%s' in %s\n", key, path);
                free(values);
                free(buf);
                return false;
            }
        }
    }

    free(buf);

    if (!found_payload || !found_values) {
        fprintf(stderr, "Did not find payload/values in %s\n", path);
        free(values);
        return false;
    }

    *out_raw = values;
    *out_total_floats = total_floats;
    return true;
}

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

    size_t *start_frames = (size_t *)malloc(chunks_to_emit * sizeof(size_t));
    if (!start_frames) {
        fprintf(stderr, "Sample %s: failed to allocate start_frames array\n",
                sample_name);
        return;
    }

#if WINDOW_SELECTION_MODE == WINDOW_MODE_RANDOM
    // --------------------------- Random selection ---------------------------
    size_t *all_starts = (size_t *)malloc(available_windows * sizeof(size_t));
    if (!all_starts) {
        fprintf(stderr, "Sample %s: failed to allocate all_starts array\n",
                sample_name);
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
    }
    else {
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

// Scan directory for files whose names contain ".cbor" and build a list.
static bool scan_sample_files(const char *dir,
                              char ***out_files,
                              size_t *out_count)
{
    *out_files = NULL;
    *out_count = 0;

    DIR *d = opendir(dir);
    if (!d) {
        fprintf(stderr, "Failed to open directory '%s': %s\n",
                dir, strerror(errno));
        return false;
    }

    size_t capacity = 16;
    size_t count    = 0;
    char **list     = (char **)malloc(capacity * sizeof(char *));
    if (!list) {
        fprintf(stderr, "Failed to allocate file list\n");
        closedir(d);
        return false;
    }

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

        char path[512];
        int n = snprintf(path, sizeof(path), "%s/%s", dir, name);
        if (n <= 0 || (size_t)n >= sizeof(path)) {
            fprintf(stderr, "Path too long, skipping %s/%s\n", dir, name);
            continue;
        }

        if (count == capacity) {
            capacity *= 2;
            char **tmp = (char **)realloc(list, capacity * sizeof(char *));
            if (!tmp) {
                fprintf(stderr, "Failed to grow file list\n");
                // Clean up previous entries
                for (size_t i = 0; i < count; i++) {
                    free(list[i]);
                }
                free(list);
                closedir(d);
                return false;
            }
            list = tmp;
        }

        list[count] = strdup(path);
        if (!list[count]) {
            fprintf(stderr, "Failed to allocate path string\n");
            // Clean up previous entries
            for (size_t i = 0; i < count; i++) {
                free(list[i]);
            }
            free(list);
            closedir(d);
            return false;
        }
        count++;
    }

    closedir(d);

    if (count == 0) {
        fprintf(stderr, "No *.cbor* files found in '%s'\n", dir);
        free(list);
        *out_files = NULL;
        *out_count = 0;
        return true;
    }

    *out_files = list;
    *out_count = count;
    return true;
}

// -----------------------------------------------------------------------------
// File-level processing
// -----------------------------------------------------------------------------

static void process_file(const char *path)
{
    float  *raw          = NULL;
    size_t  total_floats = 0;

    printf("Processing file: %s\n", path);

    if (!decode_cbor_file(path, &raw, &total_floats)) {
        fprintf(stderr, "Failed to decode CBOR file %s\n", path);
        return;
    }

    publish_windows_for_sample(path, raw, total_floats);

    free(raw);
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    // Optional: unbuffered stdout for immediate logging
    setvbuf(stdout, NULL, _IONBF, 0);

#if WINDOW_SELECTION_MODE == WINDOW_MODE_RANDOM
    // Seed RNG for random start positions
    srand((unsigned)time(NULL));
#endif

    const char *sample_dir = (argc > 1 && argv[1] && argv[1][0] != '\0')
                               ? argv[1]
                               : DEFAULT_SAMPLE_DIR;

    printf("CBOR publisher starting. Sample directory: '%s'\n", sample_dir);
    printf("CHUNKS_PER_SAMPLE=%d, SEND_DELAY_MS=%d, WINDOW_SELECTION_MODE=%s\n",
           CHUNKS_PER_SAMPLE,
           SEND_DELAY_MS,
#if WINDOW_SELECTION_MODE == WINDOW_MODE_RANDOM
           "RANDOM"
#elif WINDOW_SELECTION_MODE == WINDOW_MODE_DETERMINISTIC
           "DETERMINISTIC"
#else
           "UNKNOWN"
#endif
    );

    char **files = NULL;
    size_t num_files = 0;

    if (!scan_sample_files(sample_dir, &files, &num_files)) {
        fprintf(stderr, "Failed to scan sample directory '%s'\n", sample_dir);
        return 1;
    }

    if (num_files == 0) {
        fprintf(stderr, "No CBOR samples found, exiting.\n");
        return 1;
    }

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
