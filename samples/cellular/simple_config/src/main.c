/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_coap.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>

LOG_MODULE_REGISTER(fota_support_coap, CONFIG_APP_LOG_LEVEL);

K_SEM_DEFINE(connected_sem, 0, 1);


#define FOTA_THREAD_DELAY_S 10
#define JOB_CHECK_RATE_MINUTES 5

static void fota_reboot(enum nrf_cloud_fota_reboot_status status);

/* FOTA support context */
static struct nrf_cloud_fota_poll_ctx ctx = {
	.reboot_fn = fota_reboot
};

void fota_reboot(enum nrf_cloud_fota_reboot_status status)
{
	switch (status) {
	case FOTA_REBOOT_SUCCESS:
		LOG_INF("Rebooting to complete FOTA update...");
		conn_mgr_all_if_down(true);
		k_sleep(K_SECONDS(FOTA_THREAD_DELAY_S));
		sys_reboot(SYS_REBOOT_COLD);
		break;
	case FOTA_REBOOT_REQUIRED:
		LOG_INF("Rebooting to install FOTA update...");
		conn_mgr_all_if_down(true);
		k_sleep(K_SECONDS(FOTA_THREAD_DELAY_S));
		sys_reboot(SYS_REBOOT_COLD);
		break;
	case FOTA_REBOOT_FAIL:
	case FOTA_REBOOT_SYS_ERROR:
	default:
		k_sleep(K_SECONDS(FOTA_THREAD_DELAY_S));
		sys_reboot(SYS_REBOOT_WARM);
		break;
	}
}

int coap_fota_init(void)
{
	return nrf_cloud_fota_poll_init(&ctx);
}

int coap_fota_begin(void)
{
	return nrf_cloud_fota_poll_start(&ctx);
}

int coap_fota_thread_fn(void)
{
	int err;

	while (1) {
		/* Wait until we are able to communicate. */
		LOG_DBG("Waiting for valid connection before processing FOTA");
		k_sem_take(&connected_sem, K_FOREVER);
		k_sem_give(&connected_sem);

		/* confirm updated image */
		boot_write_img_confirmed();

		/* Query for any pending FOTA jobs. If one is found, download and install
		 * it. This is a blocking operation which can take a long time.
		 * This function is likely to reboot in order to complete the FOTA update.
		 */
		err = nrf_cloud_fota_poll_process(&ctx);
		if (err == -EAGAIN) {
			LOG_DBG("Retrying in %d minute(s)",
				JOB_CHECK_RATE_MINUTES);
			k_sleep(K_MINUTES(JOB_CHECK_RATE_MINUTES));
		} else {
			k_sleep(K_SECONDS(FOTA_THREAD_DELAY_S));
		}
	}
	return 0;
}

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event,
			     struct net_if *iface)
{
	int err;

	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established");
		k_sem_give(&connected_sem);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		k_sem_take(&connected_sem, K_FOREVER);
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
		return;
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

	/* Connecting to the configured connectivity layer.
	 * Wi-Fi or LTE depending on the board that the sample was built for.
	 */
	LOG_INF("Bringing network interface up and connecting to the network");

	err = conn_mgr_all_if_up(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", err);
		return;
	}

	err = conn_mgr_all_if_connect(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", err);
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


int main(void)
{
	network_task();
	coap_fota_thread_fn();
}
