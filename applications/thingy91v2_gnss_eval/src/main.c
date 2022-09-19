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
#include <date_time.h>
#include <nrf_modem_gnss.h>
#include <modem/modem_info.h>

#include "app.h"
#include "mqtt_helpers.h"

LOG_MODULE_REGISTER(gnss_eval, CONFIG_GNSS_EVAL_LOG_LEVEL);

/* The mqtt client struct */
static struct mqtt_client client;

/* date time semaphore */
static K_SEM_DEFINE(time_sem, 0, 1);

/* measure wait semaphore */
static K_SEM_DEFINE(measure_sem, 0, 1);

static struct nrf_modem_gnss_pvt_data_frame pvts[CONFIG_GNSS_TIMEOUT_S+1];
static size_t pvts_idx = 0;
static size_t pvts_send_idx = 0;

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

		if (state == STATE_WAIT_FOR_COMMAND) {
			int64_t now_ms = 0; date_time_now(&now_ms);
			ret = _mqtt_command_publish(&client, now_ms/1000+30);
			if (ret) {
				LOG_ERR("command publish failed: %d", ret);
			}
		} else if (state == STATE_MEASURE_WAIT) {
			if(!k_sem_count_get(&measure_sem)) {
				k_sem_give(&measure_sem);
			}
		}
	}
}

bool test_modem_fw_ver(const char *should)
{
	int ret;
	char buf[41] = {0};
	char format[23] = {0};

	sprintf(format, "%%%%SHORTSWVER: %%%d[^\r\n]", ARRAY_SIZE(buf));

	ret = nrf_modem_at_scanf("AT%SHORTSWVER",
				 format,
				 buf);

	if (ret != 1) {
		LOG_ERR("Could not get FW version, error: %d", ret);
		return false;
	}
	if (strcmp(should, buf) != 0){
		LOG_ERR("Modem firmware is [%s], but should be [%s]", buf, should);
		return false;
	}
	return true;
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static int modem_configure(void)
{
	lte_lc_psm_req(true);

	// enable more detailed error messages
	nrf_modem_at_printf("AT+CMEE=1");
	nrf_modem_at_printf("AT+CNEC=24");

	nrf_modem_gnss_stop();

	int err;

	LOG_INF("LTE Link Connecting...");
	err = lte_lc_init_and_connect();
	if (err) {
		LOG_INF("Failed to establish LTE connection: %d", err);
		return err;
	}
	// force provider to Telenor
	nrf_modem_at_printf("AT+COPS=1,2,\"24201\"");
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
		retval = nrf_modem_gnss_read(&pvts[pvts_idx], sizeof(pvts[pvts_idx]), NRF_MODEM_GNSS_DATA_PVT);
		if (retval == 0) {
			LOG_INF("PVT received, flags: 0x%02x, sv:%d%d%d%d%d%d%d%d%d%d%d%d",
				pvts[pvts_idx].flags,
				pvts[pvts_idx].sv[0].flags,
				pvts[pvts_idx].sv[1].flags,
				pvts[pvts_idx].sv[2].flags,
				pvts[pvts_idx].sv[3].flags,
				pvts[pvts_idx].sv[4].flags,
				pvts[pvts_idx].sv[5].flags,
				pvts[pvts_idx].sv[6].flags,
				pvts[pvts_idx].sv[7].flags,
				pvts[pvts_idx].sv[8].flags,
				pvts[pvts_idx].sv[9].flags,
				pvts[pvts_idx].sv[10].flags,
				pvts[pvts_idx].sv[11].flags);
			pvts_idx++;
		}
		break;
	case NRF_MODEM_GNSS_EVT_FIX:
		dk_set_leds(STATE_GOT_FIX_COLOR);
		if (IS_ENABLED(CONFIG_STOP_AFTER_FIRST_FIX)){
			k_sem_give(&measure_sem);
		}
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

	if ((ret = nrf_modem_gnss_fix_retry_set(0))) {
		LOG_ERR("Failed to set GNSS timeout %d", ret);
		return -1;
	}

	LOG_INF("Starting GNSS");
	if ((ret = nrf_modem_gnss_start())) {
		LOG_ERR("Failed to start GNSS %d", ret);
		return -1;
	}
	return 0;
}



void main(void)
{
	int ret;
	bool send_connection_data = true;
	struct connection_data_type connection_data = {0};

	/* one-time setup */
	date_time_register_handler(date_time_evt_handler);
	dk_leds_init();
	dk_buttons_init(button_handler);

	/* modem state cleanup */
	nrf_modem_gnss_stop();
	if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_POWER_OFF) != 0) {
		LOG_ERR("Failed to return to power off modem");
	}
	if (modem_info_init() != 0) {
		LOG_ERR("Failed to initialized modem_info");
	}
	if (!test_modem_fw_ver("nrf9160_1.3.2")) {
		while (true)
		{
			dk_set_leds(STATE_INIT_COLOR);
			k_sleep(K_SECONDS(1));
			dk_set_leds(0);
			k_sleep(K_SECONDS(1));
		}
	}

	while (true) {
		switch (state)
		{
		case STATE_INIT:
			dk_set_leds(STATE_INIT_COLOR);
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
			send_connection_data = true;
			set_state(STATE_CONNECT_MQTT);
			break;
		case STATE_CONNECT_MQTT:
			dk_set_leds(STATE_CONNECT_MQTT_COLOR);
			if (mqtt_connect_attempt++ > 0) {
				LOG_INF("Reconnecting in %d seconds...",
					CONFIG_MQTT_RECONNECT_DELAY_S);
				k_sleep(K_SECONDS(CONFIG_MQTT_RECONNECT_DELAY_S));
			}
			ret = _mqtt_establish_connection(&client);
			if (!ret) {
				set_state(STATE_WAIT_FOR_COMMAND);
			}
			break;
		case STATE_WAIT_FOR_COMMAND:
			dk_set_leds(STATE_SEND_RESULTS_COLOR);
			if (send_connection_data){
				modem_info_string_get(MODEM_INFO_CELLID,
						      connection_data.cellid,
						      ARRAY_SIZE(connection_data.cellid));
				modem_info_short_get(MODEM_INFO_CUR_BAND, &connection_data.band);
				modem_info_short_get(MODEM_INFO_RSRP, &connection_data.rsrp);
				modem_info_short_get(MODEM_INFO_BATTERY, &connection_data.vbatt);
				ret = _mqtt_data_publish_raw(&client,
					(const uint8_t*) &connection_data,
					sizeof(struct connection_data_type));
				if (!ret) {
					send_connection_data = false;
				}
			}
			while (pvts_send_idx < pvts_idx) {
				ret = _mqtt_data_publish_raw(&client,
					(const uint8_t*) &pvts[pvts_send_idx],
					sizeof(struct nrf_modem_gnss_pvt_data_frame));
				if (ret) {
					break;
				}
				pvts_send_idx++;
			}
			dk_set_leds(STATE_WAIT_FOR_COMMAND_COLOR);
			ret = _mqtt_handle_connection(&client);
			if (ret) {
				set_state(STATE_CONNECT_MQTT);
			}
			break;
		case STATE_MEASURE:
			date_time_now(&connection_data.last_measure_ms);
			dk_set_leds(STATE_MEASURE_COLOR);
			if ((ret = mqtt_disconnect(&client))) {
				LOG_ERR("mqtt_disconnect failed: %d", ret);
			}
			pvts_idx = 0;
			pvts_send_idx = 0;
			if (gnss_test()){
				set_state(STATE_INIT);
			} else {
				set_state(STATE_MEASURE_WAIT);
			}
			break;
		case STATE_MEASURE_WAIT:
			dk_set_leds(STATE_MEASURE_WAIT_COLOR);
			ret = k_sem_take(&measure_sem, K_SECONDS(CONFIG_GNSS_TIMEOUT_S));
			if (ret) {
				LOG_ERR("GNSS timeout reached: %s", (ret==-EAGAIN)?"-EAGAIN":"-EBUSY");
			}
			set_state(STATE_INIT);
			break;
		default:
			break;
		}
	}
}
