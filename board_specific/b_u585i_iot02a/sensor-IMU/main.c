/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 * IMU Sensor Continuous Reader Example
 */
#include <stdio.h>
#include <ocre_api.h>

int main(void)
{
    printf("=== IMU Sensor Continuous Reader Example ===\n");

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

    printf("\n=== Finding IMU Sensor by Name ===\n");

    // Try to open IMU sensor by name
    if (ocre_sensors_open_by_name("imu") != 0)
    {
        printf("Could not open IMU sensor by name 'imu'\n");
    }
    else
    {
        printf("Successfully opened IMU sensor by name, handle: %s\n", "imu");

        // Get channel count by name
        int channel_count = ocre_sensors_get_channel_count_by_name("imu");
        printf("IMU sensor (by name) has %d channels\n", channel_count);
    }

    printf("\n=== Finding IMU Sensor by Handle ===\n");

    // Search for IMU sensor by iterating through all sensors
    int imu_sensor_id = -1;
    ocre_sensor_handle_t imu_handle_by_id = -1;

    for (int sensor_id = 0; sensor_id < nr_of_sensors; sensor_id++)
    {
        // Get sensor handle
        int handle = ocre_sensors_get_handle(sensor_id);
        if (handle < 0)
            continue;

        // Try to open the sensor
        if (ocre_sensors_open(handle) == 0)
        {
            // Check if this might be the IMU (you might need to check sensor name or properties)
            // For this example, we'll assume sensor ID 1 is IMU or try to find it
            // In a real implementation, you'd have a way to identify the IMU sensor
            int channel_count = ocre_sensors_get_channel_count(sensor_id);

            // IMU typically has multiple channels (accel X,Y,Z, gyro X,Y,Z, etc.)
            if (channel_count >= 3) // Assuming IMU has at least 3 channels
            {
                printf("Found potential IMU sensor at ID %d with %d channels\n", sensor_id, channel_count);
                imu_sensor_id = sensor_id;
                imu_handle_by_id = handle;
                break;
            }
        }
    }

    if (imu_sensor_id == -1)
    {
        printf("Could not find IMU sensor by handle iteration\n");
        return -1;
    }

    printf("Successfully found IMU sensor by handle - ID: %d, Handle: %d\n",
           imu_sensor_id, imu_handle_by_id);

    printf("\n=== Starting Continuous IMU Reading ===\n");
    printf("Reading IMU sensor every 4 seconds...\n");

    int reading_count = 0;

    while (1)
    {
        reading_count++;
        printf("\n--- IMU Reading #%d ---\n", reading_count);

        // Read using name-based API
        printf("Reading by name:\n");
        int channel_count_by_name = ocre_sensors_get_channel_count_by_name("imu");
        for (int j = 0; j < channel_count_by_name; j++)
        {
            int channel_type = ocre_sensors_get_channel_type_by_name("imu", j);
            if (channel_type >= 0)
            {
                double value = ocre_sensors_read_by_name("imu", channel_type);
                printf("  Channel %d (type %d): %f\n", j, channel_type, value);
            }
        }

        // Read using handle-based API
        printf("Reading by handle:\n");
        int channel_count_by_id = ocre_sensors_get_channel_count(imu_sensor_id);
        for (int j = 0; j < channel_count_by_id; j++)
        {
            int channel_type = ocre_sensors_get_channel_type(imu_sensor_id, j);
            if (channel_type >= 0)
            {
                double value = ocre_sensors_read(imu_sensor_id, channel_type);
                printf("  Channel %d (type %d): %f\n", j, channel_type, value);
            }
        }

        printf("Waiting 4 seconds before next reading...\n");
        ocre_sleep(4000); // Wait 4 seconds
    }

    printf("IMU Sensor Reader exiting.\n");
    return 0;
}
