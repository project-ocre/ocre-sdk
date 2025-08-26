/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 * Sensor Discovery and Read Once Example
 */
#include <stdio.h>
#include <ocre_api.h>

int main(void)
{
    printf("=== Sensor Discovery and Read Once Example ===\n");

    // Initialize the sensor API
    int ret = ocre_sensors_init();
    if (ret != 0)
    {
        printf("Error: Sensors not initialized (code: %d)\n", ret);
        return -1;
    }
    printf("Sensors initialized successfully\n");

    // Discover available sensors
    int nr_of_sensors = ocre_sensors_discover();
    printf("Sensors found: %d\n", nr_of_sensors);

    if (nr_of_sensors <= 0)
    {
        printf("Error: No sensors discovered\n");
        return -1;
    }

    printf("\n=== Reading All Discovered Sensors ===\n");

    // Iterate through all sensors
    for (int sensor_id = 0; sensor_id < nr_of_sensors; sensor_id++)
    {
        printf("\n--- Sensor ID: %d ---\n", sensor_id);

        // Get sensor handle
        int handle = ocre_sensors_get_handle(sensor_id);
        if (handle < 0)
        {
            printf("Failed to get handle for sensor %d (error: %d)\n", sensor_id, handle);
            continue;
        }
        printf("Sensor handle: %d\n", handle);

        // Open the sensor
        if (ocre_sensors_open(handle) != 0)
        {
            printf("Failed to open sensor %d\n", sensor_id);
            continue;
        }
        printf("Sensor %d opened successfully\n", sensor_id);

        // Get channel count
        int channel_count = ocre_sensors_get_channel_count(sensor_id);
        if (channel_count < 0)
        {
            printf("Failed to get channel count for sensor %d (error: %d)\n", sensor_id, channel_count);
            continue;
        }
        printf("Sensor %d has %d channels\n", sensor_id, channel_count);

        // Read from each channel
        for (int channel_idx = 0; channel_idx < channel_count; channel_idx++)
        {
            // Get channel type
            int channel_type = ocre_sensors_get_channel_type(sensor_id, channel_idx);
            if (channel_type < 0)
            {
                printf("  Channel %d: Failed to get type (error: %d)\n", channel_idx, channel_type);
                continue;
            }

            // Read sensor data
            double value = ocre_sensors_read(sensor_id, channel_type);

            printf("  Channel %d (type %d): Value = %.2f\n",
                   channel_idx, channel_type, value);
        }
    }

    printf("\n=== Sensor Discovery Complete ===\n");
    printf("All sensors have been discovered and read once.\n");

    return 0;
}
