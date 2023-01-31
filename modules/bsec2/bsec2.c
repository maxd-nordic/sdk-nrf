// this contains the library's thread, init and so on

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include "bsec2_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bsec_lib, CONFIG_BSEC_LIB_LOG_LEVEL);

static struct bme68x_dev dev;
/* some RAM space needed by bsec_get_state and bsec_set_state for (de-)serialization. */
static uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE];

/* Stack size of internal BSEC thread. */
#define BSEC_STACK_SIZE CONFIG_EXTERNAL_SENSORS_BSEC_THREAD_STACK_SIZE
static K_THREAD_STACK_DEFINE(thread_stack, BSEC_STACK_SIZE);
static struct k_thread thread;

/* Used for a timeout for when BSEC's state should be saved. */
static K_TIMER_DEFINE(bsec_save_state_timer, NULL, NULL);

/* Define which sensor values to request.
 * The order is not important, but output_ready needs to be updated if different types
 * of sensor values are requested.
 */
static const bsec_sensor_configuration_t requested_virtual_sensors[4] = {
	{
		.sensor_id   = BSEC_OUTPUT_IAQ,
		.sample_rate = BSEC_SAMPLE_RATE,
	},
	{
		.sensor_id   = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
		.sample_rate = BSEC_SAMPLE_RATE,
	},
	{
		.sensor_id   = BSEC_OUTPUT_RAW_PRESSURE,
		.sample_rate = BSEC_SAMPLE_RATE,
	},
	{
		.sensor_id   = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
		.sample_rate = BSEC_SAMPLE_RATE,
	},
};

static void bsec_thread_fn(void)
{
	int ret;
	struct bme68x_data sensor_data[3] = {0};
	bsec_bme_settings_t sensor_settings = {0};
	bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR] = {0};
	bsec_output_t outputs[ARRAY_SIZE(requested_virtual_sensors)] = {0};
	uint8_t n_fields = 0;
	uint8_t n_outputs = 0;
	uint8_t n_inputs = 0;

	while (true) {
		uint64_t timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());

		if (timestamp_ns < sensor_settings.next_call) {
			LOG_DBG("bsec_sensor_control not ready yet");
			k_sleep(K_NSEC(sensor_settings.next_call - timestamp_ns));
			continue;
		}

		memset(&sensor_settings, 0, sizeof(sensor_settings));
		ret = bsec_sensor_control((int64_t)timestamp_ns, &sensor_settings);
		if (ret != BSEC_OK) {
			LOG_ERR("bsec_sensor_control err: %d", ret);
			continue;
		}

		if (bsec_apply_sensor_settings(sensor_settings)) {
			continue;
		}

		if (sensor_settings.trigger_measurement &&
		    sensor_settings.op_mode != BME68X_SLEEP_MODE) {
			ret = bme68x_get_data(sensor_settings.op_mode,
					      sensor_data, &n_fields, &dev);
			if (ret) {
				LOG_DBG("bme68x_get_data err: %d", ret);
				continue;
			}

			for (size_t i = 0; i < n_fields; ++i) {
				n_outputs = ARRAY_SIZE(requested_virtual_sensors);
				n_inputs = bme68x_data_to_bsec_inputs(sensor_settings,
								      &sensor_data[i],
								      inputs, timestamp_ns);

				if (n_inputs > 0) {
					ret = bsec_do_steps(inputs, n_inputs, outputs, &n_outputs);
					if (ret != BSEC_OK) {
						LOG_ERR("bsec_do_steps err: %d", ret);
						continue;
					}
					output_ready(outputs, n_outputs);
				}
			}

		}

		/* if save timer is expired, save and restart timer */
		if (k_timer_remaining_get(&bsec_save_state_timer) == 0) {
			state_save();
			k_timer_start(&bsec_save_state_timer,
				      K_SECONDS(BSEC_SAVE_INTERVAL_S),
				      K_NO_WAIT);
		}

		k_sleep(K_SECONDS(BSEC_SAMPLE_PERIOD_S));
	}

}

int bsec_lib_init(void)
{
	int err;
	bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
	uint8_t n_required_sensor_settings;

	err = enable_settings();
	if (err) {
		LOG_ERR("enable_settings, error: %d", err);
		return err;
	}

	err = bsec_bme68x_init(&dev);
	if (err) {
		LOG_ERR("bsec_bme68x_init failed: %d", err);
		return err;
	}

	err = bsec_init();
	if (err != BSEC_OK) {
		LOG_ERR("Failed to init BSEC: %d", err);
		return err;
	}

	bsec_state_load(work_buffer, ARRAY_SIZE(work_buffer));

	bsec_update_subscription(requested_virtual_sensors, ARRAY_SIZE(requested_virtual_sensors),
				 required_sensor_settings, &n_required_sensor_settings);

	k_thread_create(&thread,
			thread_stack,
			BSEC_STACK_SIZE,
			(k_thread_entry_t)bsec_thread_fn,
			NULL, NULL, NULL, K_HIGHEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	k_timer_start(&bsec_save_state_timer,
		      K_SECONDS(BSEC_SAVE_INTERVAL_S),
		      K_NO_WAIT);

	return 0;

}
