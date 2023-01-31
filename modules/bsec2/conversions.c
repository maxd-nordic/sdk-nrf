// this converts data between bme68x and bsec

#include <zephyr/kernel.h>
#include "bsec2_internal.h"s

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bsec_lib_conv, CONFIG_BSEC_LIB_LOG_LEVEL);

#define BSEC_TOTAL_HEAT_DUR		UINT16_C(140)
#define BSEC_INPUT_PRESENT(x, shift)	(x.process_data & (1 << (shift - 1)))

/* Temperature offset due to external heat sources. */
float temp_offset = (CONFIG_EXTERNAL_SENSORS_BSEC_TEMPERATURE_OFFSET / (float)100);

size_t bme68x_data_to_bsec_inputs(bsec_bme_settings_t sensor_settings,
					 const struct bme68x_data *data,
					 bsec_input_t *inputs, uint64_t timestamp_ns)
{
	size_t i = 0;

	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_TEMPERATURE)) {
		inputs[i].sensor_id = BSEC_INPUT_HEATSOURCE;
		inputs[i].signal = temp_offset;
		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Temp offset: %.2f", inputs[i].signal);
		i++;
		inputs[i].sensor_id = BSEC_INPUT_TEMPERATURE;
		inputs[i].signal = data->temperature;

		if (!IS_ENABLED(BME68X_USE_FPU)) {
			inputs[i].signal /= 100.0f;
		}

		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Temp: %.2f", inputs[i].signal);
		i++;
	}
	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_HUMIDITY)) {
		inputs[i].sensor_id = BSEC_INPUT_HUMIDITY;
		inputs[i].signal =  data->humidity;

		if (!IS_ENABLED(BME68X_USE_FPU)) {
			inputs[i].signal /= 1000.0f;
		}

		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Hum: %.2f", inputs[i].signal);
		i++;
	}
	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_PRESSURE)) {
		inputs[i].sensor_id = BSEC_INPUT_PRESSURE;
		inputs[i].signal =  data->pressure;
		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Press: %.2f", inputs[i].signal);
		i++;
	}
	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_GASRESISTOR)) {
		inputs[i].sensor_id = BSEC_INPUT_GASRESISTOR;
		inputs[i].signal =  data->gas_resistance;
		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Gas: %.2f", inputs[i].signal);
		i++;
	}
	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_PROFILE_PART)) {
		inputs[i].sensor_id = BSEC_INPUT_PROFILE_PART;
		if (sensor_settings.op_mode == BME68X_FORCED_MODE) {
			inputs[i].signal =  0;
		} else {
			inputs[i].signal =  data->gas_index;
		}
		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Profile: %.2f", inputs[i].signal);
		i++;
	}
	return i;
}

int bsec_apply_sensor_settings(bsec_bme_settings_t sensor_settings)
{
	int ret;
	struct bme68x_conf config = {0};
	struct bme68x_heatr_conf heater_config = {0};

	heater_config.enable = BME68X_ENABLE;
	heater_config.heatr_temp = sensor_settings.heater_temperature;
	heater_config.heatr_dur = sensor_settings.heater_duration;
	heater_config.heatr_temp_prof = sensor_settings.heater_temperature_profile;
	heater_config.heatr_dur_prof = sensor_settings.heater_duration_profile;
	heater_config.profile_len = sensor_settings.heater_profile_len;
	heater_config.shared_heatr_dur = 0;

	switch (sensor_settings.op_mode) {
	case BME68X_PARALLEL_MODE:
		heater_config.shared_heatr_dur =
			BSEC_TOTAL_HEAT_DUR -
			(bme68x_get_meas_dur(sensor_settings.op_mode, &config, &ctx.dev)
			/ INT64_C(1000));

		/* fall through */
	case BME68X_FORCED_MODE:
		ret = bme68x_get_conf(&config, &ctx.dev);
		if (ret) {
			LOG_ERR("bme68x_get_conf err: %d", ret);
			return ret;
		}

		config.os_hum = sensor_settings.humidity_oversampling;
		config.os_temp = sensor_settings.temperature_oversampling;
		config.os_pres = sensor_settings.pressure_oversampling;

		ret = bme68x_set_conf(&config, &ctx.dev);
		if (ret) {
			LOG_ERR("bme68x_set_conf err: %d", ret);
			return ret;
		}

		bme68x_set_heatr_conf(sensor_settings.op_mode,
					&heater_config, &ctx.dev);

		/* fall through */
	case BME68X_SLEEP_MODE:
		ret = bme68x_set_op_mode(sensor_settings.op_mode, &ctx.dev);
		if (ret) {
			LOG_ERR("bme68x_set_op_mode err: %d", ret);
			return ret;
		}
		break;
	default:
		LOG_ERR("unknown op mode: %d", sensor_settings.op_mode);
	}
	return 0;
}
