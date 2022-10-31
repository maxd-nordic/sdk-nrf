/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/sys/crc.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#include <nrfx.h>
#include <nrf.h>
#include <hal/nrf_nfct.h>
#include <hal/nrf_power.h>

#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>

#include <dk_buttons_and_leds.h>
#include <nrfx_nfct.h>

#include "tag_data.h"

#define SYSTEM_OFF_DELAY_S	3

#define MAX_REC_COUNT		1
#define NDEF_MSG_BUF_SIZE	128

#define NFC_FIELD_LED		DK_LED1
#define SYSTEM_ON_LED		DK_LED2

/* Delayed work that enters system off. */
static struct k_work_delayable system_off_work;

static uint8_t rx_buffer[16];
static const nrfx_nfct_data_desc_t rx_buffer_param =
{
	.p_data    = rx_buffer,
	.data_size = ARRAY_SIZE(rx_buffer)
};
static uint8_t tx_buffer[16];
static nrfx_nfct_data_desc_t tx_buffer_param =
{
	.p_data    = tx_buffer,
	.data_size = ARRAY_SIZE(tx_buffer)
};

static void handle_command(nrfx_nfct_evt_rx_frameend_t const * p_rx_frameend) {
	int err;
	const uint8_t command = p_rx_frameend->rx_data.p_data[0];
	const uint32_t length = p_rx_frameend->rx_data.data_size;
	//printk("command: 0x%02X %02X, length: %d\n", command, p_rx_frameend->rx_data.p_data[1], length);

	if (command == 0x30 && length == 2) {
		uint8_t page = p_rx_frameend->rx_data.p_data[1];
		if (page *4 + 16 > ARRAY_SIZE(internal_buffer)) {
			tx_buffer_param.data_size = 1;
			tx_buffer[0] = 0x00; // NAK
		} else {
			memcpy(tx_buffer, internal_buffer+(page*4), 16);
			tx_buffer_param.data_size = 16;
		}
		err = nrfx_nfct_tx(&tx_buffer_param, NRF_NFCT_FRAME_DELAY_MODE_WINDOWGRID);
		if (err != NRFX_SUCCESS) {
			printk("nrfx_nfct_tx error: %08X\n", err);
		}
	} else if (command == 0x50) {
		nrfx_nfct_init_substate_force(NRFX_NFCT_ACTIVE_STATE_SLEEP);
	} else {
		nrfx_nfct_init_substate_force(NRFX_NFCT_ACTIVE_STATE_DEFAULT);
	}
}

static void nrfx_nfct_evt_handler(nrfx_nfct_evt_t const * p_nfct_evt)
{
	nfc_platform_event_handler(p_nfct_evt);
	switch (p_nfct_evt->evt_id)
	{
		case NRFX_NFCT_EVT_FIELD_DETECTED:
			printk("field detected\n");
			break;
		case NRFX_NFCT_EVT_FIELD_LOST:
			printk("field lost\n");
			break;
		case NRFX_NFCT_EVT_SELECTED:
			printk("selected\n");
			nrfx_nfct_rx(&rx_buffer_param);
			break;
		case NRFX_NFCT_EVT_RX_FRAMEEND:
			handle_command(&p_nfct_evt->params.rx_frameend);
			break;
		case NRFX_NFCT_EVT_TX_FRAMEEND:
			printk("tx frame end\n");
			nrfx_nfct_rx(&rx_buffer_param);
			break;
		case NRFX_NFCT_EVT_ERROR:
			printk("nfct_evt_error: %d\n", p_nfct_evt->params.error.reason);
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
			NRF_NFCT_INT_RXFRAMEEND_MASK |
			NRF_NFCT_INT_TXFRAMEEND_MASK |
			NRF_NFCT_INT_ERROR_MASK,
		.cb = nrfx_nfct_evt_handler
	};

	nfc_platform_setup();

	uint8_t uid[7] = {0x5F, 0x22, 0x39, 0xBE, 0xEC, 0x05, 0x38};
	nrfx_nfct_param_t uid_param = {
		.id = NRFX_NFCT_PARAM_ID_NFCID1,
		.data.nfcid1 = {
			.id_size = NRFX_NFCT_NFCID1_DOUBLE_SIZE,
			.p_id = uid,
		},
	};

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

/*
TODO:
-software parity (odd, one bit after each byte, e.g. |00010000|0|)
-software CRC, polynomial is 0x8408, initial value is 0x6363.
*/

static inline bool odd_parity8(uint8_t a) {
	return (1 ^ (a << 0)^ (a << 1)^ (a << 2)^ (a << 3)^ (a << 4)^ (a << 5)^ (a << 6)^ (a << 7))&1;
}

size_t add_parity_and_crc(uint8_t *in, size_t in_len, uint8_t *out) {
	uint16_t crc = crc16_ccitt(0x6363, in, in_len);
	uint8_t crc_buf[2] = {crc & 0xFF, (crc >> 8) & 0xFF };

	size_t offset_bits = 0;
	size_t offset_bytes = 0;

	size_t result_bitcount = (8 + 1) * (in_len + 2); // (byte + parity) * (data + crc)

	for (size_t i = 0; i < in_len; ++i) {
		out[offset_bytes]   |= (in[i] << offset_bits) & 0xFF;
		out[offset_bytes+1] |= (in[i] >> (8-offset_bits)) & 0xFF;
		out[offset_bytes+1] |= odd_parity8(in[i]) << offset_bits;

		offset_bits += 9;
		offset_bytes += offset_bits >> 3;
		offset_bits = offset_bits & 0b111;
	}
	for (size_t i = 0; i < ARRAY_SIZE(crc_buf); ++i) {
		out[offset_bytes]   |= (crc_buf[i] << offset_bits) & 0xFF;
		out[offset_bytes+1] |= (crc_buf[i] >> (8-offset_bits)) & 0xFF;
		out[offset_bytes+1] |= odd_parity8(crc_buf[i]) << offset_bits;

		offset_bits += 9;
		offset_bytes += offset_bits >> 3;
		offset_bits = offset_bits & 0b111;
	}

	return result_bitcount;
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

	printk("size of internal buffer: %02X\n", ARRAY_SIZE(internal_buffer));

	/* Start NFC */
	if (start_nfc() < 0) {
		printk("ERROR: NFC configuration failed\n");
		return;
	}

	uint8_t test_buf[] = {0, 0};
	uint8_t test_out_buf[5] = {0};
	add_parity_and_crc(test_buf, 2, test_out_buf);
	printk("foo: %02X %02X %02X %02X %02X\n",
		test_out_buf[0],
		test_out_buf[1],
		test_out_buf[2],
		test_out_buf[3],
		test_out_buf[4]);
	//printk("crc: 0x%04X\n", crc16_ccitt(0x6363, test_buf, ARRAY_SIZE(test_buf)));

	/* Prevent deep sleep (system off) from being entered */
	pm_policy_state_lock_get(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);	

	/* Exit main function - the rest will be done by the callbacks */
}
