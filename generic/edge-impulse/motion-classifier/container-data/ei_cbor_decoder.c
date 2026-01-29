#include "ei_cbor_decoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#ifndef EI_CBOR_LOG_PREFIX
#define EI_CBOR_LOG_PREFIX "[CBOR] "
#endif

#include "qcbor/qcbor.h"

/* Uncomment to enable extra debug logging for values[][] */
#define EI_CBOR_DEBUG_VALUES 0

/* Local helper: compare UsefulBufC to a C string literal */
static bool ub_equals_cstr(UsefulBufC ub, const char *s)
{
    size_t slen = strlen(s);
    return (ub.len == slen) && (ub.ptr != NULL) &&
           (memcmp(ub.ptr, s, slen) == 0);
}

/* First pass: discover number of frames and axes from payload.values */
static bool ei_cbor_find_values_dims(const uint8_t *buf, size_t len,
                                     size_t *out_frames,
                                     size_t *out_axes)
{
    QCBORDecodeContext dc;
    QCBORItem          item;
    QCBORError         err;

    *out_frames = 0;
    *out_axes   = 0;

    UsefulBufC ub = (UsefulBufC){ .ptr = buf, .len = len };
    QCBORDecode_Init(&dc, ub, QCBOR_DECODE_MODE_NORMAL);

    bool    in_values          = false;
    uint8_t values_array_level = 0;
    uint8_t frame_array_level  = 0;

    while ((err = QCBORDecode_GetNext(&dc, &item)) == QCBOR_SUCCESS) {

        if (!in_values) {
            /* Look for label "values" that is an array */
            if (item.uLabelType == QCBOR_TYPE_TEXT_STRING &&
                item.uDataType == QCBOR_TYPE_ARRAY &&
                ub_equals_cstr(item.label.string, "values"))
            {
                in_values          = true;
                values_array_level = item.uNestingLevel;
                frame_array_level  = item.uNextNestLevel;
                continue;
            }
        }
        else {
            /* We are inside values[]; exit when nesting level drops */
            if (item.uNestingLevel <= values_array_level) {
                break; /* Done scanning values array */
            }

            /* Each frame is itself an array at frame_array_level */
            if (item.uNestingLevel == frame_array_level &&
                item.uDataType == QCBOR_TYPE_ARRAY)
            {
                (*out_frames)++;

                if (*out_axes == 0) {
                    /* Number of items in each frame array */
                    *out_axes = (size_t)item.val.uCount;
                }
                else if (*out_axes != (size_t)item.val.uCount) {
                    printf(EI_CBOR_LOG_PREFIX
                           "CBOR values array has inconsistent axis counts "
                           "(got %llu, expected %zu)\n",
                           (unsigned long long)item.val.uCount, *out_axes);
                    return false;
                }
            }
        }
    }

    /* Check for decode error (other than 'no more items') */
    QCBORError finish_err = QCBORDecode_Finish(&dc);
    if (finish_err != QCBOR_SUCCESS) {
        printf(EI_CBOR_LOG_PREFIX "QCBOR dimension pass failed: %d\n",
               (int)finish_err);
        return false;
    }

    if (*out_frames == 0 || *out_axes == 0) {
        printf(EI_CBOR_LOG_PREFIX "Failed to find payload.values array in CBOR\n");
        return false;
    }

    return true;
}

/* Second pass just for values[][]: fills an existing values buffer.
 * This uses the same "find payload.values" logic as ei_cbor_find_values_dims,
 * but decodes each numeric element into the flat [frame * axes + axis] array.
 */
static bool ei_cbor_decode_values_only(const uint8_t *buf, size_t len,
                                       float *values,
                                       size_t frames, size_t axes)
{
    if (!buf || !values || frames == 0 || axes == 0) {
        return false;
    }

    QCBORDecodeContext dc;
    QCBORItem          item;
    QCBORError         err;

    UsefulBufC ub = (UsefulBufC){ .ptr = buf, .len = len };
    QCBORDecode_Init(&dc, ub, QCBOR_DECODE_MODE_NORMAL);

    bool    in_values          = false;
    uint8_t values_array_level = 0;
    uint8_t frame_array_level  = 0;

    size_t frame_index = 0;
    size_t axis_index  = 0;
    size_t decoded     = 0;

    while ((err = QCBORDecode_GetNext(&dc, &item)) == QCBOR_SUCCESS) {

        if (!in_values) {
            /* Look for label "values" that is an array, same as
             * ei_cbor_find_values_dims().
             */
            if (item.uLabelType == QCBOR_TYPE_TEXT_STRING &&
                item.uDataType == QCBOR_TYPE_ARRAY &&
                ub_equals_cstr(item.label.string, "values"))
            {
                in_values          = true;
                values_array_level = item.uNestingLevel;
                frame_array_level  = item.uNextNestLevel;
                frame_index        = 0;
                axis_index         = 0;
                continue;
            }
        }
        else {
            /* We are inside values[][]; exit when nesting drops back. */
            if (item.uNestingLevel <= values_array_level) {
                break; /* Done scanning values array */
            }

            /* Each frame is itself an array at frame_array_level. */
            if (item.uNestingLevel == frame_array_level &&
                item.uDataType == QCBOR_TYPE_ARRAY)
            {
                /* Start of a new frame */
                axis_index = 0;

                /* Optionally sanity-check number of axes */
                if (item.val.uCount != (uint64_t)axes) {
                    printf(EI_CBOR_LOG_PREFIX
                           "Warning: frame has %u axes, expected %zu\n",
                           (unsigned)item.val.uCount, axes);
                }

                if (frame_index >= frames) {
                    printf(EI_CBOR_LOG_PREFIX
                           "Warning: extra frame %zu (max %zu)\n",
                           frame_index, frames);
                }

                frame_index++;
                continue;
            }

            /* Numeric scalars inside each frame array live one level deeper. */
            if (item.uNestingLevel == (uint8_t)(frame_array_level + 1)) {

                double v = 0.0;
                bool   is_number = false;

                switch (item.uDataType) {
                    case QCBOR_TYPE_DOUBLE:
                        v = item.val.dfnum;
                        is_number = true;
                        break;
                    case QCBOR_TYPE_INT64:
                        v = (double)item.val.int64;
                        is_number = true;
                        break;
                    case QCBOR_TYPE_UINT64:
                        v = (double)item.val.uint64;
                        is_number = true;
                        break;
                    default:
                        /* Other types (e.g. bool, text) are ignored */
                        break;
                }

                if (is_number) {
                    /* Note: frame_index was pre-incremented on seeing the
                     * frame array above, so we store into frame_index-1.
                     */
                    size_t f = (frame_index == 0) ? 0 : (frame_index - 1);
                    size_t a = axis_index;

                    if (f < frames && a < axes) {
                        size_t idx = f * axes + a;
                        values[idx] = (float)v;
                    }

                    decoded++;

                    axis_index++;
                    if (axis_index >= axes) {
                        axis_index = 0;
                    }
                }
            }
        }
    }

    QCBORError finish_err = QCBORDecode_Finish(&dc);
    if (finish_err != QCBOR_SUCCESS) {
        printf(EI_CBOR_LOG_PREFIX "QCBOR values-only decode failed: %d\n",
               (int)finish_err);
        return false;
    }

    if (decoded == 0) {
        printf(EI_CBOR_LOG_PREFIX
               "No numeric entries decoded from payload.values\n");
        return false;
    }

    size_t expected = frames * axes;
    if (decoded != expected) {
        printf(EI_CBOR_LOG_PREFIX
               "Warning: decoded %zu values, expected %zu (%zu frames * %zu axes)\n",
               decoded, expected, frames, axes);
        /* Not fatal – we still return true, but some values may remain zero. */
    }

    return true;
}

/* Second pass: fill metadata + values[] */
static bool ei_cbor_decode_full(const uint8_t *buf, size_t len,
                                ei_cbor_sample_t *out,
                                size_t frames, size_t axes)
{
    QCBORDecodeContext dc;
    QCBORItem          item;
    QCBORError         err;

    UsefulBufC ub = (UsefulBufC){ .ptr = buf, .len = len };
    QCBORDecode_Init(&dc, ub, QCBOR_DECODE_MODE_NORMAL);

    memset(out, 0, sizeof(*out));
    out->n_frames = frames;
    out->n_axes   = axes;
    out->n_values = frames * axes;

    out->values = (float *)calloc(out->n_values, sizeof(float));
    if (!out->values) {
        printf(EI_CBOR_LOG_PREFIX "Failed to allocate values array (%zu floats)\n",
               out->n_values);
        return false;
    }

    bool    in_values          = false;
    uint8_t values_array_level = 0;
    uint8_t frame_array_level  = 0;

    size_t frame_index = 0;
    size_t axis_index  = 0;

    /* Sensors decoding state */
    bool    in_sensors           = false;
    uint8_t sensors_array_level  = 0;
    uint8_t sensor_map_level     = 0;
    uint8_t sensor_field_level   = 0;
    int     current_sensor_index = -1;

    while ((err = QCBORDecode_GetNext(&dc, &item)) == QCBOR_SUCCESS) {

        /* -------------------- payload metadata -------------------- */
        if (item.uLabelType == QCBOR_TYPE_TEXT_STRING) {
            UsefulBufC key = item.label.string;

            /* device_type */
            if (item.uDataType == QCBOR_TYPE_TEXT_STRING &&
                ub_equals_cstr(key, "device_type"))
            {
                size_t cpy_len = item.val.string.len;
                if (cpy_len >= sizeof(out->device_type)) {
                    cpy_len = sizeof(out->device_type) - 1;
                }
                memcpy(out->device_type, item.val.string.ptr, cpy_len);
                out->device_type[cpy_len] = '\0';
            }
            /* device_name */
            else if (item.uDataType == QCBOR_TYPE_TEXT_STRING &&
                     ub_equals_cstr(key, "device_name"))
            {
                size_t cpy_len = item.val.string.len;
                if (cpy_len >= sizeof(out->device_name)) {
                    cpy_len = sizeof(out->device_name) - 1;
                }
                memcpy(out->device_name, item.val.string.ptr, cpy_len);
                out->device_name[cpy_len] = '\0';
            }
            /* interval_ms */
            else if (ub_equals_cstr(key, "interval_ms")) {
                if (item.uDataType == QCBOR_TYPE_DOUBLE) {
                    out->interval_ms = item.val.dfnum;
                    out->has_interval_ms = true;
                }
                else if (item.uDataType == QCBOR_TYPE_UINT64) {
                    out->interval_ms = (double)item.val.uint64;
                    out->has_interval_ms = true;
                }
                else if (item.uDataType == QCBOR_TYPE_INT64) {
                    out->interval_ms = (double)item.val.int64;
                    out->has_interval_ms = true;
                }
            }
            /* sensors[] array */
            else if (item.uDataType == QCBOR_TYPE_ARRAY &&
                     ub_equals_cstr(key, "sensors"))
            {
                in_sensors          = true;
                sensors_array_level = item.uNestingLevel;
                sensor_map_level    = item.uNextNestLevel;
                current_sensor_index = -1;
            }
            /* values[][] array */
            else if (item.uDataType == QCBOR_TYPE_ARRAY &&
                     ub_equals_cstr(key, "values"))
            {
                in_values          = true;
                values_array_level = item.uNestingLevel;
                frame_array_level  = item.uNextNestLevel;
                frame_index        = 0;
                axis_index         = 0;
            }
        }

        /* -------------------- sensors decoding -------------------- */
        if (in_sensors) {
            if (item.uNestingLevel <= sensors_array_level) {
                /* Left the sensors[] array */
                in_sensors           = false;
                sensors_array_level  = 0;
                sensor_map_level     = 0;
                sensor_field_level   = 0;
                current_sensor_index = -1;
            }
            else if (item.uNestingLevel == sensor_map_level &&
                     item.uDataType == QCBOR_TYPE_MAP)
            {
                /* New sensor entry */
                if (out->n_sensors < EI_CBOR_MAX_SENSORS) {
                    current_sensor_index = (int)out->n_sensors++;
                } else {
                    current_sensor_index = -1; /* ignore extras */
                }
                sensor_field_level = item.uNextNestLevel;
            }
            else if (item.uNestingLevel == sensor_field_level &&
                     item.uLabelType == QCBOR_TYPE_TEXT_STRING &&
                     current_sensor_index >= 0 &&
                     current_sensor_index < (int)EI_CBOR_MAX_SENSORS)
            {
                ei_cbor_sensor_t *s = &out->sensors[current_sensor_index];
                UsefulBufC key = item.label.string;

                if (item.uDataType == QCBOR_TYPE_TEXT_STRING &&
                    ub_equals_cstr(key, "name"))
                {
                    size_t cpy_len = item.val.string.len;
                    if (cpy_len >= sizeof(s->name)) {
                        cpy_len = sizeof(s->name) - 1;
                    }
                    memcpy(s->name, item.val.string.ptr, cpy_len);
                    s->name[cpy_len] = '\0';
                }
                else if (item.uDataType == QCBOR_TYPE_TEXT_STRING &&
                         ub_equals_cstr(key, "units"))
                {
                    size_t cpy_len = item.val.string.len;
                    if (cpy_len >= sizeof(s->units)) {
                        cpy_len = sizeof(s->units) - 1;
                    }
                    memcpy(s->units, item.val.string.ptr, cpy_len);
                    s->units[cpy_len] = '\0';
                }
            }
        }

        /* -------------------- values decoding -------------------- */
        if (in_values)
        {
            if (item.uNestingLevel <= values_array_level) {
                /* Left values[][] */
                in_values          = false;
                values_array_level = 0;
                frame_array_level  = 0;
            }
            else if (item.uNestingLevel == frame_array_level &&
                     item.uDataType == QCBOR_TYPE_ARRAY)
            {
                /* Start of a new frame; reset axis index */
                axis_index = 0;
#if defined(EI_CBOR_DEBUG_VALUES)
                printf(EI_CBOR_LOG_PREFIX
                       "values: new frame %zu (array, uCount=%u, uNestingLevel=%u)\n",
                       frame_index, (unsigned)item.val.uCount,
                       (unsigned)item.uNestingLevel);
#endif
            }
            else if (item.uNestingLevel >= (uint8_t)(frame_array_level + 1) &&
                     item.uNestingLevel <= (uint8_t)(frame_array_level + 2))
            {
                /* We expect numeric scalars one level deeper than frame arrays.
                 * But we log everything here to see what QCBOR is giving us. */

#if defined(EI_CBOR_DEBUG_VALUES)
                printf(EI_CBOR_LOG_PREFIX
                       "values: item at nest=%u, type=%u\n",
                       (unsigned)item.uNestingLevel,
                       (unsigned)item.uDataType);
#endif

                /* Treat any basic numeric type as a sample value */
                double v;
                bool   is_number = false;

                switch (item.uDataType) {
                    case QCBOR_TYPE_DOUBLE:
                        v = item.val.dfnum;
                        is_number = true;
                        break;
#ifdef QCBOR_TYPE_FLOAT
                    case QCBOR_TYPE_FLOAT:
                        v = (double)item.val.fnum;
                        is_number = true;
                        break;
#endif
                    case QCBOR_TYPE_UINT64:
                        v = (double)item.val.uint64;
                        is_number = true;
                        break;
                    case QCBOR_TYPE_INT64:
                        v = (double)item.val.int64;
                        is_number = true;
                        break;
                    default:
                        /* Not a numeric type we know how to handle yet */
                        is_number = false;
                        break;
                }

                if (is_number) {
                    if (frame_index < frames && axis_index < axes) {
                        size_t idx = frame_index * axes + axis_index;
                        out->values[idx] = (float)v;

#if defined(EI_CBOR_DEBUG_VALUES)
                        if (frame_index < 3 && axis_index < 3) {
                            printf(EI_CBOR_LOG_PREFIX
                                   "values: set[%zu,%zu] = %f\n",
                                   frame_index, axis_index, (float)v);
                        }
#endif
                    }

                    axis_index++;
                    if (axis_index >= axes) {
                        axis_index = 0;
                        frame_index++;
                    }
                }
            }
        }
        // else {
        //     printf(EI_CBOR_LOG_PREFIX
        //            "Unexpected item outside values[] at nest=%u, type=%u\n",
        //            (unsigned)item.uNestingLevel,
        //            (unsigned)item.uDataType);
        // }
    }

    QCBORError finish_err = QCBORDecode_Finish(&dc);
    if (finish_err != QCBOR_SUCCESS) {
        printf(EI_CBOR_LOG_PREFIX "QCBOR full decode failed: %d\n",
               (int)finish_err);
        return false;
    }

    return true;
}

/* Public API: decode from buffer */
bool ei_cbor_decode_buffer(const uint8_t *buf, size_t len,
                           ei_cbor_sample_t *out)
{
    if (!buf || len == 0 || !out) {
        return false;
    }

    size_t frames = 0;
    size_t axes   = 0;

    /* First pass: discover dimensions. */
    if (!ei_cbor_find_values_dims(buf, len, &frames, &axes)) {
        return false;
    }

    /* Second pass: decode metadata + allocate values[].
     * (This already sets out->n_frames, out->n_axes, and calloc's values[].)
     */
    if (!ei_cbor_decode_full(buf, len, out, frames, axes)) {
        return false;
    }

    /* Third pass: robustly fill values[][] into out->values. We overwrite
     * whatever ei_cbor_decode_full() put there (currently all zeros).
     */
    if (!ei_cbor_decode_values_only(buf, len, out->values, frames, axes)) {
        return false;
    }

    return true;
}

/* Public API: decode from file on disk */
bool ei_cbor_decode_file(const char *path, ei_cbor_sample_t *out)
{
    if (!path || !out) {
        return false;
    }

    // ftell() is currently not supported on Zephyr - use stat instead.
    struct stat st;
    if (stat(path, &st) != 0) {
        printf(EI_CBOR_LOG_PREFIX "stat failed for %s: %s\n", path, strerror(errno));
        return false;
    }

    if (!S_ISREG(st.st_mode)) {
        printf(EI_CBOR_LOG_PREFIX "%s is not a regular file\n", path);
        return false;
    }

    long fsize = (long)st.st_size;
    if (fsize <= 0) {
        printf(EI_CBOR_LOG_PREFIX "%s has zero length\n", path);
        return false;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        printf(EI_CBOR_LOG_PREFIX "Failed to open %s: %s\n",
               path, strerror(errno));
        return false;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)fsize);
    if (!buf) {
        printf(EI_CBOR_LOG_PREFIX "Failed to allocate %ld bytes for %s\n",
               fsize, path);
        fclose(f);
        return false;
    }

    size_t nread = 0;
    while (nread < (size_t)fsize) {
        size_t r = fread(buf + nread, 1, (size_t)fsize - nread, f);
        if (r == 0) break;
        nread += r;
    }
    fclose(f);

    if (nread != (size_t)fsize) {
        printf(EI_CBOR_LOG_PREFIX "Short read from %s\n", path);
        free(buf);
        return false;
    }

    bool ok = ei_cbor_decode_buffer(buf, (size_t)fsize, out);
    free(buf);
    return ok;
}

/* Public API: free allocations */
void ei_cbor_free_sample(ei_cbor_sample_t *sample)
{
    if (!sample) {
        return;
    }
    free(sample->values);
    sample->values   = NULL;
    sample->n_values = 0;
    sample->n_frames = 0;
    sample->n_axes   = 0;
}
