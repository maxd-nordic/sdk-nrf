/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/__assert.h>
#include <drivers/bme68x_iaq.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

#if defined(CONFIG_APP_TRIGGER)
const struct sensor_trigger trig = {
	.chan = SENSOR_CHAN_ALL,
	.type = SENSOR_TRIG_TIMER,
};

int thresholds[] = {
	100,
	150,
	200,
	500,
};

bool led_states[][3] = {
	{0,1,0},
	{1,1,0},
	{1,0,0},
	{1,0,1},
};

static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	struct sensor_value temp, press, humidity, iaq;

	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
	sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);
	sensor_channel_get(dev, SENSOR_CHAN_IAQ, &iaq);

	for (size_t i = 0; i < ARRAY_SIZE(thresholds); ++i) {
		if (iaq.val1 <= thresholds[i]) {
			for(size_t j = 0; j < 3; ++j) {
				dk_set_led(j, led_states[i][j]);
			}
			break;
		}
	}

	LOG_INF("temp: %d.%06d; press: %d.%06d; humidity: %d.%06d; iaq: %d",
		temp.val1, temp.val2, press.val1, press.val2,
		humidity.val1, humidity.val2, iaq.val1);
};
#endif /* defined(CONFIG_APP_TRIGGER) */

int main(void)
{
	dk_leds_init();
	const struct device *const dev = DEVICE_DT_GET_ANY(bosch_bme680);

	LOG_INF("App started");

	k_sleep(K_SECONDS(5));

	if (dev == NULL) {
		LOG_ERR("no device found");
		return 0;
	}
	if (!device_is_ready(dev)) {
		LOG_ERR("device is not ready");
		return 0;
	}

#if defined(CONFIG_APP_TRIGGER)
	int ret = sensor_trigger_set(dev, &trig, trigger_handler);

	if (ret) {
		LOG_ERR("couldn't set trigger");
		return 0;
	}
#else
	while (1) {
		struct sensor_value temp, press, humidity, iaq;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
		sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);
		sensor_channel_get(dev, SENSOR_CHAN_IAQ, &iaq);

		LOG_INF("temp: %d.%06d; press: %d.%06d; humidity: %d.%06d; iaq: %d",
			temp.val1, temp.val2, press.val1, press.val2,
			humidity.val1, humidity.val2, iaq.val1);
		k_sleep(K_MSEC(1000));
	}
#endif /* defined(CONFIG_APP_TRIGGER) */

	return 0;
}
