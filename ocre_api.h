/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_SDK_H
#define OCRE_SDK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

// For exported callback functions (optional - only needed for WASM callbacks)
#define OCRE_EXPORT(name) __attribute__((export_name(name)))

// OCRE SDK Version Information
#define OCRE_SDK_VERSION_MAJOR  1
#define OCRE_SDK_VERSION_MINOR  0
#define OCRE_SDK_VERSION_PATCH  0
#define OCRE_SDK_VERSION        "1.0.0"

// Common Return Codes
#define OCRE_SUCCESS            0
#define OCRE_ERROR_INVALID      -1
#define OCRE_ERROR_TIMEOUT      -2
#define OCRE_ERROR_NOT_FOUN     -3
#define OCRE_ERROR_BUSY         -4
#define OCRE_ERROR_NO_MEMORY    -5

// Configuration
#define OCRE_MAX_TIMERS         16
#define OCRE_MAX_SENSORS        32
#define OCRE_MAX_CALLBACKS      64
#define OCRE_MAX_TOPIC_LEN      128
#define OCRE_MAX_PAYLOAD_LEN    1024
#define CONFIG_MAX_SENSOR_NAME_LENGTH 125

// GPIO Configuration
#ifndef CONFIG_OCRE_GPIO_MAX_PINS
#define CONFIG_OCRE_GPIO_MAX_PINS 32
#endif

#ifndef CONFIG_OCRE_GPIO_MAX_PORTS
#define CONFIG_OCRE_GPIO_MAX_PORTS 8
#endif

#ifndef CONFIG_OCRE_GPIO_PINS_PER_PORT
#define CONFIG_OCRE_GPIO_PINS_PER_PORT 16
#endif

// Internal state tracking
typedef struct {
    bool initialized;
    uint32_t active_timers;
    uint32_t active_sensors;
} ocre_sdk_state_t;

// =============================================================================
// Resource Types
// =============================================================================

/**
 * Enum representing different resource types
 */
typedef enum {
    OCRE_RESOURCE_TYPE_TIMER,
    OCRE_RESOURCE_TYPE_GPIO,
    OCRE_RESOURCE_TYPE_SENSOR,
    OCRE_RESOURCE_TYPE_COUNT
} ocre_resource_type_t;

// =============================================================================
// GPIO API
// =============================================================================

/**
 * GPIO pin state
 */
typedef enum {
    OCRE_GPIO_PIN_RESET = 0,
    OCRE_GPIO_PIN_SET = 1
} ocre_gpio_pin_state_t;

/**
 * GPIO pin direction
 */
typedef enum {
    OCRE_GPIO_DIR_INPUT = 0,
    OCRE_GPIO_DIR_OUTPUT = 1
} ocre_gpio_direction_t;

/**
 * GPIO configuration structure
 */
typedef struct {
    int pin;                         /**< GPIO pin number (logical) */
    ocre_gpio_direction_t direction; /**< Pin direction */
} ocre_gpio_config_t;

/**
 * GPIO callback function type
 */
typedef void (*ocre_gpio_callback_t)(int pin, ocre_gpio_pin_state_t state);

/**
 * Initialize GPIO subsystem
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_init(void);

/**
 * Configure a GPIO pin
 * @param port GPIO port number
 * @param pin GPIO pin number
 * @param direction Pin direction (input/output)
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_configure(int port, int pin, int direction);

/**
 * Set GPIO pin state
 * @param port GPIO port number
 * @param pin GPIO pin number
 * @param state Desired pin state
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_pin_set(int port, int pin, int state);

/**
 * Get GPIO pin state
 * @param port GPIO port number
 * @param pin GPIO pin number
 * @return Pin state or negative error code
 */
int ocre_gpio_pin_get(int port, int pin);

/**
 * Toggle GPIO pin state
 * @param port GPIO port number
 * @param pin GPIO pin number
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_pin_toggle(int port, int pin);

/**
 * Register callback for GPIO pin state changes
 * @param port GPIO port number
 * @param pin GPIO pin number
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_register_callback(int port, int pin);

/**
 * Unregister GPIO pin callback
 * @param port GPIO port number
 * @param pin GPIO pin number
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_unregister_callback(int port, int pin);

// =============================================================================
// Timer API
// =============================================================================

/**
 * Timer identifier type
 */
typedef int ocre_timer_t;

/**
 * Initialize timer subsystem
 */
void ocre_timer_init(void);

/**
 * Create a timer with specified ID
 * @param id Timer identifier (must be between 1 and MAX_TIMERS)
 * @return 0 on success, -1 on error
 */
int ocre_timer_create(int id);

/**
 * Delete a timer
 * @param id Timer identifier
 * @return 0 on success, -1 on error
 */
int ocre_timer_delete(ocre_timer_t id);

/**
 * Start a timer
 * @param id Timer identifier
 * @param interval Timer interval in milliseconds
 * @param is_periodic True for periodic timer, false for one-shot
 * @return 0 on success, -1 on error
 */
int ocre_timer_start(ocre_timer_t id, int interval, int is_periodic);

/**
 * Stop a timer
 * @param id Timer identifier
 * @return 0 on success, -1 on error
 */
int ocre_timer_stop(ocre_timer_t id);

/**
 * Get remaining time for a timer
 * @param id Timer identifier
 * @return Remaining time in milliseconds, or -1 on error
 */
int ocre_timer_get_remaining(ocre_timer_t id);

// =============================================================================
// Sleep API
// =============================================================================

/**
 * Sleep for specified duration
 * @param milliseconds Sleep duration in milliseconds
 * @return OCRE_SUCCESS on success, error code on failure
 */
int ocre_sleep(int milliseconds);

/**
 * Pause execution indefinitely (implementation-specific)
 * @return OCRE_SUCCESS on success, error code on failure
 */
#define ocre_pause() ocre_sleep(9999999)

// =============================================================================
// Sensor API
// =============================================================================

typedef int32_t ocre_sensor_handle_t;

/**
 * Enum representing different sensor channels
 */
typedef enum {
    SENSOR_CHANNEL_ACCELERATION,
    SENSOR_CHANNEL_GYRO,
    SENSOR_CHANNEL_MAGNETIC_FIELD,
    SENSOR_CHANNEL_LIGHT,
    SENSOR_CHANNEL_PRESSURE,
    SENSOR_CHANNEL_PROXIMITY,
    SENSOR_CHANNEL_HUMIDITY,
    SENSOR_CHANNEL_TEMPERATURE,
    // Add more channels as needed
} sensor_channel_t;

/**
 * Structure representing a sensor instance
 */
typedef struct ocre_sensor_t {
    ocre_sensor_handle_t handle;
    char *sensor_name;
    int num_channels;
    sensor_channel_t channels[];
} ocre_sensor_t;

/**
 * Initialize the sensor system
 * @return 0 on success
 */
int ocre_sensors_init(void);

/**
 * Discover available sensors
 * @return Number of discovered sensors, negative error code on failure
 */
int ocre_sensors_discover(void);

/**
 * Open a sensor for use
 * @param handle Handle of the sensor to open
 * @return 0 on success, negative error code on failure
 */
int ocre_sensors_open(ocre_sensor_handle_t handle);

/**
 * Get the handle of a sensor
 * @param sensor_id ID of the sensor
 * @return Sensor handle on success, negative error code on failure
 */
int ocre_sensors_get_handle(int sensor_id);

/**
 * Get the handle of a sensor by name
 * @param name Name of the sensor
 * @param handle Pointer to store the sensor handle
 * @return OCRE_SUCCESS on success, negative error code on failure
 */
int ocre_sensors_get_handle_by_name(const char *name, ocre_sensor_handle_t *handle);

/**
 * Open a sensor by name
 * @param name Name of the sensor
 * @param handle Pointer to store the sensor handle
 * @return OCRE_SUCCESS on success, negative error code on failure
 */
int ocre_sensors_open_by_name(const char *name, ocre_sensor_handle_t *handle);

/**
 * Get the number of channels available in a sensor
 * @param sensor_id ID of the sensor
 * @return Number of channels on success, negative error code on failure
 */
int ocre_sensors_get_channel_count(int sensor_id);

/**
 * Get the type of a specific sensor channel
 * @param sensor_id ID of the sensor
 * @param channel_index Index of the channel
 * @return Channel type on success, negative error code on failure
 */
int ocre_sensors_get_channel_type(int sensor_id, int channel_index);

/**
 * Read data from a sensor channel
 * @param sensor_id ID of the sensor
 * @param channel_type Type of the channel to read
 * @return Sensor value in integer format, negative error code on failure
 */
int ocre_sensors_read(int sensor_id, int channel_type);

// =============================================================================
// RNG Sensor API
// =============================================================================

#define SENSOR_CHAN_CUSTOM 1

/**
 * Initialize RNG sensor
 * @return 0 on success, negative error code on failure
 */
int rng_sensor_init(void);

// =============================================================================
// Messages API
// =============================================================================

/**
 * Structure of ocre messages
 */
typedef struct ocre_msg {
    uint64_t mid;       /**< message id - increments on each message */
    char *topic;        /**< url of the request */
    char *content_type; /**< payload format (MIME type) */
    void *payload;      /**< payload of the request */
    int payload_len;    /**< length in bytes of the payload */
} ocre_msg_t;

/**
 * Initialize OCRE Messaging System
 */
void ocre_msg_system_init(void);

/**
 * Publish a message to the specified target
 * @param topic the name of the topic on which to publish the message
 * @param content_type the content type of the message; it is recommended to use a MIME type
 * @param payload a buffer containing the message contents
 * @param payload_len the length of the payload buffer
 * @return 0 on success, negative error code on failure
 */
int ocre_publish_message(char *topic, char *content_type, void *payload, int payload_len);

/**
 * Subscribe to messages on the specified topic
 * @param topic the name of the topic on which to subscribe
 * @param handler_name name of callback function that will be called when a message is received on this topic
 * @return 0 on success, negative error code on failure
 */
int ocre_subscribe_message(char *topic, char *handler_name);

// =============================================================================
// Event API
// =============================================================================

/**
 * Structure for event data
 */
typedef struct {
    int32_t type;  /**< Resource type (e.g., OCRE_RESOURCE_TYPE_*) */
    int32_t id;    /**< Resource ID */
    int32_t port;  /**< Port number (for GPIO) */
    int32_t state; /**< State (e.g., pin state for GPIO) */
} event_data_t;

/**
 * Get event data for a specific resource
 * @param type_offset Offset for resource type
 * @param id_offset Offset for resource ID
 * @param port_offset Offset for port number
 * @param state_offset Offset for state
 * @return OCRE_SUCCESS on success, negative error code on failure
 */
int ocre_get_event(uint32_t type_offset, uint32_t id_offset, uint32_t port_offset, uint32_t state_offset);

/**
 * Register a dispatcher for a resource type
 * @param type Resource type to register the dispatcher for
 * @param function_name Name of the callback function
 * @return OCRE_SUCCESS on success, negative error code on failure
 */
int ocre_register_dispatcher(ocre_resource_type_t type, const char *function_name);

// =============================================================================
// SDK Utility Functions
// =============================================================================

/**
 * Initialize the entire OCRE SDK
 * @return OCRE_SUCCESS on success, error code on failure
 */
int ocre_sdk_init(void);

/**
 * Cleanup and shutdown the OCRE SDK
 * @return OCRE_SUCCESS on success, error code on failure
 */
int ocre_sdk_cleanup(void);

/**
 * Get SDK version string
 * @return Version string
 */
const char *ocre_sdk_get_version(void);

#ifdef __cplusplus
}
#endif

#endif // OCRE_SDK_H
