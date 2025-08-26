/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 * RNG Sensor Continuous Reader Example
 */
#include <stdio.h>
#include <ocre_api.h>

int main(void)
{
    printf("=== RNG Sensor Continuous Reader Example ===\n");

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

    printf("\n=== Finding RNG Sensor by Name ===\n");

    // Try to open RNG sensor by name
    ocre_sensor_handle_t rng_handle_by_name;
    rng_handle_by_name = ocre_sensors_open_by_name("RNG Sensor");
    if (rng_handle_by_name != 0)
    {
        printf("Could not open RNG sensor by name 'RNG Sensor'\n");
    }
    else
    {
        printf("Successfully opened RNG sensor by name, handle: %d\n", rng_handle_by_name);

        // Get channel count by name
        int channel_count = ocre_sensors_get_channel_count_by_name("RNG Sensor");
        printf("RNG sensor (by name) has %d channels\n", channel_count);
    }

    printf("\n=== Finding RNG Sensor by Handle ===\n");

    // Search for RNG sensor by iterating through all sensors
    int rng_sensor_id = -1;
    ocre_sensor_handle_t rng_handle_by_id = -1;

    for (int sensor_id = 0; sensor_id < nr_of_sensors; sensor_id++)
    {
        // Get sensor handle
        int handle = ocre_sensors_get_handle(sensor_id);
        if (handle < 0)
            continue;

        // Try to open the sensor
        if (ocre_sensors_open(handle) == 0)
        {
            // Check if this might be the RNG (typically has 1 channel)
            int channel_count = ocre_sensors_get_channel_count(sensor_id);

            // RNG typically has 1 channel
            if (channel_count == 1)
            {
                printf("Found potential RNG sensor at ID %d with %d channel\n", sensor_id, channel_count);
                rng_sensor_id = sensor_id;
                rng_handle_by_id = handle;
                // Don't break here in case there are multiple single-channel sensors
                // In a real implementation, you'd have better identification
            }
        }
    }

    if (rng_sensor_id == -1)
    {
        printf("Could not find RNG sensor by handle iteration\n");
        printf("Continuing with name-based access only...\n");
    }
    else
    {
        printf("Successfully found RNG sensor by handle - ID: %d, Handle: %d\n",
               rng_sensor_id, rng_handle_by_id);
    }

    printf("\n=== Starting Continuous RNG Reading ===\n");
    printf("Reading RNG sensor every 3 seconds...\n");

    int reading_count = 0;

    while (1)
    {
        reading_count++;
        printf("\n--- RNG Reading #%d ---\n", reading_count);

        // Read using name-based API
        printf("Reading by name:\n");
        int channel_count_by_name = ocre_sensors_get_channel_count_by_name("RNG Sensor");
        if (channel_count_by_name > 0)
        {
            for (int j = 0; j < channel_count_by_name; j++)
            {
                int channel_type = ocre_sensors_get_channel_type_by_name("RNG Sensor", j);
                if (channel_type >= 0)
                {
                    int value = ocre_sensors_read_by_name("RNG Sensor", channel_type);
                    printf("  Channel %d (type %d): Random value = %d\n", j, channel_type, value);
                }
            }
        }
        else
        {
            printf("  Failed to get channel count by name\n");
        }

        // Read using handle-based API (if available)
        if (rng_sensor_id != -1)
        {
            printf("Reading by handle:\n");
            int channel_count_by_id = ocre_sensors_get_channel_count(rng_sensor_id);
            for (int j = 0; j < channel_count_by_id; j++)
            {
                int channel_type = ocre_sensors_get_channel_type(rng_sensor_id, j);
                if (channel_type >= 0)
                {
                    int value = ocre_sensors_read(rng_sensor_id, channel_type);
                    printf("  Channel %d (type %d): Random value = %d\n", j, channel_type, value);
                }
            }
        }

        printf("Waiting 3 seconds before next reading...\n");
        ocre_sleep(3000); // Wait 3 seconds
    }

    printf("RNG Sensor Reader exiting.\n");
    return 0;
}
