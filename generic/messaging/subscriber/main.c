/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 */
#include <ocre_api.h>
#include <stdio.h>
#include <string.h>

#define TOPIC "test/"

void message_handler(const char *topic, const char *content_type, const void *payload, uint32_t payload_len);

// WASM entry point
int main(void)
{
  setvbuf(stdout, NULL, _IONBF, 0); 
  int ret = ocre_register_message_callback(TOPIC, message_handler);
  if (ret != OCRE_SUCCESS)
  {
    printf("Error: Failed to register message callback for %s\n", TOPIC);
  }

  ret = ocre_subscribe_message(TOPIC);
  if (ret != OCRE_SUCCESS)
  {
    printf("Error: Failed to subscribe to topic %s\n", TOPIC);
    ocre_unregister_message_callback(TOPIC);
  }

  printf("Subscriber initialized: listening on %s\n", TOPIC);
  while (1)
  {
    ocre_process_events();
  }
  return 0;
}

void message_handler(const char *topic, const char *content_type, const void *payload, uint32_t payload_len)
{
  if (topic && content_type && payload)
  {
    printf("Received message: topic=%s, content_type=%s, payload=%s, len=%u\n",
           topic, content_type, (const char *)payload, payload_len);
  }
  else
  {
    printf("Invalid message data received\n");
  }
}
