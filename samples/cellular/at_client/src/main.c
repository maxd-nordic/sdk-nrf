/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <modem/modem_key_mgmt.h>
#include "mbedtls/x509_crt.h"
#include "mbedtls/error.h"

/* To strictly comply with UART timing, enable external XTAL oscillator */
void enable_xtal(void)
{
	struct onoff_manager *clk_mgr;
	static struct onoff_client cli = {};

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	sys_notify_init_spinwait(&cli.notify);
	(void)onoff_request(clk_mgr, &cli);
}

#define SEC_TAG 42
uint8_t client_cert_buf[1024];

// Helper function to extract and print the Common Name
static void print_cn(const mbedtls_x509_name *name, const char *prefix)
{
    const char oid_cn[] = {0x55, 0x04, 0x03}; // OID for CommonName (2.5.4.3)
    const mbedtls_x509_name *cn_entry;

    // Look for the CN entry in the name list
    for (cn_entry = name; cn_entry != NULL; cn_entry = cn_entry->next) {
        if (cn_entry->oid.len == sizeof(oid_cn) &&
            memcmp(cn_entry->oid.p, oid_cn, sizeof(oid_cn)) == 0) {

            char cn_buf[256];
            size_t cn_len = cn_entry->val.len;

            // Make sure we don't overflow our buffer
            if (cn_len > sizeof(cn_buf) - 1) {
                cn_len = sizeof(cn_buf) - 1;
            }

            memcpy(cn_buf, cn_entry->val.p, cn_len);
            cn_buf[cn_len] = '\0';

            printf("%s Common Name: %s\n", prefix, cn_buf);
        }
    }
}

// gathers the client ID of the given sec tag and prints the CN which is the client ID
void print_client_id(void)
{
	int err;
	uint8_t client_id[64];
	size_t client_id_len = sizeof(client_id);
	mbedtls_x509_crt cert;

	err = modem_key_mgmt_read(SEC_TAG,
			MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
			client_cert_buf, &client_id_len);
	if (err) {
		printk("Failed to read client cert, error: %d\n", err);
		return;
	}
	// Unfortunately, this is not working because it's not allowed to read out the client cert.

	err = mbedtls_x509_crt_parse(&cert, client_cert_buf, client_id_len);
	if (err) {
		printk("Failed to parse client cert, error: %d\n", err);
		return;
	}

	// Print Subject CN
	print_cn(&cert.subject, "Subject");

	// Print Issuer CN
	print_cn(&cert.issuer, "Issuer");

	// Free certificate data
	mbedtls_x509_crt_free(&cert);

	printk("Client ID: ");
	for (size_t i = 0; i < client_id_len; i++) {
		printk("%02x", client_id[i]);
	}
	printk("\n");
}

int main(void)
{
	int err;

	print_client_id();

	printk("The AT host sample started\n");

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, error: %d\n", err);
		return 0;
	}
	enable_xtal();
	printk("Ready\n");

	return 0;
}
