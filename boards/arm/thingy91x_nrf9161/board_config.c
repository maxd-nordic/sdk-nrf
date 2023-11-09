/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/regulator.h>

#include <soc.h>

LOG_MODULE_REGISTER(thingy91x_nrf9161, CONFIG_LOG_DEFAULT_LEVEL);

const struct gpio_dt_spec short_range_frontend_enable = {
	.dt_flags = DT_GPIO_HOG_FLAGS_BY_IDX(DT_NODELABEL(ldsw_rf_fe_sr_en), 0),
	.pin = DT_GPIO_HOG_PIN_BY_IDX(DT_NODELABEL(ldsw_rf_fe_sr_en), 0),
	.port = DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(ldsw_rf_fe_sr_en))),
};

static const struct device *ldsw_wifi = DEVICE_DT_GET(DT_NODELABEL(ldsw_npm60_en));


static int board_config(void)
{
	int ret = 0;

	// cut off power to wifi
	ret = regulator_disable(ldsw_wifi);
	if (ret) {
		LOG_ERR("Cannot turn off wifi ldsw, err: %d", ret);
	} else {
		LOG_DBG("wifi ldsw disabled");
	}

	if (!gpio_is_ready_dt(&short_range_frontend_enable)) {
		LOG_ERR("GPIO not available!");
	} else {
		LOG_DBG("GPIO OK");
	}

	if (gpio_pin_set_dt(&short_range_frontend_enable, 0)) {
		LOG_ERR("cannot set GPIO");
	} else {
		LOG_DBG("GPIO set ok");
	}

	return 0;
}
SYS_INIT(board_config, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
