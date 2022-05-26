/*
 * iic.h
 *
 *  Created on: 26.02.2022
 *      Author: steffen
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MAIN_IIC_H_
#define MAIN_IIC_H_
#include "main.h"

static uint8_t I2C_buf[16];

esp_err_t I2C_read(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint8_t len) ;
esp_err_t I2C_register_write_byte(uint8_t slave_addr, uint8_t reg_addr, uint8_t data);
//uint8_t write_buf[2] = { reg_addr, data };
//	return i2c_master_write_to_device(I2C_MASTER_NUM, slave_addr, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
//}
esp_err_t I2C_init(void);
esp_err_t ADC_init(void);
int ADCReadAll(void);
int DoorIsClosed(int doormask);
int DoorOpen(int door);

#endif /* MAIN_IIC_H_ */
