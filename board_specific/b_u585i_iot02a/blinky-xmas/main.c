/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 */
#include <stdio.h>
#include <stdbool.h>
#include "../../../ocre-sdk/ocre_api.h"

// Define the timer ID and interval for the periodic timer
#define TIMER_ID        1
#define TIMER_INTERVAL  500

// Define the GPIO port and pins for both LEDs
#define LED_PORT        7
#define RED_LED_PIN     6
#define GREEN_LED_PIN   7

// Manages the LED state, and called by the timer callback function
void toggle_leds(void) {
    static bool red_active = true;
    
    if (red_active) {
        // Turn on red LED (active low), turn off green LED
        ocre_gpio_pin_set(LED_PORT, RED_LED_PIN, OCRE_GPIO_PIN_RESET);
        ocre_gpio_pin_set(LED_PORT, GREEN_LED_PIN, OCRE_GPIO_PIN_SET);
        printf("LED is: RED");  // No newline character
    } else {
        // Turn on green LED (active low), turn off red LED
        ocre_gpio_pin_set(LED_PORT, RED_LED_PIN, OCRE_GPIO_PIN_SET);
        ocre_gpio_pin_set(LED_PORT, GREEN_LED_PIN, OCRE_GPIO_PIN_RESET);
        printf("LED is: GREEN");  // No newline character
    }
    
    // Add carriage return to overwrite the same line
    printf("\r");
    fflush(stdout);
    
    // Toggle state for next time
    red_active = !red_active;
}

// Timer callback function for the new SDK
void timer_callback_handler(void) {
    toggle_leds();
}

int main(void) {
    int ret;
    
    // Initialize app
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("Blinky-xmas app initializing...\n");
    
    // Initialize the GPIO subsystem
    ret = ocre_gpio_init();
    if (ret != OCRE_SUCCESS) {
        printf("Failed to initialize GPIO: %d\n", ret);
        return ret;
    }
    
    // Configure the red LED pin
    ret = ocre_gpio_configure(LED_PORT, RED_LED_PIN, OCRE_GPIO_DIR_OUTPUT);
    if (ret != OCRE_SUCCESS) {
        printf("Failed to configure red LED GPIO: pin=%d, ret=%d\n", RED_LED_PIN, ret);
        return ret;
    }
    
    // Configure the green LED pin
    ret = ocre_gpio_configure(LED_PORT, GREEN_LED_PIN, OCRE_GPIO_DIR_OUTPUT);
    if (ret != OCRE_SUCCESS) {
        printf("Failed to configure green LED GPIO: pin=%d, ret=%d\n", GREEN_LED_PIN, ret);
        return ret;
    }
    
    // Set initial LED states - start with red LED on, green LED off
    ret = ocre_gpio_pin_set(LED_PORT, RED_LED_PIN, OCRE_GPIO_PIN_RESET);
    if (ret != OCRE_SUCCESS) {
        printf("Failed to set red LED initial state: pin=%d, ret=%d\n", RED_LED_PIN, ret);
        return ret;
    }
    
    ret = ocre_gpio_pin_set(LED_PORT, GREEN_LED_PIN, OCRE_GPIO_PIN_SET);
    if (ret != OCRE_SUCCESS) {
        printf("Failed to set green LED initial state: pin=%d, ret=%d\n", GREEN_LED_PIN, ret);
        return ret;
    }
    
    // Create the timer using the OCRE timer API
    ret = ocre_timer_create(TIMER_ID);
    if (ret != OCRE_SUCCESS) {
        printf("Failed to create timer: %d\n", ret);
        return ret;
    }
    
    // Register the timer callback with the new SDK
    ret = ocre_register_timer_callback(TIMER_ID, timer_callback_handler);
    if (ret != OCRE_SUCCESS) {
        printf("Failed to register timer callback: %d\n", ret);
        return ret;
    }
    
    // Start the periodic timer
    ret = ocre_timer_start(TIMER_ID, TIMER_INTERVAL, true);
    if (ret != OCRE_SUCCESS) {
        printf("Failed to start timer: %d\n", ret);
        return ret;
    }
    
    printf("Blinky-xmas app started successfully!\n");
    
    // Main event loop - process events continuously
    while (1) {
        ocre_process_events();
    }
    
    return 0;
}
