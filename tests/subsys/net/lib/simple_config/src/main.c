/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "simple_config_internal.h"


#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

/* cJSON functions */
FAKE_VOID_FUNC(cJSON_Delete, cJSON *)
FAKE_VOID_FUNC(cJSON_DeleteItemFromObject, cJSON *, const char *)
FAKE_VALUE_FUNC(cJSON *, cJSON_CreateObject)
FAKE_VALUE_FUNC(cJSON *, cJSON_AddStringToObject, cJSON * const, const char * const, const char * const)
FAKE_VALUE_FUNC(cJSON *, cJSON_AddTrueToObject, cJSON * const, const char * const)
FAKE_VALUE_FUNC(cJSON *, cJSON_AddFalseToObject, cJSON * const, const char * const)
FAKE_VALUE_FUNC(cJSON *, cJSON_AddNumberToObject, cJSON * const, const char * const, const double)
FAKE_VALUE_FUNC(cJSON *, cJSON_Parse, const char *)
FAKE_VALUE_FUNC(cJSON_bool, cJSON_IsObject, const cJSON * const)
FAKE_VALUE_FUNC(cJSON *, cJSON_GetObjectItem, const cJSON * const, const char * const)

/* nRF Cloud functions */
FAKE_VALUE_FUNC(int, nrf_cloud_coap_shadow_get, char *, size_t, bool)

void setUp(void)
{}

void tearDown(void)
{
	simple_config_clear_queued_configs();
}

void test_handle_incoming_settings_no_callback(void)
{
	int err = simple_config_handle_incoming_settings(NULL, 0);
	TEST_ASSERT_EQUAL(-EFAULT, err);
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	/* use the runner from test_runner_generate() */
	(void)unity_main();

	return 0;
}
