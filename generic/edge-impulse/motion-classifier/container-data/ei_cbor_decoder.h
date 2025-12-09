#ifndef EI_CBOR_DECODER_H
#define EI_CBOR_DECODER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Tunable limits for metadata; adjust if you have larger strings/sensor counts */
#define EI_CBOR_MAX_DEVICE_STR    64
#define EI_CBOR_MAX_SENSOR_NAME   32
#define EI_CBOR_MAX_SENSOR_UNITS  16
#define EI_CBOR_MAX_SENSORS       8

typedef struct {
    char name[EI_CBOR_MAX_SENSOR_NAME];
    char units[EI_CBOR_MAX_SENSOR_UNITS];
} ei_cbor_sensor_t;

typedef struct {
    /* Payload metadata (CBOR payload map) */
    char   device_type[EI_CBOR_MAX_DEVICE_STR];
    char   device_name[EI_CBOR_MAX_DEVICE_STR];

    double interval_ms;
    bool   has_interval_ms;

    uint32_t       n_sensors;
    ei_cbor_sensor_t sensors[EI_CBOR_MAX_SENSORS];

    /* Flattened values: frames × axes */
    float  *values;   /* heap-allocated, contiguous: frame0[axes], frame1[axes], ... */
    size_t  n_values; /* total number of floats */
    size_t  n_frames; /* number of rows in values array */
    size_t  n_axes;   /* number of columns per frame */

} ei_cbor_sample_t;

/**
 * Decode an Edge Impulse ingestion CBOR buffer into a structured sample.
 *
 * Expected structure (simplified):
 *   {
 *     "protected":  ...,
 *     "signature":  ...,
 *     "payload": {
 *       "device_type": "XXX",
 *       "device_name": "YYY",
 *       "interval_ms": 16.0,
 *       "sensors": [ { "name": "...", "units": "..." }, ... ],
 *       "values":  [ [f0, f1, ...], [f0, f1, ...], ... ]
 *     }
 *   }
 *
 * This decoder is tolerant of extra fields and will ignore unknown keys.
 *
 * On success:
 *   - out->values is heap-allocated and must be freed with ei_cbor_free_sample()
 *   - metadata fields are populated when present; missing ones are left empty.
 */
bool ei_cbor_decode_buffer(const uint8_t *buf, size_t len,
                           ei_cbor_sample_t *out);

/**
 * Convenience wrapper: read a CBOR file from disk and decode it.
 */
bool ei_cbor_decode_file(const char *path, ei_cbor_sample_t *out);

/**
 * Free heap allocations inside the sample (currently only values[]).
 * Safe to call on a zero-initialized ei_cbor_sample_t as well.
 */
void ei_cbor_free_sample(ei_cbor_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* EI_CBOR_DECODER_H */
