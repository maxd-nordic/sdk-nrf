// implements i2c bus_communication between bme68x and device
// alias bme68x for the device

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

#include "bsec2_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bsec_lib_bus, CONFIG_BSEC_LIB_LOG_LEVEL);

const static struct i2c_dt_spec bme680_i2c_dev = I2C_DT_SPEC_GET(DT_ALIAS(bme68x));

static int8_t bus_write(uint8_t reg_addr, const uint8_t *reg_data_ptr, uint32_t len, void *intf_ptr)
{
	uint8_t buf[len + 1];

	buf[0] = reg_addr;
	memcpy(&buf[1], reg_data_ptr, len);

	return i2c_write_dt(&bme680_i2c_dev, buf, len + 1);
}

static int8_t bus_read(uint8_t reg_addr, uint8_t *reg_data_ptr, uint32_t len, void *intf_ptr)
{
	return i2c_write_read_dt(&bme680_i2c_dev, &reg_addr, 1, reg_data_ptr, len);
}

static void delay_us(uint32_t period, void *intf_ptr)
{
	k_usleep((int32_t) period);
}

int bsec_bme68x_init(struct bme68x_dev *dev)
{
	if (!device_is_ready(bme680_i2c_dev.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	dev->intf = BME68X_I2C_INTF;
	dev->intf_ptr = NULL;
	dev->read = bus_read;
	dev->write = bus_write;
	dev->delay_us = delay_us;
	dev->amb_temp = CONFIG_BSEC_LIB_AMBIENT_TEMPERATURE / 100;

	return bme68x_init(&ctx.dev);
}
