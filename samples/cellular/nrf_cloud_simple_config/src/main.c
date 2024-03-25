/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <date_time.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_coap.h>
#include <modem/modem_info.h>
#include <zephyr/sys/reboot.h>

#include <dk_buttons_and_leds.h>

#include "simple_config.h"

/* Register log module */
LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);


/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

#define SEND_FATAL_ERROR() \
		LOG_ERR("fatal error");	\
		LOG_PANIC();									\
		IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)));

K_SEM_DEFINE(date_time_ready_sem, 0, 1);
K_SEM_DEFINE(connected_sem, 0, 1);

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established");
		if (k_sem_count_get(&date_time_ready_sem) == 1) {
			LOG_INF("reconnecting to cloud service");
			nrf_cloud_coap_connect(NULL);
		}
		k_sem_give(&connected_sem);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		nrf_cloud_coap_disconnect();
		break;
	default:
		/* Don't care */
		return;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint32_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
		SEND_FATAL_ERROR();
		return;
	}
}

static void date_time_handler(const struct date_time_evt *evt) {
	if (evt->type != DATE_TIME_NOT_OBTAINED) {
		LOG_INF("time aquired");
		k_sem_give(&date_time_ready_sem);
	}
}

static void network_task(void)
{
	int err;

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	/* Setup handler for date_time library */
	date_time_register_handler(date_time_handler);

	/* Connecting to the configured connectivity layer.
	 * Wi-Fi or LTE depending on the board that the sample was built for.
	 */
	LOG_INF("Bringing network interface up and connecting to the network");

	err = conn_mgr_all_if_up(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", err);
		SEND_FATAL_ERROR();
		return;
	}

	err = conn_mgr_all_if_connect(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", err);
		SEND_FATAL_ERROR();
		return;
	}

	/* Resend connection status if the sample is built for Native Posix.
	 * This is necessary because the network interface is automatically brought up
	 * at SYS_INIT() before main() is called.
	 * This means that NET_EVENT_L4_CONNECTED fires before the
	 * appropriate handler l4_event_handler() is registered.
	 */
	if (IS_ENABLED(CONFIG_BOARD_NATIVE_POSIX)) {
		conn_mgr_mon_resend_status();
	}
}

K_THREAD_DEFINE(network_task_id,
		1024,
		network_task, NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO , 0, 0);


int config_cb(const char *key, const struct simple_config_val *val)
{
	LOG_INF("config_cb: %s", key);
	switch (val->type) {
	case SIMPLE_CONFIG_VAL_BOOL:
		LOG_INF("bool: %d", val->val._bool);
		break;
	case SIMPLE_CONFIG_VAL_DOUBLE:
		LOG_INF("double: %f", val->val._double);
		break;
	case SIMPLE_CONFIG_VAL_STRING:
		LOG_INF("string: %s", val->val._str);
		break;
	default:
		LOG_ERR("unknown type");
		return -EINVAL;
	}

	if (strcmp(key, "led_red") == 0) {
		if (val->type != SIMPLE_CONFIG_VAL_BOOL) {
			LOG_ERR("invalid type");
			return -EINVAL;
		}
		if (val->val._bool) {
			dk_set_led_on(DK_LED1);
		} else {
			dk_set_led_off(DK_LED1);
		}
	}

	return 0;
}

int main(void)
{
	int err = nrf_cloud_coap_init();
	if (err)
	{
		LOG_ERR("nrf_cloud_coap_init, error: %d", err);
		SEND_FATAL_ERROR();
	}
	LOG_INF("Hello World! %s", CONFIG_BOARD);

	simple_config_set_callback(config_cb);

	dk_leds_init();
	dk_set_led_on(DK_LED1);

	LOG_INF("waiting for network connectivity");
	k_sem_take(&connected_sem, K_FOREVER);
	k_sem_take(&date_time_ready_sem, K_FOREVER);

	LOG_INF("Connecting to cloud service");
	while (true)
	{
		err = nrf_cloud_coap_connect(NULL);
		if (err)
		{
			LOG_ERR("nrf_cloud_coap_connect, error: %d", err);
			k_sleep(K_SECONDS(1));
		}
		else
		{
			break;
		}
	}


	struct nrf_cloud_svc_info_ui ui_info = {
	    .gnss = true,
	};
	struct nrf_cloud_svc_info service_info = {
	    .ui = &ui_info};
	struct nrf_cloud_modem_info modem_info = {
	    .device = NRF_CLOUD_INFO_SET,
	    .network = NRF_CLOUD_INFO_SET,
	};
	struct nrf_cloud_device_status device_status = {
	    .modem = &modem_info,
	    .svc = &service_info};

	/* sending device info to shadow */
	err = nrf_cloud_coap_shadow_device_status_update(&device_status);
	if (err)
	{
		LOG_ERR("nrf_cloud_coap_shadow_device_status_update, error: %d", err);
		SEND_FATAL_ERROR();
	}

	err = simple_config_set("led_red", &(struct simple_config_val){.type = SIMPLE_CONFIG_VAL_BOOL, .val._bool = true});
	if (err)
	{
		LOG_ERR("simple_config_set, error: %d", err);
	}
	while (true) {
		simple_config_update();
		k_sleep(K_SECONDS(10));
	}

	return 0;
}
