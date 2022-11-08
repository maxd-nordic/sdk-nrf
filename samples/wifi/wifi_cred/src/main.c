/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <net/wifi_credentials.h>

#if defined(CONFIG_SETTINGS)
#include <zephyr/settings/settings.h>
#endif

#define SSID1 "looooooooooooooooooooooong ssid"
#define PSK1 "suuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuper secret"
#define SECURITY1 WIFI_SECURITY_TYPE_PSK
#define BSSID1 "abcdef"
#define SAE_PW1 NULL
#define FLAGS1 WIFI_CREDENTIALS_FLAG_BSSID

#define SSID2 "test2"
#define SECURITY2 WIFI_SECURITY_TYPE_NONE

#define SSID3 "test3"
#define PSK3 NULL
#define SECURITY3 WIFI_SECURITY_TYPE_SAE
#define BSSID3 NULL
#define SAE_PW3 "extremely secret"
#define FLAGS3 0

void main(void)
{
#if defined(CONFIG_SETTINGS)
	settings_subsys_init();
	settings_load();
#endif

	int64_t ref = k_uptime_get();

	int err;
	enum wifi_security_type security = WIFI_SECURITY_TYPE_PSK;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "abcdef";
	char psk_buf[128] = "hunter2";
	size_t psk_len = 0;
	uint32_t flags = 0;

	err = wifi_credentials_get_by_ssid_personal(SSID1, sizeof(SSID1), &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	printk("get: %d (should be -2), elapsed_ms: %lld\n", err, k_uptime_delta(&ref));
	ref = k_uptime_get();

	err = wifi_credentials_get_by_ssid_personal(SSID3, sizeof(SSID3), &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	printk("get: %d (should be 0 after reboot), elapsed_ms: %lld\n", err, k_uptime_delta(&ref));
	ref = k_uptime_get();

	err = wifi_credentials_set_personal(SSID1, sizeof(SSID1), SECURITY1, BSSID1, 6,
					    PSK1, sizeof(PSK1), FLAGS1);

	printk("set: %d (should be 0), elapsed_ms: %lld\n", err, k_uptime_delta(&ref));
	ref = k_uptime_get();
	err = wifi_credentials_get_by_ssid_personal(SSID1, sizeof(SSID1), &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	printk("get: %d (should be 0), elapsed_ms: %lld\n", err, k_uptime_delta(&ref));
	ref = k_uptime_get();

	err = wifi_credentials_delete_by_ssid(SSID1, sizeof(SSID1));
	printk("delete: %d (should be 0), elapsed_ms: %lld\n", err, k_uptime_delta(&ref));
	ref = k_uptime_get();

	err = wifi_credentials_set_personal(SSID3, sizeof(SSID3), SECURITY3, BSSID3, 0,
					SAE_PW3, sizeof(SAE_PW3), FLAGS3);
	printk("set: %d (should be 0), elapsed_ms: %lld\n", err, k_uptime_delta(&ref));

	while (true) {
	}
}
