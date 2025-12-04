#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "edge-impulse-sdk/dsp/numpy_types.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "edge-impulse-sdk/classifier/ei_classifier_types.h"

static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = {
    1.0900, -0.0900, 5.2800, 1.2600, -0.3500, 4.3600, 1.5200, -0.6200, 4.1200, 1.5200, -0.6200, 4.1200, 1.5200, -0.6300, 3.4800, 1.6500, -1.0000, 2.7900, 1.4900, -1.2800, 2.9900, 1.6800, -1.5800, 3.2800, 1.8100, -1.5600, 3.2200, 1.8100, -1.5600, 3.2200, 1.4700, -1.2100, 2.7400, 1.2600, -0.5700, 3.5500, 1.0300, -0.6700, 5.5000, 0.5100, -0.2200, 6.0900, 0.9200, 0.6300, 6.4900, 1.8600, -0.4600, 6.8400, 1.8600, -0.4600, 6.8400, 1.8100, -1.8600, 7.3000, 1.7200, -1.9100, 6.5200, 2.2700, -0.4500, 5.4700, 3.3600, 0.4500, 5.6100, 3.6400, 0.3800, 7.4700, 2.6100, -0.8300, 9.8500, 2.6100, -0.8300, 9.8500, 1.1800, -1.8600, 10.1100, 1.5000, -1.1300, 10.7700, 2.6800, 1.0800, 10.9200, 2.6100, 2.5900, 11.0300, 1.5400, 1.8400, 12.2600, 0.5200, 1.0200, 12.1500, 0.5200, 1.0200, 12.1500, 0.0800, 0.9100, 10.5900, 0.3200, 0.7600, 11.5300, 0.7400, 2.9600, 13.8700, 1.0600, 3.7200, 12.6800, 1.0000, 2.7400, 12.8200, 0.6400, 1.4400, 11.8400, 0.6400, 1.4400, 11.8400, -0.0300, 0.7700, 12.2900, -0.3500, 0.8400, 12.1700, -0.1700, 1.2500, 11.4500, 0.1000, 0.8400, 11.2100, 0.0500, -0.1500, 12.5200, -0.4200, -0.2400, 14.0400, -0.4200, -0.2400, 14.0400, -0.4300, 0.8200, 13.7500, 0.8800, 0.6500, 11.6300, -0.1200, 1.3900, 13.2400, -0.3900, 0.2700, 12.6800, -1.1700, 0.3900, 13.3700, 0.5500, -2.2900, 12.6800, 0.5500, -2.2900, 12.6800, -1.3400, 0.2000, 11.9400, -0.3000, -0.4800, 13.6500, -2.0500, -2.0200, 16.7400, -2.5000, -3.5200, 18.6800, -2.7800, -2.8400, 17.8500, -2.0700, -0.7000, 14.4100, -2.0700, -0.7000, 14.4100, -1.6500, 0.3300, 12.9300, -1.6700, -0.2400, 14.1700, -2.0400, -1.5500, 15.9300
};
static size_t feature_ix = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;

int get_feature_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

// int read_features_file(const char *filename) {
//     char buffer[1 * 1024 * 1024] = { 0 };
//     FILE *f = (FILE*)fopen(filename, "r");
//     if (!f) {
//         printf("Cannot open file %s\n", filename);
//         return -1;
//     }
//     fseek(f, 0, SEEK_END);
//     size_t size = ftell(f);
//     rewind(f);
//     fread(buffer, 1, size, f);
//     fclose(f);

//     char *token = strtok(buffer, ",");
//     while (token != NULL) {
//         features[feature_ix++] = strtof(token, NULL);
//         token = strtok(NULL, " ");
//     }
//     return 0;
// }

EI_IMPULSE_ERROR run_classifier(signal_t *, ei_impulse_result_t *, bool);

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);             // Logs don't show up reliably so disable stdout buffering
    printf("EI Hello World!\n");
    // if (argc != 2) {
    //     printf("Requires one parameter (a comma-separated list of raw features, or a file pointing at raw features)\n");
    //     return 1;
    // }

    // if (!strchr(argv[1], ' ') && strchr(argv[1], '.')) { // looks like a filename
    //     int r = read_features_file(argv[1]);
    //     if (r != 0) {
    //         return 1;
    //     }
    // }
    // else { // looks like a features array passed in as an argument
    //     char *token = strtok(argv[1], ",");
    //     while (token != NULL) {
    //         features[feature_ix++] = strtof(token, NULL);
    //         token = strtok(NULL, ",");
    //     }
    // }

    if (feature_ix != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        printf("The size of your 'features' array is not correct. Expected %d items, but had %lu\n",
            EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, feature_ix);
        return 1;
    }

    signal_t signal;
    signal.total_length = feature_ix;
    signal.get_data = &get_feature_data;

    ei_impulse_result_t result;

    printf("Running classifier...\n");
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, true);
    printf("run_classifier returned: %d\n", res);

    printf("Begin output\n");

    // print the predictions
    printf("[");
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

    printf("End output\n");
}
