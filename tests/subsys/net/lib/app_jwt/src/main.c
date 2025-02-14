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

FAKE_VALUE_FUNC(sec_tag_t, nrf_cloud_sec_tag_get);


ZTEST(jwt_app_test, test_1)
{
	uint8_t buf[1024];
	int err = nrf_cloud_jwt_generate(0, buf, sizeof(buf));

	zassert_equal(err, 0, "expect true");
}

ZTEST_SUITE(jwt_app_test, NULL, NULL, NULL, NULL, NULL);
