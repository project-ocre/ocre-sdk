#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "edge-impulse-sdk/dsp/numpy_types.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "edge-impulse-sdk/classifier/ei_classifier_types.h"
#include <ocre_api.h>

#ifndef LOG_PREFIX
#define LOG_PREFIX "[CLS] "
#endif

// --------------------------------------------------------------------------
// Bus configuration
// --------------------------------------------------------------------------
#define RAW_TOPIC             "ei/sample/raw"
#define RAW_CONTENT_TYPE      "application/ei-bus-f32"
#define RESULT_TOPIC          "ei/result"
#define RESULT_CONTENT_TYPE   "text/plain"

// --------------------------------------------------------------------------
// Edge Impulse feature buffer & signal callback
// --------------------------------------------------------------------------
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
static size_t feature_ix = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;

int get_feature_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

EI_IMPULSE_ERROR run_classifier(signal_t *, ei_impulse_result_t *, bool);

// --------------------------------------------------------------------------
// Forward declarations
// --------------------------------------------------------------------------
static void message_handler(const char *topic,
                            const char *content_type,
                            const void *payload,
                            uint32_t payload_len);

static void publish_result(const ei_impulse_result_t *result);

// --------------------------------------------------------------------------
// WASM entry point
// --------------------------------------------------------------------------
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0); // disable stdout buffering for logs

    printf(LOG_PREFIX "EI classifier subscriber starting up (closed-loop responder)...\n");

    int ret = ocre_register_message_callback(RAW_TOPIC, message_handler);
    if (ret != OCRE_SUCCESS) {
        printf(LOG_PREFIX "Error: Failed to register message callback for %s (ret=%d)\n",
               RAW_TOPIC, ret);
    }

    ret = ocre_subscribe_message(RAW_TOPIC);
    if (ret != OCRE_SUCCESS) {
        printf(LOG_PREFIX "Error: Failed to subscribe to topic %s (ret=%d)\n",
               RAW_TOPIC, ret);
        ocre_unregister_message_callback(RAW_TOPIC);
    }

    printf(LOG_PREFIX "Listening for samples on topic '%s' (content_type=%s)\n",
           RAW_TOPIC, RAW_CONTENT_TYPE);
    printf(LOG_PREFIX "Publishing results on topic '%s' (content_type=%s)\n",
           RESULT_TOPIC, RESULT_CONTENT_TYPE);

    while (1) {
        ocre_process_events();
    }

    return 0;
}

// --------------------------------------------------------------------------
// Message handler: run classifier on incoming raw float samples
// --------------------------------------------------------------------------
static void message_handler(const char *topic,
                            const char *content_type,
                            const void *payload,
                            uint32_t payload_len)
{
    if (!topic || !content_type || !payload) {
        printf(LOG_PREFIX "Invalid message data received\n");
        return;
    }

    // printf(LOG_PREFIX "Received message: topic=%s, content_type=%s, len=%u\n",
        //    topic, content_type, payload_len);

    if (strcmp(topic, RAW_TOPIC) != 0) {
        printf(LOG_PREFIX "Ignoring message on unexpected topic '%s'\n", topic);
        return;
    }

    if (strcmp(content_type, RAW_CONTENT_TYPE) != 0) {
        printf(LOG_PREFIX "Ignoring message with unexpected content_type '%s'\n",
               content_type);
        return;
    }

    if (payload_len == 0) {
        printf(LOG_PREFIX "Payload is empty\n");
        return;
    }

    if (payload_len % sizeof(float) != 0) {
        printf(LOG_PREFIX "Payload length (%u) is not a multiple of sizeof(float)=%zu\n",
               payload_len, sizeof(float));
        return;
    }

    size_t total_values = payload_len / sizeof(float);
    const float *src = (const float *)payload;

    // ----------------------------------------------------------------------
    // Map raw float samples into EI feature buffer
    // ----------------------------------------------------------------------
    size_t num_needed = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    size_t to_copy    = (total_values < num_needed) ? total_values : num_needed;

    memcpy(features, src, to_copy * sizeof(float));
    if (to_copy < num_needed) {
        memset(features + to_copy, 0,
               (num_needed - to_copy) * sizeof(float));
    }

    feature_ix = num_needed; // matches total_length of the signal

    // printf(LOG_PREFIX "Running classifier on %zu features (received %zu floats)\n",
        //    num_needed, total_values);

    signal_t signal;
    signal.total_length = feature_ix;
    signal.get_data     = &get_feature_data;

    ei_impulse_result_t result;
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
    // printf(LOG_PREFIX "run_classifier returned: %d\n", res);

    if (res != EI_IMPULSE_OK) {
        printf(LOG_PREFIX "Classifier error: %d\n", res);
        return;
    }

    // ----------------------------------------------------------------------
    // Print predictions (same format as original single-shot app)
    // ----------------------------------------------------------------------
    // printf(LOG_PREFIX "Begin output\n");
    printf(LOG_PREFIX "[");

    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        printf("%.5f", result.classification[ix].value);
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        printf(", ");
#else
        if (ix != EI_CLASSIFIER_LABEL_COUNT - 1) {
            printf(", ");
        }
#endif
    }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
    printf("%.3f", result.anomaly);
#endif
    printf("]\n");
    // printf(LOG_PREFIX "End output\n");

    // ----------------------------------------------------------------------
    // Publish a simple top-1 result for closed-loop driver
    // ----------------------------------------------------------------------
    publish_result(&result);
}

// --------------------------------------------------------------------------
// Publish top-1 classification result as a simple text message
// --------------------------------------------------------------------------
static void publish_result(const ei_impulse_result_t *result)
{
    if (!result) {
        return;
    }

    const char *top_label = NULL;
    float top_score = -1.0f;

    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (result->classification[ix].value > top_score) {
            top_score = result->classification[ix].value;
            top_label = result->classification[ix].label;
        }
    }

    if (!top_label) {
        printf(LOG_PREFIX "No labels found in result; not publishing\n");
        return;
    }

    char payload[128];
    int n = snprintf(payload, sizeof(payload),
                     "label=%s score=%.5f", top_label, top_score);
    if (n < 0 || n >= (int)sizeof(payload)) {
        printf(LOG_PREFIX "Result message truncated; not publishing\n");
        return;
    }

    int ret = ocre_publish_message(RESULT_TOPIC,
                                   RESULT_CONTENT_TYPE,
                                   payload,
                                   (uint32_t)(n + 1)); // include NUL
    if (ret == OCRE_SUCCESS) {
        printf(LOG_PREFIX "Published result: %s on topic %s\n", payload, RESULT_TOPIC);
    } else {
        printf(LOG_PREFIX "Failed to publish result (ret=%d)\n", ret);
    }
}
