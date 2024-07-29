/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/gpio.h>

/* To strictly comply with UART timing, enable external XTAL oscillator */
void enable_xtal(void)
{
	struct onoff_manager *clk_mgr;
	static struct onoff_client cli = {};

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	sys_notify_init_spinwait(&cli.notify);
	(void)onoff_request(clk_mgr, &cli);
}

const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));

#define CHECKERR if (err) {printk("GPIO error: %d\n", err); return err;}

int main(void)
{
	int err;

	printk("The AT host sample started\n");

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, error: %d\n", err);
		return 0;
	}
	enable_xtal();

	printk("Releasing I2C/SPI buses\n");

	err = gpio_pin_configure(gpio0, 29, GPIO_OUTPUT_ACTIVE); CHECKERR; // LED_RED

	err = gpio_pin_configure(gpio0,  8, GPIO_DISCONNECTED); CHECKERR; // SCL
	err = gpio_pin_configure(gpio0,  9, GPIO_DISCONNECTED); CHECKERR; // SDA

	err = gpio_pin_configure(gpio0, 13, GPIO_DISCONNECTED); CHECKERR; // SCK
	err = gpio_pin_configure(gpio0, 14, GPIO_DISCONNECTED); CHECKERR; // MOSI
	err = gpio_pin_configure(gpio0, 15, GPIO_DISCONNECTED); CHECKERR; // MISO

	err = gpio_pin_configure(gpio0, 12, GPIO_DISCONNECTED); CHECKERR; // GD25LE255E CS
	err = gpio_pin_configure(gpio0, 17, GPIO_DISCONNECTED); CHECKERR; // nRF7002 CS
	err = gpio_pin_configure(gpio0, 10, GPIO_DISCONNECTED); CHECKERR; // BMI270 CS

	err = gpio_pin_configure(gpio0, 29, GPIO_OUTPUT_INACTIVE); CHECKERR; // LED_RED
	err = gpio_pin_configure(gpio0, 31, GPIO_OUTPUT_ACTIVE); CHECKERR; // LED_GREEN
	err = gpio_pin_configure(gpio0, 22, GPIO_OUTPUT_INACTIVE); CHECKERR; // GPIO1 -> nRF53

	printk("I2C/SPI buses released\n");

	printk("Ready\n");

	return 0;
}
