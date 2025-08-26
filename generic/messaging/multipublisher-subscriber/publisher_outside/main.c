/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 */
#include <ocre_api.h>
#include <stdio.h>
#include <string.h>

#define TIMER_ID 2
#define TOPIC "temperature/outside"
#define CONTENT_TYPE "text/plain"

void timer_handler(void);

// WASM entry point
int main(void)
{
  if (ocre_timer_create(TIMER_ID) != OCRE_SUCCESS)
  {
    printf("Failed to create timer %d\n", TIMER_ID);
  }
  if (ocre_register_timer_callback(TIMER_ID, timer_handler) != OCRE_SUCCESS)
  {
    printf("Failed to register timer callback\n");
  }
  if (ocre_timer_start(TIMER_ID, 4000, 1) != OCRE_SUCCESS)
  {
    printf("Failed to start timer %d\n", TIMER_ID);
  }
  printf("Publisher initialized: timer %d started, publishing to %s\n", TIMER_ID, TOPIC);
  while (1)
  {
    ocre_process_events();
  }
  return 0;
}

void timer_handler(void)
{
  static int message_count = 0;
  char payload[32];
  snprintf(payload, sizeof(payload), "Temperature outside %d", message_count++);
  if (ocre_publish_message(TOPIC, CONTENT_TYPE, payload, strlen(payload) + 1) == OCRE_SUCCESS)
  {
    printf("Published: %s to topic %s\n", payload, TOPIC);
  }
  else
  {
    printf("Failed to publish message %d\n", message_count);
  }
}
