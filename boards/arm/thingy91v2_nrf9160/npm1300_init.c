/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

LOG_MODULE_REGISTER(board_secure, CONFIG_BOARD_LOG_LEVEL);

#define CHECKERR if (err) {LOG_ERR("I2C error: %d", err); return err;}

static const struct i2c_dt_spec pmic = I2C_DT_SPEC_GET(DT_NODELABEL(pmic_main));

static int pmic_write_reg(uint16_t address, uint8_t value)
{
	uint8_t buf[] = {
		address >> 8,
		address & 0xFF,
		value,
	};

	return i2c_write_dt(&pmic, buf, ARRAY_SIZE(buf));
}

static int pmic_read_reg(uint16_t address, uint8_t *value)
{
	int rc;
	uint8_t buf[] = {
		address >> 8,
		address & 0xFF,
	};

	rc = i2c_write_dt(&pmic, buf, ARRAY_SIZE(buf));
	if (rc) {
		return rc;
	}

	/* read one byte */
	return i2c_read_dt(&pmic, value, 1);
}

static int power_mgmt_init(void)
{
	int err = 0;
	uint8_t reg = 0;

	if (!device_is_ready(pmic.bus)) {
		LOG_ERR("cannot reach PMIC!");
		return -ENODEV;
	}

	// disable charger for config
	err = pmic_write_reg(0x0305, 0x03); CHECKERR;

	// set VBUS current limit 500mA 
	err = pmic_write_reg(0x0201, 0x00); CHECKERR;
	err = pmic_write_reg(0x0202, 0x00); CHECKERR;
	err = pmic_write_reg(0x0200, 0x01); CHECKERR;

#if defined(CONFIG_WIFI)
	// turn on wifi pmic
	err = pmic_write_reg(0x0800, 0x01); CHECKERR;
#else
	// turn off wifi pmic
	err = pmic_write_reg(0x0801, 0x01); CHECKERR;
#endif /* defined(CONFIG_WIFI) */
	
	// set RF switch to BLE by default
	err = pmic_write_reg(0x0601, 0x08); CHECKERR;

	// enable VDD_SENS: 
	err = pmic_write_reg(0x0802, 0x01); CHECKERR;

	// let BUCK2 be controlled by GPIO2
	err = pmic_write_reg(0x0602, 0x00); CHECKERR;
	err = pmic_write_reg(0x040C, 0x18); CHECKERR;


	// set bias resistor for 10k NTC 
	err = pmic_write_reg(0x050A, 0x01); CHECKERR;
	// set COLD threshold to 0C
	err = pmic_write_reg(0x0310, 0xbb); CHECKERR;
	err = pmic_write_reg(0x0311, 0x01); CHECKERR;
	// set COOL threshold to 10C
	err = pmic_write_reg(0x0312, 0xa4); CHECKERR;
	err = pmic_write_reg(0x0313, 0x02); CHECKERR;
	// set WARM threshold to 45C
	err = pmic_write_reg(0x0314, 0x54); CHECKERR;
	err = pmic_write_reg(0x0315, 0x01); CHECKERR;
	// set HOT threshold to 45C
	err = pmic_write_reg(0x0316, 0x54); CHECKERR;
	err = pmic_write_reg(0x0317, 0x01); CHECKERR;

	// set charging current to 800mA
	err = pmic_write_reg(0x0308, 0xc8); CHECKERR;
	err = pmic_write_reg(0x0309, 0x00); CHECKERR;
	// set charging termination voltage 4.2V 
	err = pmic_write_reg(0x030C, 0x08); CHECKERR;
	// enable charger 
	err = pmic_write_reg(0x0304, 0x03); CHECKERR;

	err = pmic_read_reg(0x0410, &reg); CHECKERR;
	if (reg != 0x8) {
		LOG_ERR("unexpected BUCK1 setting: %02X", reg);
	}

	err = pmic_read_reg(0x0411, &reg); CHECKERR;
	if (reg != 0x17) {
		LOG_ERR("unexpected BUCK2 setting: %02X", reg);
	}

	LOG_INF("PMIC configuration complete!");
	return err;
}

static int thingy91v2_board_init(void)
{
	int err;

	err = power_mgmt_init();
	if (err) {
		LOG_ERR("power_mgmt_init failed with error: %d", err);
		return err;
	}

	return 0;
}

SYS_INIT(thingy91v2_board_init, POST_KERNEL, CONFIG_BOARD_INIT_PRIORITY);
