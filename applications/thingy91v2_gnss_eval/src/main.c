/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>

#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <dk_buttons_and_leds.h>
#include <date_time.h>
#include <nrf_modem_gnss.h>

#include "app.h"
#include "mqtt_helpers.h"

LOG_MODULE_REGISTER(gnss_eval, CONFIG_GNSS_EVAL_LOG_LEVEL);

static uint8_t output_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

/* The mqtt client struct */
static struct mqtt_client client;

/* date time semaphore */
static K_SEM_DEFINE(time_sem, 0, 1);

/* measure wait semaphore */
static K_SEM_DEFINE(measure_sem, 0, 1);

static struct nrf_modem_gnss_pvt_data_frame last_pvt;
static uint32_t time_to_fix;
static uint64_t fix_timestamp;

static uint32_t mqtt_connect_attempt;
static enum state_type state;

void set_state(enum state_type _state) {
	state = _state;
	if (_state == STATE_MEASURE){
		mqtt_connect_attempt = 0;
	}
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & BIT(0)) {
		int ret;

		ret = _mqtt_status_publish(&client, "button");
		if (ret) {
			LOG_ERR("Publish failed: %d", ret);
		}
		set_state(STATE_MEASURE);
	}
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static int modem_configure(void)
{
	lte_lc_psm_req(true);

	nrf_modem_at_printf("AT+CMEE=1");
	nrf_modem_at_printf("AT+CNEC=24");

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

static void gnss_event_handler(int event)
{
	int retval;
	switch (event) {
	case NRF_MODEM_GNSS_EVT_PVT:
		retval = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);
		if (retval == 0) {
			LOG_INF("PVT received, flags: 0x%02x", last_pvt.flags);
		}
		break;
	case NRF_MODEM_GNSS_EVT_FIX:
		time_to_fix = (k_uptime_get() - fix_timestamp) / 1000;
		LOG_INF("Time to fix: %u", time_to_fix);
		k_sem_give(&measure_sem);
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

	if ((ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_LTE))) {
		LOG_ERR("Failed to return to power off modem %d", ret);
	}

	if ((ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS))) {
		LOG_ERR("Failed to activate GNSS functional mode %d", ret);
		return -1;
	}
	/* Configure GNSS. */
	if ((ret = nrf_modem_gnss_event_handler_set(gnss_event_handler))) {
		LOG_ERR("Failed to set GNSS event handler %d", ret);
		return -1;
	}
	/* Disable all NMEA messages. */
	if ((ret = nrf_modem_gnss_nmea_mask_set(0))) {
		LOG_ERR("Failed to set GNSS NMEA mask %d", ret);
		return -1;
	}

	uint8_t use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;
	if ((ret = nrf_modem_gnss_use_case_set(use_case))) {
		LOG_ERR("Failed to set GNSS use case %d", ret);
		return -1;
	}

	if ((ret = ttff_test_force_cold_start())) {
		LOG_ERR("Failed to reset asssistance data %d", ret);
		return -1;
	}

	LOG_INF("Starting GNSS");
	if ((ret = nrf_modem_gnss_start())) {
		LOG_ERR("Failed to start GNSS %d", ret);
		return -1;
	}
	fix_timestamp = k_uptime_get();
	time_to_fix = 0;
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
			ret = _mqtt_client_init(&client);
			if (ret != 0) {
				LOG_ERR("mqtt client_init: %d", ret);
				continue;
			}
			set_state(STATE_CONNECT_MQTT);
			break;
		case STATE_CONNECT_MQTT:
			if (mqtt_connect_attempt++ > 0) {
				LOG_INF("Reconnecting in %d seconds...",
					CONFIG_MQTT_RECONNECT_DELAY_S);
				k_sleep(K_SECONDS(CONFIG_MQTT_RECONNECT_DELAY_S));
			}
			ret = _mqtt_establish_connection(&client);
			if (!ret) {
				set_state(STATE_WAIT_FOR_COMMAND);
				if (time_to_fix) {
					ret = snprintf(output_buf, sizeof(output_buf),
						       "ttf: %us", time_to_fix);
					if (ret > 0) {
						_mqtt_data_publish(&client, output_buf);
					}
				}
			}
			break;
		case STATE_WAIT_FOR_COMMAND:
			ret = _mqtt_handle_connection(&client);
			if (ret) {
				set_state(STATE_CONNECT_MQTT);
			}
			break;
		case STATE_MEASURE:
			if ((ret = mqtt_disconnect(&client))) {
				LOG_ERR("mqtt_disconnect failed: %d", ret);
			}
			if (gnss_test()){
				set_state(STATE_INIT);
			} else {
				set_state(STATE_MEASURE_WAIT);
			}
			break;
		case STATE_MEASURE_WAIT:
			k_sem_take(&measure_sem, K_SECONDS(CONFIG_GNSS_TIMEOUT_S));
			set_state(STATE_INIT);
			break;
		default:
			break;
		}
	}
}
