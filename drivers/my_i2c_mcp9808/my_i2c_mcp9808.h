/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MY_I2C_MCP9808_H_
#define MY_I2C_MCP9808_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#define MY_I2C_MCP9808_REG_CONFIG		0x01
#define MY_I2C_MCP9808_REG_UPPER_LIMIT		0x02
#define MY_I2C_MCP9808_REG_LOWER_LIMIT		0x03
#define MY_I2C_MCP9808_REG_CRITICAL		0x04
#define MY_I2C_MCP9808_REG_TEMP_AMB		0x05

/* 16 bits control configuration and state.
 *
 * * Bit 0 controls alert signal output mode
 * * Bit 1 controls interrupt polarity
 * * Bit 2 disables upper and lower threshold checking
 * * Bit 3 enables alert signal output
 * * Bit 4 records alert status
 * * Bit 5 records interrupt status
 * * Bit 6 locks the upper/lower window registers
 * * Bit 7 locks the critical register
 * * Bit 8 enters shutdown mode
 * * Bits 9-10 control threshold hysteresis
 */
#define MY_I2C_MCP9808_CFG_ALERT_MODE_INT	BIT(0)
#define MY_I2C_MCP9808_CFG_ALERT_ENA		BIT(3)
#define MY_I2C_MCP9808_CFG_ALERT_STATE		BIT(4)
#define MY_I2C_MCP9808_CFG_INT_CLEAR		BIT(5)

/* 16 bits are used for temperature and state encoding:
 * * Bits 0..11 encode the temperature in a 2s complement signed value
 *   in Celsius with 1/16 Cel resolution
 * * Bit 12 is set to indicate a negative temperature
 * * Bit 13 is set to indicate a temperature below the lower threshold
 * * Bit 14 is set to indicate a temperature above the upper threshold
 * * Bit 15 is set to indicate a temperature above the critical threshold
 */
#define MY_I2C_MCP9808_TEMP_SCALE_CEL		16 /* signed */
#define MY_I2C_MCP9808_TEMP_SIGN_BIT		BIT(12)
#define MY_I2C_MCP9808_TEMP_ABS_MASK		((uint16_t)(MY_I2C_MCP9808_TEMP_SIGN_BIT - 1U))
#define MY_I2C_MCP9808_TEMP_LWR_BIT		BIT(13)
#define MY_I2C_MCP9808_TEMP_UPR_BIT		BIT(14)
#define MY_I2C_MCP9808_TEMP_CRT_BIT		BIT(15)

#define MY_I2C_MCP9808_REG_RESOLUTION          0x08

struct my_i2c_mcp9808_data {
	uint16_t reg_val;
};

struct my_i2c_mcp9808_config {
	struct i2c_dt_spec i2c;
	uint8_t resolution;
};

int my_i2c_mcp9808_reg_read(const struct device *dev, uint8_t reg, uint16_t *val);
int my_i2c_mcp9808_reg_write_16bit(const struct device *dev, uint8_t reg,
			    uint16_t val);
int my_i2c_mcp9808_reg_write_8bit(const struct device *dev, uint8_t reg,
			   uint8_t val);

/* Encode a signed temperature in scaled Celsius to the format used in
 * register values.
 */
static inline uint16_t my_i2c_mcp9808_temp_reg_from_signed(int temp)
{
	/* Get the 12-bit 2s complement value */
	uint16_t rv = temp & MY_I2C_MCP9808_TEMP_ABS_MASK;

	if (temp < 0) {
		rv |= MY_I2C_MCP9808_TEMP_SIGN_BIT;
	}
	return rv;
}

/* Decode a register temperature value to a signed temperature in
 * scaled Celsius.
 */
static inline int my_i2c_mcp9808_temp_signed_from_reg(uint16_t reg)
{
	int rv = reg & MY_I2C_MCP9808_TEMP_ABS_MASK;

	if (reg & MY_I2C_MCP9808_TEMP_SIGN_BIT) {
		/* Convert 12-bit 2s complement to signed negative
		 * value.
		 */
		rv = -(1U + (rv ^ MY_I2C_MCP9808_TEMP_ABS_MASK));
	}
	return rv;
}

#endif /* MY_I2C_MCP9808_H_ */
