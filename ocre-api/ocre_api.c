/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 */
#include "ocre_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Callback storage
static void (*timer_callbacks[OCRE_MAX_CALLBACKS])(void) = {0};
static void (*gpio_callbacks[OCRE_MAX_CALLBACKS])(void) = {0};
static message_callback_func_t message_callbacks[OCRE_MAX_CALLBACKS] = {0};
static char message_callback_topics[OCRE_MAX_CALLBACKS][OCRE_MAX_TOPIC_LEN] = {{0}};
static int gpio_callback_pins[OCRE_MAX_CALLBACKS] = {-1};
static int gpio_callback_ports[OCRE_MAX_CALLBACKS] = {-1};

// Initialize callback arrays
static void init_callback_system(void)
{
    static bool initialized = false;
    if (!initialized)
    {
        for (int i = 0; i < OCRE_MAX_CALLBACKS; i++)
        {
            timer_callbacks[i] = NULL;
            gpio_callbacks[i] = NULL;
            message_callbacks[i] = NULL;
            message_callback_topics[i][0] = '\0';
            gpio_callback_pins[i] = -1;
            gpio_callback_ports[i] = -1;
        }
        initialized = true;
    }
}

// =============================================================================
// INTERNAL CALLBACK DISPATCHERS
// =============================================================================

void OCRE_EXPORT("timer_callback") timer_callback(int timer_id)
{
    init_callback_system();
    if (timer_id >= 0 && timer_id < OCRE_MAX_CALLBACKS && timer_callbacks[timer_id])
    {
#ifdef OCRE_SDK_LOG
        printf("Executing timer callback for ID: %d\n", timer_id);
#endif
        timer_callbacks[timer_id]();
    }
    else
    {
#ifdef OCRE_SDK_LOG
        printf("No timer callback registered for ID: %d\n", timer_id);
#endif
    }
}

void OCRE_EXPORT("gpio_callback") gpio_callback(int pin, int state, int port)
{
    init_callback_system();
#ifdef OCRE_SDK_LOG
    printf("GPIO event triggered: pin=%d, port=%d, state=%d\n", pin, port, state);
#endif
    for (int i = 0; i < OCRE_MAX_CALLBACKS; i++)
    {
        if (gpio_callback_pins[i] == pin && gpio_callback_ports[i] == port && gpio_callbacks[i])
        {
#ifdef OCRE_SDK_LOG
            printf("Executing GPIO callback for pin: %d, port: %d\n", pin, port);
#endif
            gpio_callbacks[i]();
            return;
        }
    }
#ifdef OCRE_SDK_LOG
    printf("No GPIO callback registered for pin: %d, port: %d\n", pin, port);
#endif
}

void OCRE_EXPORT("message_callback") message_callback(uint32_t message_id, char *topic_ptr, char *content_type_ptr, uint8_t *payload_ptr, uint32_t payload_len)
{
    init_callback_system();
#ifdef OCRE_SDK_LOG
    printf("Message ID: %d\n", message_id);
    printf("Topic: %s\n", topic_ptr);
    printf("Content-Type: %s\n", content_type_ptr);
    printf("Payload: %s\n", payload_ptr);
    printf("Payload len: %d\n", payload_len);

    printf("Message event triggered: topic=%s, content_type=%s, payload_len=%d\n", topic_ptr, content_type_ptr, payload_len);
#endif
    for (int i = 0; i < OCRE_MAX_CALLBACKS; i++)
    {
        if (message_callbacks[i] && strncmp(message_callback_topics[i], topic_ptr, strlen(message_callback_topics[i])) == 0)
        {
#ifdef OCRE_SDK_LOG
            printf("Executing message callback for topic: %s\n", topic_ptr);
#endif
            message_callbacks[i](topic_ptr, content_type_ptr, payload_ptr, payload_len);
            return;
        }
    }
#ifdef OCRE_SDK_LOG
    printf("No message callback registered for topic: %s\n", topic_ptr);
#endif
}

void ocre_process_events(void)
{
    int event_count = 0;
    const int max_events_per_loop = 5;

    char topic_copy[OCRE_MAX_TOPIC_LEN];
    char content_type_copy[OCRE_MAX_CONTENT_TYPE_LEN];
    uint8_t payload_copy[OCRE_MAX_PAYLOAD_LEN];

    event_data_t event_data;
    while (event_count < max_events_per_loop)
    {
        int ret = ocre_get_event(
            (uint32_t)&event_data.type,
            (uint32_t)&event_data.id,
            (uint32_t)&event_data.port,
            (uint32_t)&event_data.state,
            (uint32_t)&event_data.extra,
            (uint32_t)&event_data.payload_len);
        ocre_sleep(10);
        if (ret != OCRE_SUCCESS)
        {
                break;
        }
#ifdef OCRE_SDK_LOG
        printf("Ocre process event retrieved: type=%u, id=%d, port(topic)=%u, state(content)=%u, extra(payload)=%u payload_len=%d\n", event_data.type, event_data.id, event_data.port, event_data.state, event_data.extra, event_data.payload_len);
#endif
        switch (event_data.type)
        {
        case OCRE_RESOURCE_TYPE_TIMER:
            timer_callback(event_data.id);
            break;
        case OCRE_RESOURCE_TYPE_GPIO:
            gpio_callback(event_data.id, event_data.state, event_data.port);
            break;
        case OCRE_RESOURCE_TYPE_MESSAGE:
            // Copy topic
            strncpy(topic_copy, (const char *)event_data.port, OCRE_MAX_TOPIC_LEN - 1);
            topic_copy[OCRE_MAX_TOPIC_LEN - 1] = '\0';

            // Copy content_type
            strncpy(content_type_copy, (const char *)event_data.state, OCRE_MAX_CONTENT_TYPE_LEN - 1);
            content_type_copy[OCRE_MAX_CONTENT_TYPE_LEN - 1] = '\0';

            // Copy payload
            uint32_t len = event_data.payload_len > OCRE_MAX_PAYLOAD_LEN ? OCRE_MAX_PAYLOAD_LEN : event_data.payload_len;
            memcpy(payload_copy, (const uint8_t *)event_data.extra, len);

            if (ocre_messaging_free_module_event_data(event_data.port, event_data.state, event_data.extra) != OCRE_SUCCESS)
            {
#ifdef OCRE_SDK_LOG
                printf("Error: Module event data wasn't freed successfully");
#endif
            }

            message_callback(event_data.id, topic_copy, content_type_copy, payload_copy, len);
            break;
        default:
#ifdef OCRE_SDK_LOG
            printf("Unknown event: type=%d, id=%d, port=%d, state=%d\n",
                   event_data.type, event_data.id, event_data.port, event_data.state);
#endif
        }
        event_count++;
    }

    if (event_count == 0)
    {
        ocre_sleep(10);
    }
}

// =============================================================================
// PUBLIC API FUNCTIONS
// =============================================================================

int ocre_register_timer_callback(int timer_id, timer_callback_func_t callback)
{
    init_callback_system();
    if (timer_id < 0 || timer_id >= OCRE_MAX_CALLBACKS)
    {
#ifdef OCRE_SDK_LOG
        printf("Error: Timer ID %d out of range (0-%d)\n", timer_id, OCRE_MAX_CALLBACKS - 1);
#endif
        return OCRE_ERROR_INVALID;
    }
    if (callback == NULL)
    {
#ifdef OCRE_SDK_LOG
        printf("Error: Timer callback is NULL for ID %d\n", timer_id);
#endif
        return OCRE_ERROR_INVALID;
    }
    if (ocre_register_dispatcher(OCRE_RESOURCE_TYPE_TIMER, "timer_callback") != OCRE_SUCCESS)
    {
#ifdef OCRE_SDK_LOG
        printf("Failed to register timer dispatcher\n");
#endif
        return OCRE_ERROR_INVALID;
    }
    timer_callbacks[timer_id] = callback;
#ifdef OCRE_SDK_LOG
    printf("Timer callback registered for ID: %d\n", timer_id);
#endif
    return OCRE_SUCCESS;
}

int ocre_register_gpio_callback(int pin, int port, gpio_callback_func_t callback)
{
    init_callback_system();
    if (callback == NULL)
    {
#ifdef OCRE_SDK_LOG
        printf("Error: GPIO callback is NULL for pin %d, port %d\n", pin, port);
#endif
        return OCRE_ERROR_INVALID;
    }
    if (pin < 0 || pin >= CONFIG_OCRE_GPIO_PINS_PER_PORT || port < 0 || port >= CONFIG_OCRE_GPIO_MAX_PORTS)
    {
#ifdef OCRE_SDK_LOG
        printf("Error: Invalid pin %d or port %d\n", pin, port);
#endif
        return OCRE_ERROR_INVALID;
    }
    int slot = -1;
    for (int i = 0; i < OCRE_MAX_CALLBACKS; i++)
    {
        if (gpio_callback_pins[i] == pin && gpio_callback_ports[i] == port)
        {
            slot = i;
            break;
        }
        if (slot == -1 && gpio_callback_pins[i] == -1)
        {
            slot = i;
        }
    }
    if (slot == -1)
    {
#ifdef OCRE_SDK_LOG
        printf("Error: No available slots for GPIO callbacks\n");
#endif
        return OCRE_ERROR_NO_MEMORY;
    }
    if (ocre_register_dispatcher(OCRE_RESOURCE_TYPE_GPIO, "gpio_callback") != OCRE_SUCCESS)
    {
#ifdef OCRE_SDK_LOG
        printf("Failed to register GPIO dispatcher\n");
#endif
        return OCRE_ERROR_INVALID;
    }
    gpio_callback_pins[slot] = pin;
    gpio_callback_ports[slot] = port;
    gpio_callbacks[slot] = callback;
#ifdef OCRE_SDK_LOG
    printf("GPIO callback registered for pin: %d, port: %d (slot %d)\n", pin, port, slot);
#endif
    return ocre_gpio_register_callback(port, pin);
}

int ocre_register_message_callback(const char *topic, message_callback_func_t callback)
{
    init_callback_system();
    if (!topic || topic[0] == '\0')
    {
#ifdef OCRE_SDK_LOG
        printf("Error: Topic is NULL or empty\n");
#endif
        return OCRE_ERROR_INVALID;
    }
    if (callback == NULL)
    {
#ifdef OCRE_SDK_LOG
        printf("Error: Message callback is NULL for topic %s\n", topic);
#endif
        return OCRE_ERROR_INVALID;
    }
    int slot = -1;
    for (int i = 0; i < OCRE_MAX_CALLBACKS; i++)
    {
        if (message_callbacks[i] && strcmp(message_callback_topics[i], topic) == 0)
        {
            slot = i;
            break;
        }
        if (slot == -1 && message_callback_topics[i][0] == '\0')
        {
            slot = i;
        }
    }
    if (slot == -1)
    {
#ifdef OCRE_SDK_LOG
        printf("Error: No available slots for message callbacks\n");
#endif
        return OCRE_ERROR_NO_MEMORY;
    }
    if (ocre_register_dispatcher(OCRE_RESOURCE_TYPE_MESSAGE, "message_callback") != OCRE_SUCCESS)
    {
#ifdef OCRE_SDK_LOG
        printf("Failed to register message dispatcher\n");
#endif
        return OCRE_ERROR_INVALID;
    }
    strncpy(message_callback_topics[slot], topic, OCRE_MAX_TOPIC_LEN - 1);
    message_callback_topics[slot][OCRE_MAX_TOPIC_LEN - 1] = '\0';
    message_callbacks[slot] = callback;
#ifdef OCRE_SDK_LOG
    printf("Message callback registered for topic: %s (slot %d)\n", topic, slot);
#endif

    return OCRE_SUCCESS;
}
int ocre_unregister_timer_callback(int timer_id)
{
    init_callback_system();
    if (timer_id < 0 || timer_id >= OCRE_MAX_CALLBACKS)
    {
#ifdef OCRE_SDK_LOG
        printf("Error: Timer ID %d out of range (0-%d)\n", timer_id, OCRE_MAX_CALLBACKS - 1);
#endif
        return OCRE_ERROR_INVALID;
    }
    if (timer_callbacks[timer_id] == NULL)
    {
#ifdef OCRE_SDK_LOG
        printf("Error: No timer callback registered for ID %d\n", timer_id);
#endif
        return OCRE_ERROR_NOT_FOUND;
    }
    timer_callbacks[timer_id] = NULL;
#ifdef OCRE_SDK_LOG
    printf("Timer callback unregistered for ID: %d\n", timer_id);
#endif
    return OCRE_SUCCESS;
}

int ocre_unregister_gpio_callback(int pin, int port)
{
    init_callback_system();
    int slot = -1;
    for (int i = 0; i < OCRE_MAX_CALLBACKS; i++)
    {
        if (gpio_callback_pins[i] == pin && gpio_callback_ports[i] == port)
        {
            slot = i;
            break;
        }
    }
    if (slot == -1 || gpio_callbacks[slot] == NULL)
    {
#ifdef OCRE_SDK_LOG
        printf("Error: No GPIO callback registered for pin %d, port %d\n", pin, port);
#endif
        return OCRE_ERROR_NOT_FOUND;
    }
    gpio_callback_pins[slot] = -1;
    gpio_callback_ports[slot] = -1;
    gpio_callbacks[slot] = NULL;
#ifdef OCRE_SDK_LOG
    printf("GPIO callback unregistered for pin: %d, port: %d\n", pin, port);
#endif
    return ocre_gpio_unregister_callback(port, pin);
}

int ocre_unregister_message_callback(const char *topic)
{
    init_callback_system();
    if (!topic || topic[0] == '\0')
    {
#ifdef OCRE_SDK_LOG
        printf("Error: Topic is NULL or empty\n");
#endif
        return OCRE_ERROR_INVALID;
    }
    int slot = -1;
    for (int i = 0; i < OCRE_MAX_CALLBACKS; i++)
    {
        if (message_callbacks[i] && strcmp(message_callback_topics[i], topic) == 0)
        {
            slot = i;
            break;
        }
    }
    if (slot == -1 || message_callbacks[slot] == NULL)
    {
#ifdef OCRE_SDK_LOG
        printf("Error: No message callback registered for topic %s\n", topic);
#endif
        return OCRE_ERROR_NOT_FOUND;
    }
    message_callback_topics[slot][0] = '\0';
    message_callbacks[slot] = NULL;
#ifdef OCRE_SDK_LOG
    printf("Message callback unregistered for topic: %s\n", topic);
#endif
    return OCRE_SUCCESS;
}
