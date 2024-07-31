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
#include <zephyr/device.h>
#include <nrfx_twim.h>
#include <nrfx_spim.h>
#include <nrfx_gpiote.h>

struct i2c_nrfx_twim_config {
	nrfx_twim_t twim;
	nrfx_twim_config_t twim_config;
	uint16_t msg_buf_size;
	void (*irq_connect)(void);
	const struct pinctrl_dev_config *pcfg;
	uint16_t max_transfer_size;
};

struct spi_nrfx_config {
	nrfx_spim_t	   spim;
	uint32_t	   max_freq;
	nrfx_spim_config_t def_config;
	void (*irq_connect)(void);
	uint16_t max_chunk_len;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58
	bool anomaly_58_workaround;
#endif
	uint32_t wake_pin;
	nrfx_gpiote_t wake_gpiote;
#ifdef CONFIG_DCACHE
	uint32_t mem_attr;
#endif
};

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
const struct device *i2c2 = DEVICE_DT_GET(DT_NODELABEL(i2c2));
const struct device *spi3 = DEVICE_DT_GET(DT_NODELABEL(spi3));

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

	const struct i2c_nrfx_twim_config *i2c_dev_config = i2c2->config;
	nrfx_twim_disable(&i2c_dev_config->twim);

	const struct spi_nrfx_config *spi_dev_config = spi3->config;
	nrfx_spim_uninit(&spi_dev_config->spim);

	err = gpio_pin_configure(gpio0,  8, GPIO_INPUT); CHECKERR; // SCL
	err = gpio_pin_configure(gpio0,  9, GPIO_INPUT); CHECKERR; // SDA
	err = gpio_pin_configure(gpio0, 13, GPIO_INPUT); CHECKERR; // SCK
	err = gpio_pin_configure(gpio0, 14, GPIO_INPUT); CHECKERR; // MOSI
	err = gpio_pin_configure(gpio0, 15, GPIO_INPUT); CHECKERR; // MISO
	err = gpio_pin_configure(gpio0, 12, GPIO_INPUT); CHECKERR; // GD25LE255E CS
	err = gpio_pin_configure(gpio0, 17, GPIO_INPUT); CHECKERR; // nRF7002 CS
	err = gpio_pin_configure(gpio0, 10, GPIO_INPUT); CHECKERR; // BMI270 CS

	err = gpio_pin_configure(gpio0, 31, GPIO_OUTPUT_ACTIVE); CHECKERR; // LED_GREEN
	err = gpio_pin_configure(gpio0, 22, GPIO_OUTPUT_INACTIVE); CHECKERR; // GPIO1 -> nRF53

	printk("I2C/SPI buses released\n");

	printk("Ready\n");

	return 0;
}
