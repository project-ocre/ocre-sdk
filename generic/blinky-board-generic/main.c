/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 * Physical LED Blinky Example
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ocre_api.h>

// Timer callback function
static void my_timer_function(void)
{
    printf("Timer triggered - blinking LED!\n");
    static bool led_state = false;
    static int blink_count = 0;

    // Active-low: RESET (low) = ON, SET (high) = OFF
    int ret = led_state ? ocre_gpio_set_by_name("led0", OCRE_GPIO_PIN_RESET)
                        : ocre_gpio_set_by_name("led0", OCRE_GPIO_PIN_SET); 

    if (ret != 0)
    {
        printf("Failed to set LED: %d\n", ret);
    }
    else
    {
        printf("LED state set to %s (logical %d, count %d)\n",
               led_state ? "ON" : "OFF", led_state, ++blink_count);
    }
    led_state = !led_state;
}

int main(void)
{
    const int timer_id = 1;
    int interval_ms = 1000; // 1 second blink
    bool is_periodic = true;

    printf("=== Physical LED Blinky Example ===\n");

    // Initialize GPIO
    if (ocre_gpio_init() != 0)
    {
        printf("GPIO init failed\n");
        return -1;
    }

    // Configure LED 
    // "led0" - Device tree configuration must be available 
    // Or the application will not work 
    if (ocre_gpio_configure_by_name("led0", OCRE_GPIO_DIR_OUTPUT) != 0)
    {
        printf("LED config failed\n");
        return -1;
    }

    // Register timer callback
    if (ocre_register_timer_callback(timer_id, my_timer_function) != 0)
    {
        printf("Failed to register timer callback function\n");
        return -1;
    }

    // Create and start timer
    if (ocre_timer_create(timer_id) != 0)
    {
        printf("Timer creation failed\n");
        return -1;
    }
    printf("Timer created. ID: %d, Interval: %dms\n", timer_id, interval_ms);

    if (ocre_timer_start(timer_id, interval_ms, is_periodic) != 0)
    {
        printf("Timer start failed\n");
        return -1;
    }

    printf("LED blinking started. Press Ctrl+C to stop.\n");

    while (1)
    {
        ocre_process_events();
        ocre_sleep(10);
    }

    printf("Physical LED Blinky exiting.\n");
    return 0;
}
