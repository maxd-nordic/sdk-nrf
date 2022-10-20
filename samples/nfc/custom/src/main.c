/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#include <nrfx.h>
#include <nrf.h>
#include <hal/nrf_nfct.h>
#include <hal/nrf_power.h>
#if !NRF_POWER_HAS_RESETREAS
#include <hal/nrf_reset.h>
#endif

#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>

#include <dk_buttons_and_leds.h>
#include <nrfx_nfct.h>

#include "memory.h"

#define SYSTEM_OFF_DELAY_S	3

#define MAX_REC_COUNT		1
#define NDEF_MSG_BUF_SIZE	128

#define NFC_FIELD_LED		DK_LED1
#define SYSTEM_ON_LED		DK_LED2


/* Delayed work that enters system off. */
static struct k_work_delayable system_off_work;

static volatile uint8_t rx_buffer[16];
static const nrfx_nfct_data_desc_t rx_buffer_param =
{
	.p_data    = rx_buffer,
	.data_size = ARRAY_SIZE(rx_buffer)
};
static volatile uint8_t tx_buffer[16];
static nrfx_nfct_data_desc_t tx_buffer_param =
{
	.p_data    = tx_buffer,
	.data_size = ARRAY_SIZE(tx_buffer)
};

static void handle_command(nrfx_nfct_evt_rx_frameend_t const * p_rx_frameend) {
	const uint8_t command = p_rx_frameend->rx_data.p_data[0];
	const uint32_t length = p_rx_frameend->rx_data.data_size;
	printk("command: 0x%02X %02X, length: %d\n", command, p_rx_frameend->rx_data.p_data[1], length);
	if (command == 0x30 && length == 2) {
		command_0x30(p_rx_frameend->rx_data.p_data, tx_buffer);
		nrfx_nfct_tx(&tx_buffer_param, NRF_NFCT_FRAME_DELAY_MODE_WINDOWGRID);
	}
}

static void nrfx_nfct_evt_handler(nrfx_nfct_evt_t const * p_nfct_evt)
{
	nfc_platform_event_handler(p_nfct_evt);
	switch (p_nfct_evt->evt_id)
	{
		case NRFX_NFCT_EVT_FIELD_DETECTED:
			break;
		case NRFX_NFCT_EVT_FIELD_LOST:
			break;
		case NRFX_NFCT_EVT_SELECTED:
			nrfx_nfct_rx(&rx_buffer_param);
			break;
		case NRFX_NFCT_EVT_RX_FRAMESTART:
			break;
		case NRFX_NFCT_EVT_RX_FRAMEEND:
			handle_command(&p_nfct_evt->params.rx_frameend);
			break;
		case NRFX_NFCT_EVT_TX_FRAMESTART:
			break;
		case NRFX_NFCT_EVT_TX_FRAMEEND:
			nrfx_nfct_rx(&rx_buffer_param);
			break;
		case NRFX_NFCT_EVT_ERROR:
			break;
		default:
			break;
	}
}


/**
 * @brief Function for configuring and starting the NFC.
 */
static int start_nfc(void)
{

	nrfx_nfct_config_t config =
	{
		.rxtx_int_mask = NRF_NFCT_INT_FIELDDETECTED_MASK |
			NRF_NFCT_INT_FIELDLOST_MASK |
			NRF_NFCT_INT_SELECTED_MASK |
			NRF_NFCT_INT_RXFRAMESTART_MASK |
			NRF_NFCT_INT_RXFRAMEEND_MASK |
			NRF_NFCT_INT_TXFRAMESTART_MASK |
			NRF_NFCT_INT_TXFRAMEEND_MASK |
			NRF_NFCT_INT_ERROR_MASK,
		.cb = nrfx_nfct_evt_handler
	};

	nfc_platform_setup();

	uint8_t uid[4] = {0xde, 0xad, 0xbe, 0xef};
	nrfx_nfct_param_t uid_param = {
		.id = NRFX_NFCT_PARAM_ID_NFCID1,
		.data.nfcid1 = {
			.id_size = NRFX_NFCT_NFCID1_SINGLE_SIZE,
			.p_id = uid,
		},
	};

	init_internal_buffer(uid, ARRAY_SIZE(uid));

	nrfx_nfct_parameter_set(&uid_param);
	//NRF_NFCT->SELRES = 0x00;
	//NRF_NFCT->SENSRES = 0x0044;
	nrfx_nfct_init(&config);
	nrfx_nfct_enable();

	printk("NFC configuration done\n");
	return 0;
}


/**
 * @brief Function entering system off.
 * System off is delayed to make sure that NFC tag was correctly read.
 */
static void system_off(struct k_work *work)
{
	printk("Entering system off.\nApproach a NFC reader to restart.\n");

	/* Before we disabled entry to deep sleep. Here we need to override
	 * that, then force a sleep so that the deep sleep takes effect.
	 */
	const struct pm_state_info si = {PM_STATE_SOFT_OFF, 0, 0};

	pm_state_force(0, &si);

	dk_set_led_off(SYSTEM_ON_LED);

	/* Going into sleep will actually go to system off mode, because we
	 * forced it above.
	 */
	k_sleep(K_MSEC(1));

	/* k_sleep will never exit, so below two lines will never be executed
	 * if system off was correct. On the other hand if someting gone wrong
	 * we will see it on terminal and LED.
	 */
	dk_set_led_on(SYSTEM_ON_LED);
	printk("ERROR: System off failed\n");
}


/**
 * @brief  Helper function for printing the reason of the last reset.
 * Can be used to confirm that NCF field actually woke up the system.
 */
static void print_reset_reason(void)
{
	uint32_t reas;

#if NRF_POWER_HAS_RESETREAS

	reas = nrf_power_resetreas_get(NRF_POWER);
	nrf_power_resetreas_clear(NRF_POWER, reas);
	if (reas & NRF_POWER_RESETREAS_NFC_MASK) {
		printk("Wake up by NFC field detect\n");
	} else if (reas & NRF_POWER_RESETREAS_RESETPIN_MASK) {
		printk("Reset by pin-reset\n");
	} else if (reas & NRF_POWER_RESETREAS_SREQ_MASK) {
		printk("Reset by soft-reset\n");
	} else if (reas) {
		printk("Reset by a different source (0x%08X)\n", reas);
	} else {
		printk("Power-on-reset\n");
	}

#else

	reas = nrf_reset_resetreas_get(NRF_RESET);
	nrf_reset_resetreas_clear(NRF_RESET, reas);
	if (reas & NRF_RESET_RESETREAS_NFC_MASK) {
		printk("Wake up by NFC field detect\n");
	} else if (reas & NRF_RESET_RESETREAS_RESETPIN_MASK) {
		printk("Reset by pin-reset\n");
	} else if (reas & NRF_RESET_RESETREAS_SREQ_MASK) {
		printk("Reset by soft-reset\n");
	} else if (reas) {
		printk("Reset by a different source (0x%08X)\n", reas);
	} else {
		printk("Power-on-reset\n");
	}

#endif
}


void main(void)
{
	/* Configure LED-pins */
	if (dk_leds_init() < 0) {
		printk("Cannot init LEDs!\n");
		return;
	}
	dk_set_led_on(SYSTEM_ON_LED);

	/* Configure and start delayed work that enters system off */
	k_work_init_delayable(&system_off_work, system_off);
	k_work_reschedule(&system_off_work, K_SECONDS(SYSTEM_OFF_DELAY_S));

	/* Show last reset reason */
	print_reset_reason();

	/* Start NFC */
	if (start_nfc() < 0) {
		printk("ERROR: NFC configuration failed\n");
		return;
	}

	/* Prevent deep sleep (system off) from being entered */
	pm_policy_state_lock_get(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);

	/* Exit main function - the rest will be done by the callbacks */
}
