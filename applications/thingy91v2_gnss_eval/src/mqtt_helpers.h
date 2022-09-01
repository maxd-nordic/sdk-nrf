#pragma once

#include <zephyr/net/mqtt.h>

/**@brief Initialize the MQTT client structure
 */
int _mqtt_client_init(struct mqtt_client *);

int _mqtt_establish_connection(struct mqtt_client *);

int _mqtt_handle_connection(struct mqtt_client *);

/**@brief Function to publish data on the configured topic
 */
int _mqtt_data_publish(struct mqtt_client *, const char *);

/**@brief Function to publish status on the configured topic
 */
int _mqtt_status_publish(struct mqtt_client *, const char *);
