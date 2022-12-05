/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <assert.h>

#include "ina3221.h"

LOG_MODULE_REGISTER(ina3221, CONFIG_LED_CONTROL_LOG_LEVEL);

static struct i2c_dt_spec bus = {
	.bus = DEVICE_DT_GET(DT_NODELABEL(i2c2)),
	.addr = INA3221_ADDR,
};

static int reg_read(uint8_t reg_addr, uint16_t *reg_data)
{
	uint8_t rx_buf[2];
	int rc;

	rc = i2c_write_read_dt(&bus,
			&reg_addr, sizeof(reg_addr),
			rx_buf, sizeof(rx_buf));

	*reg_data = sys_get_be16(rx_buf);

	return rc;
}

static int reg_write(uint8_t addr, uint16_t reg_data)
{
	uint8_t tx_buf[3];

	tx_buf[0] = addr;
	sys_put_be16(reg_data, &tx_buf[1]);

	return i2c_write_dt(&bus, tx_buf, sizeof(tx_buf));
}

int ina3221_init(void)
{
	int ret;
	uint16_t reg_data;

	/* check bus */
	if (!device_is_ready(bus.bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	/* check connection */
	ret = reg_read(INA3221_MANUF_ID, &reg_data);
	if (ret) {
		LOG_ERR("No answer from sensor.");
		return ret;
	}
	if (INA3221_MANUF_ID_VALUE != reg_data) {
		LOG_ERR("Unexpected manufacturer ID: 0x%04x", reg_data);
		return -EFAULT;
	}
	ret = reg_read(INA3221_CHIP_ID, &reg_data);
	if (ret) {
		return ret;
	}
	if (INA3221_CHIP_ID_VALUE != reg_data) {
		LOG_ERR("Unexpected chip ID: 0x%04x", reg_data);
		return -EFAULT;
	}

	/* reset */
	ret = reg_read(INA3221_CONFIG, &reg_data);
	if (ret) {
		return ret;
	}
	reg_data |= INA3221_CONFIG_RST;
	ret = reg_write(INA3221_CONFIG, reg_data);
	if (ret) {
		return ret;
	}

	/* configure */
	reg_data = INA3221_CONFIG_SHUNT |
		   INA3221_CONFIG_BUS |
		   INA3221_CONFIG_CT_VBUS_8_244ms |
		   INA3221_CONFIG_CT_VSH_8_244ms |
		   INA3221_CONFIG_AVG_16 |
		   INA3221_CONFIG_CH1;
	
	ret = reg_write(INA3221_CONFIG, reg_data);
	if (ret) {
		return ret;
	}

	return 0;
}

int ina3221_start_measurement(void)
{
	int ret;
	uint16_t reg_data;

	ret = reg_read(INA3221_CONFIG, &reg_data);
	if (ret) {
		return ret;
	}
	ret = reg_write(INA3221_CONFIG, reg_data);
	if (ret) {
		return ret;
	}
	return 0;
}

bool ina3221_measurement_ready(void)
{
	int ret;
	uint16_t reg_data;

	ret = reg_read(INA3221_MASK_ENABLE, &reg_data);
	if (ret) {
		return false;
	}
	return reg_data & INA3221_MASK_ENABLE_CONVERSION_READY;
}

int ina3221_get_channel(int channel, float *voltage, float *current)
{
	int ret;

	ret = ina3221_get_voltage(channel, voltage);
	if (ret) {
		return ret;
	}

	ret = ina3221_get_current(channel, current);
	if (ret) {
		return ret;
	}
	return 0;
}

int ina3221_get_voltage(int channel, float *voltage)
{
	int ret;
	int16_t reg_data;

	assert(channel >= 0);
	assert(channel < 3);

	ret = reg_read(INA3221_BUS_V1+2*channel, &reg_data);
	if (ret) {
		return ret;
	}
	reg_data = (reg_data >> 3);
	*voltage = 0.008f * reg_data;

	return 0;
}

int ina3221_get_current(int channel, float *current)
{
	int ret;
	int16_t reg_data;
	float INA3221_SHUNT_RESISTOR_VALUE = 0;

	if (channel == 0) {
		INA3221_SHUNT_RESISTOR_VALUE = INA3221_SHUNT_RESISTOR_VALUE1;
	}
	if (channel == 1) {
		INA3221_SHUNT_RESISTOR_VALUE = INA3221_SHUNT_RESISTOR_VALUE2;
	}

	assert(channel >= 0);
	assert(channel < 3);

	ret = reg_read(INA3221_SHUNT_V1+2*channel, &reg_data);
	if (ret) {
		return ret;
	}
	reg_data = (reg_data >> 3);
	*current = (0.00004f * reg_data) / INA3221_SHUNT_RESISTOR_VALUE;

	return 0;
}
