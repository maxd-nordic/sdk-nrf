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

#define CUSTOM_SEC_TAG 42

static const char * ca_cert =
"-----BEGIN CERTIFICATE-----\n"
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
"rqXRfboQnoZsG4q5WTP468SQvvG5\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIBjzCCATagAwIBAgIUOEakGUS/7BfSlprkly7UK43ZAwowCgYIKoZIzj0EAwIw\n"
"FDESMBAGA1UEAwwJblJGIENsb3VkMB4XDTIzMDUyNDEyMzUzMloXDTQ4MTIzMDEy\n"
"MzUzMlowFDESMBAGA1UEAwwJblJGIENsb3VkMFkwEwYHKoZIzj0CAQYIKoZIzj0D\n"
"AQcDQgAEPVmJXT4TA1ljMcbPH0hxlzMDiPX73FHsdGM/6mqAwq9m2Nunr5/gTQQF\n"
"MBUZJaQ/rUycLmrT8i+NZ0f/OzoFsKNmMGQwHQYDVR0OBBYEFGusC7QaV825v0Ci\n"
"qEv2m1HhiScSMB8GA1UdIwQYMBaAFGusC7QaV825v0CiqEv2m1HhiScSMBIGA1Ud\n"
"EwEB/wQIMAYBAf8CAQAwDgYDVR0PAQH/BAQDAgGGMAoGCCqGSM49BAMCA0cAMEQC\n"
"IH/C3yf5aNFSFlm44CoP5P8L9aW/5woNrzN/kU5I+H38AiAwiHYlPclp25LgY8e2\n"
"n7e2W/H1LXJ7S3ENDBwKUF4qyw==\n"
"-----END CERTIFICATE-----\n";

static const char * prv_key =
"-----BEGIN PRIVATE KEY-----\n"
"MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgVcSG8Qb7Btybiz0/\n"
"WMNa6vHDaJ/NVVV/UnUdfpyfxxuhRANCAASTMkgn4ke0ANPntBtPirpLnFijUT2H\n"
"HV9KzicOzVR56XaYRNe6VFmI9sxXzyJSaTElnGzHW6x47bd81RyH/9g3\n"
"-----END PRIVATE KEY-----\n";

static const char * client_cert =
"-----BEGIN CERTIFICATE-----\n"
"MIIBsjCCAVkCFAlU3YIj1LJ6Xt0WA557pyQQhulWMAoGCCqGSM49BAMCMIGMMQsw\n"
"CQYDVQQGEwJOTzESMBAGA1UEBwwJVHJvbmRoZWltMRMwEQYDVQQKDApOb3JkaWNT\n"
"ZW1pMQswCQYDVQQLDAJSRDEXMBUGA1UEAwwObm9yZGljc2VtaS5jb20xLjAsBgkq\n"
"hkiG9w0BCQEWH21heGltaWxpYW4uZGV1YmVsQG5vcmRpY3NlbWkubm8wHhcNMjUw\n"
"MjE4MTQzMjM0WhcNMzUwMjE2MTQzMjM0WjArMQswCQYDVQQGEwJOTzEcMBoGA1UE\n"
"AwwTbnJmLTM1ODI5OTg0MDEyMzQ1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IA\n"
"BJMySCfiR7QA0+e0G0+KukucWKNRPYcdX0rOJw7NVHnpdphE17pUWYj2zFfPIlJp\n"
"MSWcbMdbrHjtt3zVHIf/2DcwCgYIKoZIzj0EAwIDRwAwRAIgNbcpDiKKV/P/AOYL\n"
"MUGamb15QqrZs5DEXLWgafucBVkCIBw1Z+9XoORKPfQRajhNvF4sDWu4ocSxQHEs\n"
"T9pOx9j2\n"
"-----END CERTIFICATE-----\n";

static const char * expected_jwt =
"eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiJ9.eyJzdWIiOiJucmYtMzU4Mjk5ODQwMTIzNDU2IiwiZXhwIjoxNzM5ODgxMjkwNTE2fQ.AtaZ6Qop5ZQgor9F8zW5gekchg4L_3Aa5BZZO5UxQfFDy_dSt86haxcPWhmXKGBzYtl8H3_Ql3eDpfzH2aSGfQ";

int tls_credential_get(sec_tag_t sec_tag, enum tls_credential_type cred_type,
		       void *buf, size_t *len)
{
	switch (sec_tag) {
	case CUSTOM_SEC_TAG:
		if (cred_type == TLS_CREDENTIAL_CA_CERTIFICATE) {
			memcpy(buf, ca_cert, strlen(ca_cert));
			*len = strlen(ca_cert);
			return 0;
		} else if (cred_type == TLS_CREDENTIAL_SERVER_CERTIFICATE) {
			memcpy(buf, client_cert, strlen(client_cert));
			*len = strlen(client_cert);
			return 0;
		} else if (cred_type == TLS_CREDENTIAL_PRIVATE_KEY) {
			memcpy(buf, prv_key, strlen(prv_key));
			*len = strlen(prv_key);
			return 0;
		} else {
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void *calloc_list[1024] = {0};

void nrf_cloud_free(void *ptr)
{
	for (size_t i = 0; i < ARRAY_SIZE(calloc_list); i++) {
		if (calloc_list[i] == ptr) {
			calloc_list[i] = NULL;
			return;
		}
	}
	printk("Failed to find pointer to free\n");
}

void * nrf_cloud_calloc(size_t count, size_t size)
{
	for (size_t i = 0; i < ARRAY_SIZE(calloc_list); i++) {
		if (calloc_list[i] == NULL) {
			calloc_list[i] = k_calloc(count, size);
			return calloc_list[i];
		}
	}
	printk("Failed to find empty slot to calloc\n");
	return NULL;
}

int nrf_cloud_client_id_ptr_get(const char **client_id)
{
	static const char client_id_buf[] = "nrf-358299840123456";

	if (!client_id) {
		return -EINVAL;
	}

	*client_id = client_id_buf;

	return 0;
}

int date_time_now(int64_t *timestamp)
{
	*timestamp = 1739881289916;
	return 0;
}

ZTEST(jwt_app_test, test_1)
{
	uint8_t buf[1024];

	nrf_cloud_sec_tag_get_fake.return_val = CUSTOM_SEC_TAG;

	int err = nrf_cloud_jwt_generate(0, buf, sizeof(buf));

	zassert_equal(err, 0, "err: %d", err);
	zassert_equal(strcmp(buf, expected_jwt), 0, "buf: %s", buf);
}

static void *setup(void)
{
	/* reset fakes */
	RESET_FAKE(nrf_cloud_sec_tag_get);

	for (size_t i = 0; i < ARRAY_SIZE(calloc_list); i++) {
		if (calloc_list[i] != NULL) {
			k_free(calloc_list[i]);
		}
		calloc_list[i] = NULL;
	}

	return NULL;
}

static void teardown(void *unit)
{
	(void)unit;
}

ZTEST_SUITE(jwt_app_test, NULL, setup, teardown, NULL, NULL);
