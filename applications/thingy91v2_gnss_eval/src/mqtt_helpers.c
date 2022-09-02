#include "app.h"
#include "mqtt_helpers.h"

#include <zephyr/random/rand32.h>
#include <zephyr/net/socket.h>
#include <nrf_modem_at.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <date_time.h>

LOG_MODULE_REGISTER(gnss_eval_mqtt, CONFIG_GNSS_EVAL_LOG_LEVEL);

#define IMEI_LEN 15
#define CGSN_RESPONSE_LENGTH (IMEI_LEN + 6 + 1) /* Add 6 for \r\nOK\r\n and 1 for \0 */
#define CLIENT_ID_LEN sizeof("nrf-") + IMEI_LEN
#define WILL_MSG "offline"

/* MQTT Broker details. */
static struct sockaddr_storage broker;

/* File descriptor */
static struct pollfd fds;

static uint8_t status_topic[100];
static uint8_t data_topic[100];
#define COMMAND_TOPIC (CONFIG_MQTT_TOPIC_PREFIX "/" CONFIG_MQTT_COMMAND_TOPIC)

/* Buffers for MQTT client. */
static uint8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t payload_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

/* Function to get the client id */
static const uint8_t* client_id_get(void)
{
	static uint8_t client_id[MAX(sizeof(CONFIG_MQTT_CLIENT_ID),
				     CLIENT_ID_LEN)];

	if (strlen(CONFIG_MQTT_CLIENT_ID) > 0) {
		snprintf(client_id, sizeof(client_id), "%s",
			 CONFIG_MQTT_CLIENT_ID);
		goto exit;
	}

	char imei_buf[CGSN_RESPONSE_LENGTH + 1];
	int err;

	err = nrf_modem_at_cmd(imei_buf, sizeof(imei_buf), "AT+CGSN");
	if (err) {
		LOG_ERR("Failed to obtain IMEI, error: %d", err);
		goto exit;
	}

	imei_buf[IMEI_LEN] = '\0';

	snprintf(client_id, sizeof(client_id), "nrf-%.*s", IMEI_LEN, imei_buf);

exit:
	LOG_DBG("client_id = %s", log_strdup(client_id));

	return client_id;
}

/**@brief Resolves the configured hostname and
 * initializes the MQTT broker structure
 */
static int mqtt_broker_init(void)
{
	int err;
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(CONFIG_MQTT_BROKER_HOSTNAME, NULL, &hints, &result);
	if (err) {
		LOG_ERR("getaddrinfo failed: %d", err);
		return -ECHILD;
	}

	addr = result;

	/* Look for address of the broker. */
	while (addr != NULL) {
		/* IPv4 Address. */
		if (addr->ai_addrlen == sizeof(struct sockaddr_in)) {
			struct sockaddr_in *broker4 =
				((struct sockaddr_in *)&broker);
			char ipv4_addr[NET_IPV4_ADDR_LEN];

			broker4->sin_addr.s_addr =
				((struct sockaddr_in *)addr->ai_addr)
				->sin_addr.s_addr;
			broker4->sin_family = AF_INET;
			broker4->sin_port = htons(CONFIG_MQTT_BROKER_PORT);

			inet_ntop(AF_INET, &broker4->sin_addr.s_addr,
				  ipv4_addr, sizeof(ipv4_addr));
			LOG_INF("IPv4 Address found %s", log_strdup(ipv4_addr));

			break;
		} else {
			LOG_ERR("ai_addrlen = %u should be %u or %u",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
		}

		addr = addr->ai_next;
	}

	/* Free the address. */
	freeaddrinfo(result);

	return err;
}

/**@brief Function to read the published payload.
 */
static int get_payload(struct mqtt_client *c, size_t length)
{
	int ret;
	int err = 0;

	/* Return an error if the payload is larger than the payload buffer.
	 * Note: To allow new messages, we have to read the payload before returning.
	 */
	if (length > sizeof(payload_buf)) {
		err = -EMSGSIZE;
	}

	/* Truncate payload until it fits in the payload buffer. */
	while (length > sizeof(payload_buf)) {
		ret = mqtt_read_publish_payload_blocking(
				c, payload_buf, (length - sizeof(payload_buf)));
		if (ret == 0) {
			return -EIO;
		} else if (ret < 0) {
			return ret;
		}

		length -= ret;
	}

	ret = mqtt_readall_publish_payload(c, payload_buf, length);
	if (ret) {
		return ret;
	}

	return err;
}

/**@brief Function to subscribe to the configured topic
 */
static int subscribe(struct mqtt_client *c)
{
	struct mqtt_topic subscribe_topic = {
		.topic = {
			.utf8 = COMMAND_TOPIC,
			.size = strlen(COMMAND_TOPIC)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = 1234
	};

	return mqtt_subscribe(c, &subscription_list);
}

/**@brief MQTT client event handler
 */
static void mqtt_evt_handler(struct mqtt_client *const client,
		      const struct mqtt_evt *evt)
{
	int ret;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT connect failed: %d", evt->result);
			break;
		}

		LOG_INF("MQTT client connected");
		subscribe(client);
		ret = _mqtt_status_publish(client, "connected");
		if (ret) {
			LOG_ERR("Publish failed: %d", ret);
		}
		break;
	case MQTT_EVT_DISCONNECT:
		LOG_INF("MQTT client disconnected: %d", evt->result);
		break;
	case MQTT_EVT_PUBLISH: {
		/* Command received */
		const struct mqtt_publish_param *p = &evt->param.publish;

		LOG_INF("MQTT PUBLISH result=%d len=%d",
			evt->result, p->message.payload.len);
		ret = get_payload(client, p->message.payload.len);

		if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack = {
				.message_id = p->message_id
			};

			/* Send acknowledgement. */
			mqtt_publish_qos1_ack(client, &ack);
		}

		if (ret >= 0) {
			payload_buf[p->message.payload.len] = 0;
			int64_t unix_time_s = 0;
			ret = sscanf(payload_buf, "%lld", &unix_time_s);
			
			int64_t now = 0; date_time_now(&now);
			if (ret == 1) {
				int64_t delta = (unix_time_s*1000)-now;
				LOG_INF("will wait %lld ms", delta);
				if (delta > 0){
					k_sleep(K_MSEC(delta));
					set_state(STATE_MEASURE);
				} else {
					LOG_ERR("requested time is in the past");
					ret = _mqtt_status_publish(client, "missed");
					if (ret) {
						LOG_ERR("Publish failed: %d", ret);
					}
				}
			}
		} else if (ret == -EMSGSIZE) {
			LOG_ERR("Received payload (%d bytes) is larger than the payload buffer "
				"size (%d bytes).",
				p->message.payload.len, sizeof(payload_buf));
		} else {
			LOG_ERR("publish_get_payload failed: %d", ret);
			LOG_INF("Disconnecting MQTT client...");

			ret = mqtt_disconnect(client);
			if (ret) {
				LOG_ERR("Could not disconnect: %d", ret);
			}
		}
	} break;

	default:
		LOG_DBG("Unhandled MQTT event type: %d", evt->type);
		break;
	}
}

int _mqtt_client_init(struct mqtt_client *client)
{
	int err;

	static uint8_t will_message_str[10];
	snprintf(will_message_str, sizeof(will_message_str), "%s", WILL_MSG);

	static struct mqtt_topic will_topic = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.utf8 = status_topic,
	};
	static struct mqtt_utf8 will_message = {
		.utf8 = will_message_str,
	};

	mqtt_client_init(client);

	err = mqtt_broker_init();
	if (err) {
		LOG_ERR("Failed to initialize broker connection");
		return err;
	}

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = client_id_get();
	client->client_id.size = strlen(client->client_id.utf8);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	client->will_message = &will_message;
	client->will_topic = &will_topic;

	snprintf(status_topic, sizeof(status_topic), "%s/%s/%s", CONFIG_MQTT_TOPIC_PREFIX, client->client_id.utf8, "status");
	snprintf(data_topic, sizeof(data_topic), "%s/%s/%s", CONFIG_MQTT_TOPIC_PREFIX, client->client_id.utf8, "data");
	will_message.size = strlen(WILL_MSG);
	will_topic.topic.size = strlen(status_topic);

	return err;
}

int _mqtt_establish_connection(struct mqtt_client *client) {
	int ret;

	ret = mqtt_connect(client);
	if (ret != 0) {
		LOG_ERR("mqtt_connect %d", ret);
		return ret;
	}

        if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds.fd = client->transport.tcp.sock;
        	fds.events = POLLIN;
	} else {
		LOG_ERR("fds init fail: transport type");
		return ret;
	}

	return EXIT_SUCCESS;
}

int _mqtt_handle_connection(struct mqtt_client *client) {
	int ret;

	ret = poll(&fds, 1, mqtt_keepalive_time_left(client));
	if (ret < 0) {
		LOG_ERR("poll: %d", errno);
		return mqtt_disconnect(client);
	}

	ret = mqtt_live(client);
	if ((ret != 0) && (ret != -EAGAIN)) {
		LOG_ERR("ERROR: mqtt_live: %d", ret);
		return mqtt_disconnect(client);
	}

	if ((fds.revents & POLLIN) == POLLIN) {
		ret = mqtt_input(client);
		if (ret != 0) {
			LOG_ERR("mqtt_input: %d", ret);
			return mqtt_disconnect(client);
		}
	}

	if ((fds.revents & POLLERR) == POLLERR) {
		LOG_ERR("POLLERR");
		return mqtt_disconnect(client);
	}

	if ((fds.revents & POLLNVAL) == POLLNVAL) {
		LOG_ERR("POLLNVAL");
		return mqtt_disconnect(client);
	}
	
	return EXIT_SUCCESS;
}


static int publish(struct mqtt_client *client, const char *data, const char *topic, size_t len) {
	struct mqtt_publish_param param;
	param.message.topic.qos = MQTT_QOS_0_AT_MOST_ONCE;
	param.message.topic.topic.utf8 = topic;
	param.message.topic.topic.size = strlen(param.message.topic.topic.utf8);
	param.message.payload.data = data;
	param.message.payload.len = len;
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;
	param.retain_flag = 0;

	return mqtt_publish(client, &param);
}

int _mqtt_data_publish(struct mqtt_client *client, const char *data)
{
	strncpy(payload_buf, data, ARRAY_SIZE(payload_buf));
	return publish(client, payload_buf, data_topic, strlen(payload_buf));
}

int _mqtt_data_publish_raw(struct mqtt_client *client, const char *data, size_t len)
{
	return publish(client, data, data_topic, len);
}

int _mqtt_status_publish(struct mqtt_client *client, const char *data)
{
	strncpy(payload_buf, data, ARRAY_SIZE(payload_buf));
	return publish(client, payload_buf, status_topic, strlen(payload_buf));
}
