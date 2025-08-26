/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 * Button Controlled LED Blinky Example
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ocre_api.h>

#define LED0_PORT 7
#define LED0 7
#define BUTTON_PORT 2
#define BUTTON_PIN 13

static bool blinky_active = false;
static bool led_state = false;
static bool button_pressed = false;

// GPIO callback function for button press
static void button_callback_function(void)
{
    printf("button_callback_function");
    // Read button state
    ocre_gpio_pin_state_t button_state = ocre_gpio_pin_get(BUTTON_PORT, BUTTON_PIN);

    // Detect button press (assuming active low button)
    if (button_state == OCRE_GPIO_PIN_RESET && !button_pressed)
    {
        button_pressed = true;

        if (!blinky_active)
        {
            printf("Button pressed - starting blinky!\n");
            blinky_active = true;

            // Blink 3 times quickly to show it works
            for (int i = 0; i < 3; i++)
            {
                ocre_gpio_pin_set(LED0_PORT, LED0, OCRE_GPIO_PIN_RESET); // ON
                printf("Init blink %d - LED ON\n", i + 1);
                ocre_sleep(200);
                ocre_gpio_pin_set(LED0_PORT, LED0, OCRE_GPIO_PIN_SET); // OFF
                printf("Init blink %d - LED OFF\n", i + 1);
                ocre_sleep(200);
            }
            blinky_active = false;
        }
        else
        {
            printf("Button pressed - stopping blinky!\n");
            blinky_active = false;
            // Turn off LED
            ocre_gpio_pin_set(LED0_PORT, LED0, OCRE_GPIO_PIN_SET); // OFF
            led_state = false;
            printf("LED turned OFF - blinky stopped\n");
        }
    }
    else if (button_state == OCRE_GPIO_PIN_SET)
    {
        button_pressed = false; // Button released
    }
}

int main(void)
{
    printf("=== Button Controlled LED Blinky Example ===\n");
    printf("Press button to start blinky!\n");

    // Initialize GPIO
    if (ocre_gpio_init() != 0)
    {
        printf("GPIO init failed\n");
        return -1;
    }

    // Configure LED as output
    if (ocre_gpio_configure(LED0_PORT, LED0, OCRE_GPIO_DIR_OUTPUT) != 0)
    {
        printf("LED config failed\n");
        return -1;
    }

    // Configure button as input
    if (ocre_gpio_configure(BUTTON_PORT, BUTTON_PIN, OCRE_GPIO_DIR_INPUT) != 0)
    {
        printf("Button config failed\n");
        return -1;
    }

    // Register callbacks
    if (ocre_gpio_register_callback(BUTTON_PORT, BUTTON_PIN) != 0)
    {
        printf("Failed to register button callback\n");
        return -1;
    }

    if (ocre_register_gpio_callback(BUTTON_PIN, BUTTON_PORT, button_callback_function) != 0)
    {
        printf("Failed to register GPIO callback function\n");
        return -1;
    }

    printf("System ready. Press button on Port %d, Pin %d to start/stop blinking.\n",
           BUTTON_PORT, BUTTON_PIN);

    while (1)
    {
        ocre_process_events();
        ocre_sleep(10);
    }

    printf("Button Controlled Blinky exiting.\n");
    return 0;
}
