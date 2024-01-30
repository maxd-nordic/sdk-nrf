/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include "cJSON.h"
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_coap.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

struct simple_config_val {
	enum {
		SIMPLE_CONFIG_VAL_STRING,
		SIMPLE_CONFIG_VAL_BOOL,
		SIMPLE_CONFIG_VAL_DOUBLE,
	} type;
	union {
		const char *_str;
		bool _bool;
		double _double;
	} val;
};

LOG_MODULE_REGISTER(simple_config, CONFIG_SIMPLE_CONFIG_LOG_LEVEL);

static simple_config_callback_t callback;

/* cJSON object holding all the pending settings entries. Lives forever. */
static cJSON_Object *queued_configs;

static void json_print_obj(const char *prefix, const cJSON *obj)
{
	char *string = cJSON_Print(obj);

	if (string == NULL) {
		LOG_ERR("Failed to print object content");
		return;
	}

	printk("%s%s\n", prefix, string);

	cJSON_FreeString(string);
}

typedef int (*simple_config_callback_t)(const char *key, const struct simple_config_val *val);
void simple_config_set_callback(simple_config_callback_t cb) {
	callback = cb;
}

// TODO: apply pending config changes, even if we got no valid shadow delta

int simple_config_update(void)
{
	int err = 0;
	cJSON *root_obj = NULL;
	cJSON *config_obj = NULL;
	char buf[COAP_SHADOW_MAX_SIZE] = {0};

	LOG_INF("Checking for shadow delta...");
	// TODO: do we want to initially request with delta=false?
	err = nrf_cloud_coap_shadow_get(buf, sizeof(buf), true);
	if (err == -EACCES) {
		LOG_DBG("Not connected yet.");
		return err;
	} else if (err) {
		LOG_ERR("Failed to request shadow delta: %d", err);
		return err;
	}

	LOG_DBG("Shadow delta: len:%zd, %s", in_data.len, buf);

	root_obj = cJSON_Parse(buf);
	if (root_obj == NULL) {
		LOG_ERR("Shadow delta could not be parsed");
	}
	if (!cJSON_IsObject(root_obj)) {
		LOG_ERR("Shadow delta is not an object");
		err = -ENOENT;
		goto exit;
	}

	config_obj = cJSON_GetObjectItem(root_obj, "config");
	if (config_obj) {
		cJSON* child = NULL;
		cJSON_ArrayForEach(child, config_obj) {
			LOG_DGB("Name: %s, Value: %s\n",
				child->string,
				child->valuestring);
			struct simple_config_val val;

			if (child->type == cJSON_String) {
				val.type = SIMPLE_CONFIG_VAL_STRING;
				val.val._str = child->valuestring;
			} else if (child->type == cJSON_Number) {
				val.type = SIMPLE_CONFIG_VAL_DOUBLE;
				val.val._double = child->valuedouble;
			} else if (child->type == cJSON_True) {
				val.type = SIMPLE_CONFIG_VAL_BOOL;
				val.val._bool = true;
			} else if (child->type == cJSON_False) {
				val.type = SIMPLE_CONFIG_VAL_BOOL;
				val.val._bool = false;
			} else {
				LOG_ERR("config entry %s has unsupported type!", child->string);
				/* reject entry */
				cJSON_DeleteItemFromObject(config_obj, child->string);
			}


			if (callback(child->string, val)) {
				/* callback returned nonzero value, reject entry */
				cJSON_DeleteItemFromObject(config_obj, child->string);
			}
		};
	}

	if (cJSON_PrintPreallocated(root_obj, buf, sizeof(buf), false)) {
		nrf_cloud_coap_shadow_state_update(buf);
	} else {
		LOG_ERR("Rendering delta response failed!");
	}


exit:
	if (root_obj) {
		cJSON_Delete(root_obj);
	}
	return err;
}

void simple_config_init_queued_configs(void) {
	if (queued_configs == NULL) {
		LOG_DBG("initilizing [queued_configs]");
		queued_configs = cJSON_CreateObject();
	}
	if (queued_configs == NULL) {
		LOG_ERR("[queued_configs] couldn't be created!");
	}
}

int simple_config_set(const char *key, const struct simple_config_val *val)
{
	cJSON *child = NULL;

	if (key == NULL || key[0] == '\0'
		|| (
			val->type != SIMPLE_CONFIG_VAL_STRING
			&& val->type != SIMPLE_CONFIG_VAL_BOOL
			&& val->type != SIMPLE_CONFIG_VAL_DOUBLE
		)
	) {
		return -EINVAL;
	}

	simple_config_init_queued_configs();

	/* delete config item if it already existed */
	cJSON_DeleteItemFromObject(queued_configs, key);

	if (val->type == SIMPLE_CONFIG_VAL_STRING) {
		child = cJSON_AddStringToObject(queued_configs, key, val->val._str);
	} else if (val->type == SIMPLE_CONFIG_VAL_BOOL) {
		if (val->val._bool) {
			child  = cJSON_AddTrueToObject(queued_configs, key);
		} else {
			child  = cJSON_AddFalseToObject(queued_configs, key);
		}
	} else if (val->type == SIMPLE_CONFIG_VAL_DOUBLE) {
		child = cJSON_AddNumberToObject(queued_configs, key, val->val._double);
	}

	if (child == NULL) {
		LOG_ERR("couldn't add config item for key [%s]", key);
		return -ENOMEM;
	}

	return 0;
}
