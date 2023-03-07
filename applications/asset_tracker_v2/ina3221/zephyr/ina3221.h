/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>


#define INA3221_ADDR 0b1000000

#define INA3221_CONFIG		0x00
#define INA3221_SHUNT_V1	0x01
#define INA3221_BUS_V1		0x02
#define INA3221_SHUNT_V2	0x03
#define INA3221_BUS_V2		0x04
#define INA3221_SHUNT_V3	0x05
#define INA3221_BUS_V3		0x06

#define INA3221_MASK_ENABLE	0x0f
#define INA3221_MANUF_ID	0xfe
#define INA3221_MANUF_ID_VALUE	0x5449
#define INA3221_CHIP_ID		0xff
#define INA3221_CHIP_ID_VALUE	0x3220

#define INA3221_MASK_ENABLE_CONVERSION_READY	BIT(0)
#define INA3221_CONFIG_RST			BIT(15)
#define INA3221_CONFIG_CH1			BIT(14)
#define INA3221_CONFIG_CH2			BIT(13)
#define INA3221_CONFIG_CH3			BIT(12)
#define INA3221_CONFIG_AVG_Msk			GENMASK(11, 9)
#define INA3221_CONFIG_AVG_1			(0)
#define INA3221_CONFIG_AVG_4			(0b001 << 9)
#define INA3221_CONFIG_AVG_16			(0b010 << 9)
#define INA3221_CONFIG_AVG_1024			(0b111 << 9)
#define INA3221_CONFIG_CT_VBUS_Msk		GENMASK(8, 6)
#define INA3221_CONFIG_CT_VBUS_1_1ms		(0b100 << 6)
#define INA3221_CONFIG_CT_VBUS_588us		(0b011 << 6)
#define INA3221_CONFIG_CT_VBUS_8_244ms		(0b111 << 6)
#define INA3221_CONFIG_CT_VSH_Msk		GENMASK(5, 3)
#define INA3221_CONFIG_CT_VSH_1_1ms		(0b100 << 3)
#define INA3221_CONFIG_CT_VSH_588us		(0b011 << 3)
#define INA3221_CONFIG_CT_VSH_8_244ms		(0b111 << 3)

//default is continuous measurement of everything
#define INA3221_CONFIG_CONTINUOUS		BIT(2)
#define INA3221_CONFIG_BUS			BIT(1)
#define INA3221_CONFIG_SHUNT			BIT(0)

#define INA3221_SHUNT_RESISTOR_VALUE1		4.7
#define INA3221_SHUNT_RESISTOR_VALUE2		1.0

int ina3221_init(void);
int ina3221_start_measurement(void);
bool ina3221_measurement_ready(void);
int ina3221_get_channel(int channel, float *voltage, float *current);
int ina3221_get_voltage(int channel, float *voltage);
int ina3221_get_current(int channel, float *current);
