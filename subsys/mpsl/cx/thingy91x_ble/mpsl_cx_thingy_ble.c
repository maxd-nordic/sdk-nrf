/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements coexistence for the Thingy:91 X,
 *   where WiFi and LTE take precedence over BLE.
 *   Additionally, a SR_RF_EN signal has to be asserted whenever
 *   either BLE or WiFi use the radio.
 *
 */

#include <mpsl_cx_abstract_interface.h>

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include "hal/nrf_gpio.h"
#include <nrfx_gpiote.h>

#if DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_coex))
#define CX_NODE DT_NODELABEL(nrf_radio_coex)
#else
#define CX_NODE DT_INVALID_NODE
#error No enabled coex nodes registered in DTS.
#endif

static const struct gpio_dt_spec swctrl1_spec = GPIO_DT_SPEC_GET(CX_NODE, swctrl1_gpios);
static const struct gpio_dt_spec lte_coex2_spec = GPIO_DT_SPEC_GET(CX_NODE, lte_coex2_gpios);
static const struct gpio_dt_spec rf_fe_sr_en_spec = GPIO_DT_SPEC_GET(CX_NODE, rf_fe_sr_en_gpios);

__ASSERT(swctrl1_spec.port == lte_coex2_spec.port,
	 "swctrl1 and lte_coex2 pins need to be on the same port!");

static bool swctrl1_asserted;
static bool lte_coex2_asserted;
static bool ble_requested;
static struct gpio_callback port_interrupt_cb;
static mpsl_cx_cb_t mpsl_cb;


static int32_t request(const mpsl_cx_request_t *req_params)
{
	ble_requested = true;
	update_rf_fe_sr_en();
}

static int32_t release(void)
{
	ble_requested = false;
	update_rf_fe_sr_en();
}

static int32_t granted_ops_get(mpsl_cx_op_map_t *granted_ops)
{
	*granted_ops = MPSL_CX_OP_IDLE_LISTEN | MPSL_CX_OP_RX;

	// TODO: does it make sense to allow listening here?
	if (!swctrl1_asserted && !lte_coex2_asserted) {
		*granted_ops  |= MPSL_CX_OP_TX;
	}
	return 0;
}

static int32_t register_callback(mpsl_cx_cb_t cb)
{
	mpsl_cb = cb;
	return 0;
}

/* BGS12WN6 power-up settling time is up to 7us */
/* TCK106AG takes something like 100-300us to actually ramp up power */
#define DELAY_BEFORE_ANTENNA_CHANNEL_IS_READY 310U

static uint32_t req_grant_delay_get(void)
{
	return 0; /* we don't have to wait between requests since they are free */
}

static const mpsl_cx_interface_t m_mpsl_cx_methods = {
	.p_request = request,
	.p_release = release,
	.p_granted_ops_get = granted_ops_get,
	.p_req_grant_delay_get = req_grant_delay_get,
	.p_register_callback = register_callback,
};

static void update_rf_fe_sr_en(void) {
	int ret = 0;
	mpsl_cx_op_map_t granted_ops = 0;

	/* RF Switch is powered if either BLE or Wi-Fi radio is active */
	gpio_pin_set_dt(rf_fe_sr_en_spec, (int)(ble_requested || swctrl1_asserted));

	if (!mpsl_cb) {
		return;
	}
	(void) granted_ops_get(&granted_ops);
	mpsl_cb(granted_ops);
}

static void gpiote_irq_handler(const struct device *gpiob, struct gpio_callback *cb, uint32_t pins)
{
	int err = 0;

	(void)gpiob;
	(void)cb;
	(void)pins;

	err = gpio_pin_get_dt(&swctrl1_spec);
	if (err < 0) {
		return;
	}
	swctrl1_asserted = err;

	err = gpio_pin_get_dt(&lte_coex2_spec);
	if (err < 0) {
		return;
	}
	lte_coex2_asserted = err;

	update_rf_fe_sr_en();
}

static int mpsl_cx_init(void)
{

	int32_t ret;

	callback = NULL;

	ret = mpsl_cx_interface_set(&m_mpsl_cx_methods);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&swctrl1_spec, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&lte_coex2_spec, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&rf_fe_sr_en_spec, GPIO_OUTPUT_LOW);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&swctrl1_spec,
					      GPIO_INT_ENABLE | GPIO_INT_EDGE | GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&lte_coex2_spec,
					      GPIO_INT_ENABLE | GPIO_INT_EDGE | GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&port_interrupt_cb, gpiote_irq_handler,
			   BIT(swctrl1_spec.pin) | BIT(lte_coex2_spec.pin));
	gpio_add_callback(lte_coex2_spec.port, &port_interrupt_cb);

	/* check initial pin state */
	gpiote_irq_handler(NULL, NULL, 0);

	return 0;
}

SYS_INIT(mpsl_cx_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
