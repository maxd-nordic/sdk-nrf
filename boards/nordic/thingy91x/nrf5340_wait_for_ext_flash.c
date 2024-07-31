/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util_macro.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(thingy91x_nrf5340_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);

const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
const struct device *gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));

#define CHECKERR if (err) {printk("GPIO error: %d\n", err); return err;}


static int boot_task(void)
{
	int err;

	err = gpio_pin_configure(gpio0, 30, GPIO_INPUT | GPIO_ACTIVE_LOW | GPIO_PULL_UP); CHECKERR; // GPIO1 -> nRF53

	while (gpio_pin_get(gpio0, 30) == 0) {
	 	k_sleep(K_MSEC(100));
	}
	return 0;
}

SYS_INIT(boot_task, POST_KERNEL, CONFIG_GPIO_HOGS_INIT_PRIORITY);
