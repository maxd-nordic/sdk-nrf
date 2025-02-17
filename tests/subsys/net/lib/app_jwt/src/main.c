/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <net/nrf_cloud.h>
#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(nrf_cloud_free, void *);
FAKE_VALUE_FUNC(sec_tag_t, nrf_cloud_sec_tag_get);
FAKE_VALUE_FUNC(int, nrf_cloud_client_id_ptr_get, const char **);
FAKE_VALUE_FUNC(int, date_time_now, int64_t *);
FAKE_VALUE_FUNC(void *, nrf_cloud_calloc, size_t, size_t);
FAKE_VALUE_FUNC(int, tls_credential_get, sec_tag_t, enum tls_credential_type, void *, size_t *);
FAKE_VALUE_FUNC(int, base64_encode, uint8_t *, size_t, size_t *, const uint8_t *, size_t);
FAKE_VALUE_FUNC(int, base64_decode, uint8_t *, size_t, size_t *, const uint8_t *, size_t);

ZTEST(jwt_app_test, test_1)
{
	uint8_t buf[1024];
	int err = nrf_cloud_jwt_generate(0, buf, sizeof(buf));

	zassert_equal(err, 0, "err: %d", err);
}

static void *setup(void)
{
	/* reset fakes */
	RESET_FAKE(nrf_cloud_free);
	RESET_FAKE(nrf_cloud_sec_tag_get);
	RESET_FAKE(nrf_cloud_client_id_ptr_get);
	RESET_FAKE(date_time_now);
	RESET_FAKE(nrf_cloud_calloc);
	RESET_FAKE(tls_credential_get);
	RESET_FAKE(base64_encode);
	RESET_FAKE(base64_decode);
	return NULL;
}

static void teardown(void *unit)
{
	(void)unit;
}

ZTEST_SUITE(jwt_app_test, NULL, setup, teardown, NULL, NULL);
