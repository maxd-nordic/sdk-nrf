#include "bsec2_internal.h"
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bsec_lib_pers, CONFIG_BSEC_LIB_LOG_LEVEL);

/* Definitions used to store and retrieve BSEC state from the settings API */
#define SETTINGS_NAME_BSEC "bsec"
#define SETTINGS_KEY_STATE "state"
#define SETTINGS_BSEC_STATE SETTINGS_NAME_BSEC "/" SETTINGS_KEY_STATE

static size_t state_length;

void bsec_state_save(uint8_t work_buffer, size_t work_buffer_len)
{
	int ret;
	uint8_t state_buffer[BSEC_MAX_STATE_BLOB_SIZE];

	ret = bsec_get_state(0, state_buffer, ARRAY_SIZE(state_buffer),
				work_buffer, work_buffer_len,
				&state_length);

	__ASSERT(ret == BSEC_OK, "bsec_get_state failed.");
	__ASSERT(state_length <= ARRAY_SIZE(state_buffer), "state buffer too big to save.");

	ret = settings_save_one(SETTINGS_BSEC_STATE, state_buffer, state_length);

	__ASSERT(ret == 0, "storing state to flash failed.");
}

static ssize_t bsec_state_read_cb(const char *key, size_t len,
					       settings_read_cb read_cb,
					       void *cb_arg, void *param)
{
	uint8_t *state_buffer = param;
	ARG_UNUSED(key);

	if (len == 0) {
		return 0;
	}
	if (len > BSEC_MAX_STATE_BLOB_SIZE) {
		LOG_ERR("invalid bsec state size");
		return -EINVAL;
	}

	state_length = read_cb(cb_arg, state_buffer, BSEC_MAX_STATE_BLOB_SIZE);

	if (state_length < len) {
		LOG_ERR("Settings error: entry incomplete");
		return -ENODATA;
	}

	return 0;
}

void bsec_state_load(uint8_t work_buffer, size_t work_buffer_len)
{
	int ret;
	uint8_t state_buffer[BSEC_MAX_STATE_BLOB_SIZE];

	ret = settings_load_subtree_direct(SETTINGS_BSEC_STATE, bsec_state_read_cb, state_buffer);
	if (ret) {
		LOG_ERR("Failed to read stored BSEC state: %d", ret);
		return;
	}

	ret = bsec_set_state(state_buffer, state_length,
			     work_buffer, work_buffer_len);
	if (ret != BSEC_OK && ret != BSEC_E_CONFIG_EMPTY) {
		LOG_ERR("Failed to set BSEC state: %d", ret);
	} else if (ret == BSEC_OK) {
		LOG_DBG("Setting BSEC state successful.");
	}
}
