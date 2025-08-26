/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 */
#ifndef OCRE_SDK_H
#define OCRE_SDK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file ocre_sdk.h
 * @brief OCRE SDK header file for interacting with timers, GPIO, sensors, and messaging.
 */

// Forward declarations for WASM types
struct WASMModuleInstance;
typedef struct WASMModuleInstance *wasm_module_inst_t;

#ifdef __cplusplus
extern "C"
{
#endif

// #define OCRE_SDK_LOG 1

// For exported callback functions (optional - only needed for WASM callbacks)
#define OCRE_EXPORT(name) __attribute__((export_name(name)))

// OCRE SDK Version Information
#define OCRE_SDK_VERSION_MAJOR 1
#define OCRE_SDK_VERSION_MINOR 0
#define OCRE_SDK_VERSION_PATCH 0
#define OCRE_SDK_VERSION "1.0.0"

// Common Return Codes
#define OCRE_SUCCESS 0
#define OCRE_ERROR_INVALID -1
#define OCRE_ERROR_TIMEOUT -2
#define OCRE_ERROR_NOT_FOUND -3
#define OCRE_ERROR_BUSY -4
#define OCRE_ERROR_NO_MEMORY -5

// Configuration
#define OCRE_MAX_TIMERS 16
#define OCRE_MAX_SENSORS 32
#define OCRE_MAX_CALLBACKS 64
#define OCRE_MAX_TOPIC_LEN 128
#define OCRE_MAX_CONTENT_TYPE_LEN 128
#define OCRE_MAX_PAYLOAD_LEN 1024
#define CONFIG_MAX_SENSOR_NAME_LENGTH 125
#define OCRE_API_POSIX_BUF_SIZE 65

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

    /**
     * @brief Internal state tracking for the OCRE SDK
     */
    typedef struct
    {
        bool initialized;        /**< Indicates if the SDK is initialized */
        uint32_t active_timers;  /**< Number of active timers */
        uint32_t active_sensors; /**< Number of active sensors */
    } ocre_sdk_state_t;

    /**
     * @brief Structure for event data
     */
    typedef struct
    {
        uint32_t type;        /**< Resource type (e.g., OCRE_RESOURCE_TYPE_*) */
        uint32_t id;          /**< Resource ID */
        uint32_t port;        /**< Port number (for GPIO) */
        uint32_t state;       /**< State (e.g., pin state for GPIO) */
        uint32_t extra;       /**< Extra data for events */
        uint32_t payload_len; /**< Payload length (for message events) */
    } event_data_t;

    // =============================================================================
    // Resource Types
    // =============================================================================

    /**
     * @brief Enum representing different resource types
     */
    typedef enum
    {
        OCRE_RESOURCE_TYPE_TIMER,   /**< Timer resource */
        OCRE_RESOURCE_TYPE_GPIO,    /**< GPIO resource */
        OCRE_RESOURCE_TYPE_SENSOR,  /**< Sensor resource */
        OCRE_RESOURCE_TYPE_MESSAGE, /**< Message resource */
        OCRE_RESOURCE_TYPE_COUNT    /**< Number of resource types */
    } ocre_resource_type_t;

    // =============================================================================
    // Timer API
    // =============================================================================

    /**
     * @brief Create a timer with specified ID
     * @param id Timer identifier (must be between 1 and OCRE_MAX_TIMERS)
     * @return OCRE_SUCCESS on success, OCRE_ERROR_INVALID on error
     */
    int ocre_timer_create(int id);

    /**
     * @brief Delete a timer
     * @param id Timer identifier
     * @return OCRE_SUCCESS on success, OCRE_ERROR_INVALID on error
     */
    int ocre_timer_delete(int id);

    /**
     * @brief Start a timer
     * @param id Timer identifier
     * @param interval Timer interval in milliseconds
     * @param is_periodic True for periodic timer, false for one-shot
     * @return OCRE_SUCCESS on success, OCRE_ERROR_INVALID on error
     */
    int ocre_timer_start(int id, int interval, int is_periodic);

    /**
     * @brief Stop a timer
     * @param id Timer identifier
     * @return OCRE_SUCCESS on success, OCRE_ERROR_INVALID on error
     */
    int ocre_timer_stop(int id);

    /**
     * @brief Get remaining time for a timer
     * @param id Timer identifier
     * @return Remaining time in milliseconds, or OCRE_ERROR_INVALID on error
     */
    int ocre_timer_get_remaining(int id);

    // =============================================================================
    // GPIO API
    // =============================================================================

    /**
     * @brief GPIO pin direction
     */
    typedef enum
    {
        OCRE_GPIO_DIR_INPUT, /**< GPIO pin configured as input */
        OCRE_GPIO_DIR_OUTPUT /**< GPIO pin configured as output */
    } ocre_gpio_direction_t;

    /**
     * @brief GPIO pin state
     */
    typedef enum
    {
        OCRE_GPIO_PIN_RESET = 0, /**< GPIO pin low state */
        OCRE_GPIO_PIN_SET = 1    /**< GPIO pin high state */
    } ocre_gpio_pin_state_t;

    /**
     * @brief Initialize GPIO subsystem
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_gpio_init(void);

    /**
     * @brief Configure a GPIO pin
     * @param port GPIO port number
     * @param pin GPIO pin number
     * @param direction Pin direction (OCRE_GPIO_DIR_INPUT or OCRE_GPIO_DIR_OUTPUT)
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_gpio_configure(int port, int pin, int direction);

    /**
     * @brief Set GPIO pin state
     * @param port GPIO port number
     * @param pin GPIO pin number
     * @param state Desired pin state (OCRE_GPIO_PIN_RESET or OCRE_GPIO_PIN_SET)
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_gpio_pin_set(int port, int pin, ocre_gpio_pin_state_t state);

    /**
     * @brief Get GPIO pin state
     * @param port GPIO port number
     * @param pin GPIO pin number
     * @return Pin state (OCRE_GPIO_PIN_RESET or OCRE_GPIO_PIN_SET) or negative error code
     */
    ocre_gpio_pin_state_t ocre_gpio_pin_get(int port, int pin);

    /**
     * @brief Toggle GPIO pin state
     * @param port GPIO port number
     * @param pin GPIO pin number
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_gpio_pin_toggle(int port, int pin);

    /**
     * @brief Register callback for GPIO pin state changes
     * @param port GPIO port number
     * @param pin GPIO pin number
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_gpio_register_callback(int port, int pin);

    /**
     * @brief Unregister GPIO pin callback
     * @param port GPIO port number
     * @param pin GPIO pin number
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_gpio_unregister_callback(int port, int pin);

    /**
     * @brief Configure a GPIO pin by name
     * @param name GPIO pin name
     * @param direction Pin direction (OCRE_GPIO_DIR_INPUT or OCRE_GPIO_DIR_OUTPUT)
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_gpio_configure_by_name(const char *name, int direction);

    /**
     * @brief Set GPIO pin state by name
     * @param name GPIO pin name
     * @param state Desired pin state (OCRE_GPIO_PIN_RESET or OCRE_GPIO_PIN_SET)
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_gpio_set_by_name(const char *name, int state);

    /**
     * @brief Get GPIO pin state by name
     * @param name GPIO pin name
     * @return Pin state (OCRE_GPIO_PIN_RESET or OCRE_GPIO_PIN_SET) or negative error code
     */
    int ocre_gpio_get_by_name(const char *name);

    /**
     * @brief Toggle GPIO pin state by name
     * @param name GPIO pin name
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_gpio_toggle_by_name(const char *name);

    /**
     * @brief Register callback for GPIO pin state changes by name
     * @param name GPIO pin name
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_gpio_register_callback_by_name(const char *name);

    /**
     * @brief Unregister GPIO pin callback by name
     * @param name GPIO pin name
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_gpio_unregister_callback_by_name(const char *name);

    // =============================================================================
    // Event API
    // =============================================================================

    /**
     * @brief Timer callback function type
     */
    typedef void (*timer_callback_func_t)(void);

    /**
     * @brief GPIO callback function type
     */
    typedef void (*gpio_callback_func_t)(void);

    /**
     * @brief Message callback function type
     * @param topic The topic of the received message
     * @param content_type The content type of the message
     * @param payload The message payload
     * @param payload_len The length of the payload
     */
    typedef void (*message_callback_func_t)(const char *topic, const char *content_type, const void *payload, uint32_t payload_len);

    /**
     * @brief Get event data for a specific resource
     * @param type_offset Offset for resource type
     * @param id_offset Offset for resource ID
     * @param port_offset Offset for port number
     * @param state_offset Offset for state
     * @param extra_offset Offset for extra data
     * @param payload_len_offset Offset for payload length
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_get_event(uint32_t type_offset, uint32_t id_offset, uint32_t port_offset,
                       uint32_t state_offset, uint32_t extra_offset, uint32_t payload_len_offset);

    /**
     * @brief Process the events from runtime
     */
    void ocre_process_events(void);

    /**
     * @brief Register timer callback
     * @param timer_id Timer identifier
     * @param callback Callback function to register
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_register_timer_callback(int timer_id, timer_callback_func_t callback);

    /**
     * @brief Register GPIO callback
     * @param pin GPIO pin number
     * @param port GPIO port number
     * @param callback Callback function to register
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_register_gpio_callback(int pin, int port, gpio_callback_func_t callback);

    /**
     * @brief Register message callback
     * @param topic The topic to subscribe to
     * @param callback Callback function to register
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_register_message_callback(const char *topic, message_callback_func_t callback);

    /**
     * @brief Unregister timer callback
     * @param timer_id Timer identifier
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_unregister_timer_callback(int timer_id);

    /**
     * @brief Unregister GPIO callback
     * @param pin GPIO pin number
     * @param port GPIO port number
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_unregister_gpio_callback(int pin, int port);

    /**
     * @brief Unregister message callback
     * @param topic The topic to unsubscribe from
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_unregister_message_callback(const char *topic);

    // =============================================================================
    // Messages API
    // =============================================================================

    /**
     * @brief Structure for OCRE messages
     */
    typedef struct ocre_msg
    {
        uint32_t mid;         /**< Message ID - increments on each message */
        char *topic;          /**< URL of the request */
        char *content_type;   /**< Payload format (MIME type) */
        void *payload;        /**< Payload of the request */
        uint32_t payload_len; /**< Length in bytes of the payload */
    } ocre_msg_t;

    /**
     * @brief Initialize OCRE Messaging System
     */
    void ocre_msg_system_init(void);

    /**
     * @brief Publish a message to the specified target
     * @param topic The name of the topic on which to publish the message
     * @param content_type The content type of the message; it is recommended to use a MIME type
     * @param payload A buffer containing the message contents
     * @param payload_len The length of the payload buffer
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_publish_message(const char *topic, const char *content_type, const void *payload, uint32_t payload_len);

    /**
     * @brief Subscribe to messages on the specified topic
     * @param topic The name of the topic on which to subscribe
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_subscribe_message(const char *topic);

    /**
     * @brief Frees allocated memory for a messaging event in the WASM module.
     *
     * This function releases the allocated memory for the topic, content-type, and payload
     * associated with a messaging event received by the WASM module. It should be called
     * after processing the message in ocre_process_events() to prevent memory leaks.
     *
     * @param topic_offset    Offset in WASM memory for the message topic.
     * @param content_offset  Offset in WASM memory for the message content-type.
     * @param payload_offset  Offset in WASM memory for the message payload.
     *
     * @return OCRE_SUCCESS on success, negative error code on failure.
     */
    int ocre_messaging_free_module_event_data(uint32_t topic_offset, uint32_t content_offset, uint32_t payload_offset);

    // =============================================================================
    // Utility API
    // =============================================================================

    /**
     * @brief Sleep for specified duration
     * @param milliseconds Sleep duration in milliseconds
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_sleep(int milliseconds);

/**
 * @brief Pause execution indefinitely (implementation-specific)
 * @return OCRE_SUCCESS on success, negative error code on failure
 */
#define ocre_pause() ocre_sleep(9999999)

    // =============================================================================
    // Sensor API
    // =============================================================================

    /**
     * @brief Sensor handle type
     */
    typedef int ocre_sensor_handle_t;

    /**
     * @brief Initialize the sensor system
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_sensors_init(void);

    /**
     * @brief Discover available sensors
     * @return Number of discovered sensors, negative error code on failure
     */
    int ocre_sensors_discover(void);

    /**
     * @brief Open a sensor for use
     * @param handle Handle of the sensor to open
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_sensors_open(ocre_sensor_handle_t handle);

    /**
     * @brief Get the handle of a sensor
     * @param sensor_id ID of the sensor
     * @return Sensor handle on success, negative error code on failure
     */
    int ocre_sensors_get_handle(int sensor_id);

    /**
     * @brief Get the number of channels available in a sensor
     * @param sensor_id ID of the sensor
     * @return Number of channels on success, negative error code on failure
     */
    int ocre_sensors_get_channel_count(int sensor_id);

    /**
     * @brief Get the type of a specific sensor channel
     * @param sensor_id ID of the sensor
     * @param channel_index Index of the channel
     * @return Channel type on success, negative error code on failure
     */
    int ocre_sensors_get_channel_type(int sensor_id, int channel_index);

    /**
     * @brief Read data from a sensor channel
     * @param sensor_id ID of the sensor
     * @param channel_type Type of the channel to read
     * @return Sensor value as double, negative error code on failure
     */
    double ocre_sensors_read(int sensor_id, int channel_type);

    /**
     * @brief Get the handle of a sensor by name
     * @param sensor_name Name of the sensor
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_sensors_get_handle_by_name(const char *sensor_name);   //, ocre_sensor_handle_t handle);

    /**
     * @brief Open a sensor by name
     * @param sensor_name Name of the sensor
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_sensors_open_by_name(const char *sensor_name);

    /**
     * @brief Get the channel count of a sensor referenced by name
     * @param sensor_name Name of the sensor
     * @return Channel count on success, negative error code on failure
     */
    int ocre_sensors_get_channel_count_by_name(const char *sensor_name);

    /**
     * @brief Get the channel type of a specified channel of a sensor referenced by name
     * @param sensor_name Name of the sensor
     * @param channel_index Index of channel to query
     * @return Channel type on success, negative error code on failure
     */
    int ocre_sensors_get_channel_type_by_name(const char *sensor_name, int channel_index);

    /**
     * @brief Read data from a channel from a sensor referenced by name
     * @param sensor_name Name of the sensor
     * @param channel_type Type of the channel to read
     * @return Sensor value as double, negative error code on failure
     */
    double ocre_sensors_read_by_name(const char *sensor_name, int channel_type);

    /**
     * @brief Register a dispatcher for a resource type
     * @param type Resource type to register the dispatcher for
     * @param function_name Name of the callback function
     * @return OCRE_SUCCESS on success, negative error code on failure
     */
    int ocre_register_dispatcher(ocre_resource_type_t type, const char *function_name);

    // =============================================================================
    // POSIX API
    // =============================================================================

    /**
     * @brief Structure for system information
     */
    struct _ocre_posix_utsname
    {
        char sysname[OCRE_API_POSIX_BUF_SIZE];    /**< System name */
        char nodename[OCRE_API_POSIX_BUF_SIZE];   /**< Node name */
        char release[OCRE_API_POSIX_BUF_SIZE];    /**< Release version */
        char version[OCRE_API_POSIX_BUF_SIZE];    /**< Version string */
        char machine[OCRE_API_POSIX_BUF_SIZE];    /**< Machine type */
        char domainname[OCRE_API_POSIX_BUF_SIZE]; /**< Domain name */
    };

    /**
     * @brief Get system information
     * @param name Buffer to receive system information
     * @return OCRE_SUCCESS on success, OCRE_ERROR_INVALID on failure
     */
    int uname(struct _ocre_posix_utsname *name);

#ifdef __cplusplus
}
#endif
#endif
