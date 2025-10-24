/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 * Generic Blinky Example - Printf Only
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ocre_api.h>

// Timer callback function for generic blinking
static void generic_blink_function(void)
{
    static int blink_count = 0;
    static bool blink_state = false;

    printf("blink (count: %d, state: %s)\n",
           ++blink_count, blink_state ? "+" : "-");

    blink_state = !blink_state;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0); 
    const int timer_id = 1;
    int interval_ms = 1000; // 1 second blink
    bool is_periodic = true;

    printf("=== Generic Blinky Example (Printf Only) ===\n");
    printf("This example demonstrates software blinking without physical hardware.\n");

    // Register timer callback
    if (ocre_register_timer_callback(timer_id, generic_blink_function) != 0)
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

    printf("Generic blinking started. You should see 'blink' messages every %dms.\n", interval_ms);
    printf("Press Ctrl+C to stop.\n");

    while (1)
    {
        ocre_process_events();
        ocre_sleep(10);
    }

    printf("Generic Blinky exiting.\n");
    return 0;
}
