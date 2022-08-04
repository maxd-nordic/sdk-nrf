/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/random/rand32.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>

#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <dk_buttons_and_leds.h>
#include <date_time.h>
#include <nrf_modem_gnss.h>

LOG_MODULE_REGISTER(gnss_eval, CONFIG_GNSS_EVAL_LOG_LEVEL);

/* Buffers for MQTT client. */
static uint8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t payload_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

static uint8_t status_topic[100];
static uint8_t data_topic[100];
#define COMMAND_TOPIC (CONFIG_MQTT_TOPIC_PREFIX "/" CONFIG_MQTT_COMMAND_TOPIC)

/* The mqtt client struct */
static struct mqtt_client client;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

/* File descriptor */
static struct pollfd fds;

/* date time semaphore*/
static K_SEM_DEFINE(time_sem, 0, 1);

static struct nrf_modem_gnss_pvt_data_frame last_pvt;
static uint32_t time_to_fix;
static uint64_t fix_timestamp;

static uint32_t mqtt_connect_attempt;

static enum state_type {
	STATE_INIT,
	STATE_CONNECT_MQTT,
	STATE_WAITING,
	STATE_MEASURE,
} state;

/**@brief Function to publish data on the configured topic
 */
static int data_publish(struct mqtt_client *c, enum mqtt_qos qos,
	uint8_t *data, size_t len)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = data_topic;
	param.message.topic.topic.size = strlen(param.message.topic.topic.utf8);
	param.message.payload.data = data;
	param.message.payload.len = len;
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;
	param.retain_flag = 0;

	return mqtt_publish(c, &param);
}

/**@brief Function to publish status on the configured topic
 */
static int status_publish(struct mqtt_client *c, const char* status)
{
	struct mqtt_publish_param param;
	param.message.topic.qos = MQTT_QOS_0_AT_MOST_ONCE;
	param.message.topic.topic.utf8 = status_topic;
	param.message.topic.topic.size = strlen(param.message.topic.topic.utf8);
	param.message.payload.data = status;
	param.message.payload.len = strlen(param.message.payload.data);
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;
	param.retain_flag = 0;

	return mqtt_publish(c, &param);

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

/**@brief Function to read the published payload.
 */
static int publish_get_payload(struct mqtt_client *c, size_t length)
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

/**@brief MQTT client event handler
 */
void mqtt_evt_handler(struct mqtt_client *const c,
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
		subscribe(c);
		ret = status_publish(&client, "ok");
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
		ret = publish_get_payload(c, p->message.payload.len);

		if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack = {
				.message_id = p->message_id
			};

			/* Send acknowledgement. */
			mqtt_publish_qos1_ack(&client, &ack);
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
					/* TODO: start GNSS measurement now */
					state = STATE_MEASURE;
					mqtt_connect_attempt = 0;
					/*
					date_time_now(&now);
					ret = snprintf(payload_buf, sizeof(payload_buf),
						       "current time: %.0lf", (double)now/1000);
					if (ret > 0) {
						data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, payload_buf, ret);
					}
					ret = status_publish(&client, "ok");
					if (ret) {
						LOG_ERR("Publish failed: %d", ret);
					}
					*/
				} else {
					LOG_ERR("requested time is in the past");
					ret = status_publish(&client, "missed");
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

			ret = mqtt_disconnect(c);
			if (ret) {
				LOG_ERR("Could not disconnect: %d", ret);
			}
		}
	} break;

	default:
		LOG_INF("Unhandled MQTT event type: %d", evt->type);
		break;
	}
}

/**@brief Resolves the configured hostname and
 * initializes the MQTT broker structure
 */
static int broker_init(void)
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

#define IMEI_LEN 15
#define CGSN_RESPONSE_LENGTH (IMEI_LEN + 6 + 1) /* Add 6 for \r\nOK\r\n and 1 for \0 */
#define CLIENT_ID_LEN sizeof("nrf-") + IMEI_LEN

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


#define WILL_MSG "offline"
/**@brief Initialize the MQTT client structure
 */
static int client_init(struct mqtt_client *client)
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

	err = broker_init();
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

/**@brief Initialize the file descriptor structure used by poll.
 */
static int fds_init(struct mqtt_client *c)
{
	if (c->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds.fd = c->transport.tcp.sock;
	} else {
		return -ENOTSUP;
	}

	fds.events = POLLIN;

	return 0;
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & BIT(0)) {
		int ret;

		ret = status_publish(&client, "button");
		if (ret) {
			LOG_ERR("Publish failed: %d", ret);
		}
	}
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static int modem_configure(void)
{
	lte_lc_psm_req(true);

	int err;

	LOG_INF("LTE Link Connecting...");
	err = lte_lc_init_and_connect();
	if (err) {
		LOG_INF("Failed to establish LTE connection: %d", err);
		return err;
	}
	LOG_INF("LTE Link Connected!");
	return 0;
}

static void date_time_evt_handler(const struct date_time_evt *evt)
{
	if (evt->type != DATE_TIME_NOT_OBTAINED) {
		k_sem_give(&time_sem);
	}
}

static int establish_mqtt_connection(void) {
	int ret;

	ret = mqtt_connect(&client);
	if (ret != 0) {
		LOG_ERR("mqtt_connect %d", ret);
		return ret;
	}

	ret = fds_init(&client);
	if (ret != 0) {
		LOG_ERR("fds_init: %d", ret);
		return ret;
	}
	return EXIT_SUCCESS;
}

static int handle_mqtt_connection(void) {
	int ret;

	ret = poll(&fds, 1, mqtt_keepalive_time_left(&client));
	if (ret < 0) {
		LOG_ERR("poll: %d", errno);
		return mqtt_disconnect(&client);
	}

	ret = mqtt_live(&client);
	if ((ret != 0) && (ret != -EAGAIN)) {
		LOG_ERR("ERROR: mqtt_live: %d", ret);
		return mqtt_disconnect(&client);
	}

	if ((fds.revents & POLLIN) == POLLIN) {
		ret = mqtt_input(&client);
		if (ret != 0) {
			LOG_ERR("mqtt_input: %d", ret);
			return mqtt_disconnect(&client);
		}
	}

	if ((fds.revents & POLLERR) == POLLERR) {
		LOG_ERR("POLLERR");
		return mqtt_disconnect(&client);
	}

	if ((fds.revents & POLLNVAL) == POLLNVAL) {
		LOG_ERR("POLLNVAL");
		return mqtt_disconnect(&client);
	}
	
	return EXIT_SUCCESS;
}

static void ttff_test_got_fix_work_fn(struct k_work *item)
{
	LOG_INF("Time to fix: %u", time_to_fix);
	int ret = snprintf(payload_buf, sizeof(payload_buf), "ttf: %u", time_to_fix);
	if (ret > 0) {
		data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, payload_buf, ret);
	}
	state = STATE_INIT;
}
static struct k_work_delayable ttff_test_got_fix_work;
static K_WORK_DELAYABLE_DEFINE(ttff_test_got_fix_work, ttff_test_got_fix_work_fn);

static void gnss_event_handler(int event)
{
	int retval;
	switch (event) {
	case NRF_MODEM_GNSS_EVT_PVT:
		retval = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);
		if (retval == 0) {
			LOG_INF("PVT received");
		}
		break;
	case NRF_MODEM_GNSS_EVT_FIX:
		time_to_fix = (k_uptime_get() - fix_timestamp) / 1000;
		k_work_reschedule(&ttff_test_got_fix_work, K_MSEC(100));
		break;
	default:
		break;
	}
}

static int ttff_test_force_cold_start(void)
{
	int err;
	uint32_t delete_mask;

	LOG_INF("Deleting GNSS data");

	/* Delete everything else except the TCXO offset. */
	delete_mask = NRF_MODEM_GNSS_DELETE_EPHEMERIDES |
		      NRF_MODEM_GNSS_DELETE_ALMANACS |
		      NRF_MODEM_GNSS_DELETE_IONO_CORRECTION_DATA |
		      NRF_MODEM_GNSS_DELETE_LAST_GOOD_FIX |
		      NRF_MODEM_GNSS_DELETE_GPS_TOW |
		      NRF_MODEM_GNSS_DELETE_GPS_WEEK |
		      NRF_MODEM_GNSS_DELETE_UTC_DATA |
		      NRF_MODEM_GNSS_DELETE_GPS_TOW_PRECISION;

	/* With minimal assistance, we want to keep the factory almanac. */
	if (IS_ENABLED(CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL)) {
		delete_mask &= ~NRF_MODEM_GNSS_DELETE_ALMANACS;
	}

	err = nrf_modem_gnss_nv_data_delete(delete_mask);
	if (err) {
		LOG_ERR("Failed to delete GNSS data");
		return -1;
	}

	return 0;
}

int gnss_test(void) {
	int ret;

	LOG_INF("Starting GNSS test");

	if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_LTE) != 0) {
		LOG_ERR("Failed to return to power off modem");
	}

	if (ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS)) {
		LOG_ERR("Failed to activate GNSS functional mode %d", ret);
		return -1;
	}
	/* Configure GNSS. */
	if (ret = nrf_modem_gnss_event_handler_set(gnss_event_handler)) {
		LOG_ERR("Failed to set GNSS event handler %d", ret);
		return -1;
	}
	/* Disable all NMEA messages. */
	if (ret = nrf_modem_gnss_nmea_mask_set(0)) {
		LOG_ERR("Failed to set GNSS NMEA mask %d", ret);
		return -1;
	}

	uint8_t use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;
	if (ret = nrf_modem_gnss_use_case_set(use_case)) {
		LOG_ERR("Failed to set GNSS use case %d", ret);
		return -1;
	}

	if (ret = ttff_test_force_cold_start()) {
		LOG_ERR("Failed to reset asssistance data %d", ret);
		return -1;
	}

	LOG_INF("Starting GNSS");
	if (ret = nrf_modem_gnss_start()) {
		LOG_ERR("Failed to start GNSS %d", ret);
		return -1;
	}
	fix_timestamp = k_uptime_get();
	return 0;
}

void main(void)
{
	int ret;

	/* one-time setup */
	date_time_register_handler(date_time_evt_handler);
	dk_buttons_init(button_handler);

	/* modem state cleanup */
	nrf_modem_gnss_stop();
	if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_POWER_OFF) != 0) {
		LOG_ERR("Failed to return to power off modem");
	}


	while (true) {
		switch (state)
		{
		case STATE_INIT:
			/* LTE connect */
			ret = modem_configure();
			if (ret) {
				LOG_INF("Retrying in %d seconds",
					CONFIG_LTE_CONNECT_RETRY_DELAY_S);
				k_sleep(K_SECONDS(CONFIG_LTE_CONNECT_RETRY_DELAY_S));
				continue;
			}

			/* wait for datetime */
			LOG_INF("Waiting for time sync...");
			k_sem_take(&time_sem, K_MINUTES(10));
			LOG_INF("Got time sync!");

			/* MQTT setup */
			ret = client_init(&client);
			if (ret != 0) {
				LOG_ERR("mqtt client_init: %d", ret);
				continue;
			}
			state = STATE_CONNECT_MQTT;
			break;
		case STATE_CONNECT_MQTT:
			if (mqtt_connect_attempt++ > 0) {
				LOG_INF("Reconnecting in %d seconds...",
					CONFIG_MQTT_RECONNECT_DELAY_S);
				k_sleep(K_SECONDS(CONFIG_MQTT_RECONNECT_DELAY_S));
			}
			ret = establish_mqtt_connection();
			if (!ret) {
				state = STATE_WAITING;
			}
			break;
		case STATE_WAITING:
			ret = handle_mqtt_connection();
			if (ret) {
				state = STATE_CONNECT_MQTT;
			}
			break;
		case STATE_MEASURE:
			if (gnss_test()) {
				state = STATE_INIT;
			}
		default:
			break;
		}
	}
}
