/*
 * iic.cpp
 *
 *  Created on: 26.02.2022
 *      Author: steffen
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "iic.h"

/**
 * I2C_read: read a register of the port expander
 *
 * @param slave_addr I2C slave address
 * @param reg_addr register to read
 * @param data result array
 * @param len number of bytes to read
 * @returns Value of the register.
 */

esp_err_t I2C_read(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint8_t len) {
	return i2c_master_write_read_device(I2C_MASTER_NUM, slave_addr, &reg_addr, 1, data, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * I2C_register_write_byte: write a byte to the port expander
 *
 * @param slave_addr I2C slave address
 * @param reg_addr register to read
 * @param data value to write
 * @returns number of bytes written.
 */
esp_err_t I2C_register_write_byte(uint8_t slave_addr, uint8_t reg_addr, uint8_t data) {
	uint8_t write_buf[2] = { reg_addr, data };
	return i2c_master_write_to_device(I2C_MASTER_NUM, slave_addr, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * I2C_init: write a initalize the port expander
 *
 * @returns > then 0 if success, otherwise -1.
 */
esp_err_t I2C_init(void) {
	int res1,res2;
	int I2C_master_port = I2C_MASTER_NUM;
	i2c_config_t I2C_conf = { .mode = I2C_MODE_MASTER, .sda_io_num = I2C_MASTER_SDA_IO, .scl_io_num = I2C_MASTER_SCL_IO, .sda_pullup_en = GPIO_PULLUP_ENABLE, .scl_pullup_en = GPIO_PULLUP_ENABLE, .master { .clk_speed = I2C_MASTER_FREQ_HZ, } };
	i2c_param_config(I2C_master_port, &I2C_conf);
	res1=i2c_driver_install(I2C_master_port, I2C_conf.mode,
	I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

	I2C_buf[0] = 0x7F;	// write data to output port 1
	I2C_buf[1] = 0x00;	// write data to output port 1
	res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	res2 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP2, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	printf("\nMaster wrote %d %d %02X\n", res1,res2,I2C_buf[0]);

	I2C_buf[0] = 0x02;	// write data to output port 1
	I2C_buf[1] = 0xFF;	// write data to output port 1
	I2C_buf[2] = 0xFF;	// write data to output port 1
	I2C_buf[3] = 0x00;	// write data to output port 1
	I2C_buf[4] = 0x00;	// write data to output port 1
	res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 5, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	res2 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP2, I2C_buf, 5, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	printf("\nMaster wrote %d %d %02X\n", res1,res2,I2C_buf[0]);

	I2C_buf[0] = 0x11;	// write data to output port 1
	I2C_buf[1] = 0x10;	// write data to output port 1
	res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	res2 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP2, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	printf("\nMaster wrote %d %d %02X\n", res1,res2,I2C_buf[0]);



	//	printf("\nI2C wrote 0x%02X%02X: %d\n", I2C_buf[0], I2C_buf[1], res1);
//	I2C_buf[0] = 0x00;	// lese 3 bytes
//	res2 = i2c_master_read_from_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);


	/*
	I2C_buf[0] = 0x8C;	// write config port 0 with auto inc
	I2C_buf[1] = 0xFF;	// Port 0 Input
	I2C_buf[2] = 0x00;	// Port 1 Output
	I2C_buf[3] = 0xFF;	// Port 2 Input
	res = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 4, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	printf("\nMaster wrote %02X %02X: %d\n", I2C_buf[0], I2C_buf[1], res);

	I2C_buf[0] = 0x05;	// write data to output port 1
	I2C_buf[1] = 0x00;
	res = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	printf("\nMaster wrote %02X %02X: %d\n", I2C_buf[0], I2C_buf[1], res);
*/
	return res1;
}

/**
 * ADC_init: write a initalize the ADC
 *
 * @returns > then 0 if success, otherwise -1.
 */
esp_err_t ADC_init(void) {
	int res;

	I2C_buf[0] = 0x01;	// -> config register
	I2C_buf[1] = 0x40;	//   0 100   000 0   A0-GND +-6.144 CONT
	I2C_buf[2] = 0x0;	//   O00 0   0 0 11  8 SPS no comp
	res = i2c_master_write_to_device(I2C_MASTER_NUM, ADC_DEFAULT_IIC_ADDR, I2C_buf, 3, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	printf("\nMaster wrote %02X %02X: %d\n", I2C_buf[0], I2C_buf[1], res);
	return res;
}

/**
 * ADC_init: read all ADC inputs
 *
 * @returns > then 0 if success, otherwise -1.
 */
int ADCReadAll(void) {
//    for i in range(ADC_CHAN_NUM):
//         data=self.bus.read_i2c_block_data(self.addr,REG_RAW_DATA_START+i,2)
//         val=data[1]<<8|data[0]
//         array.append(val)
	int res1=0,res2,i;
	I2C_buf[0] = 0x00;	// conversion register
	I2C_buf[1] = 0x00;
	I2C_buf[2] = 0x00;
	res1 = i2c_master_write_to_device(I2C_MASTER_NUM, ADC_DEFAULT_IIC_ADDR, I2C_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
//	printf("\nI2C wrote 0x%02X: %d\n", I2C_buf[0], res1);

	for (i=0;i<256;i++) {
		res2 = i2c_master_read_from_device(I2C_MASTER_NUM, ADC_DEFAULT_IIC_ADDR, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
		printf("I2C read %02x -> ",i);
//	for (i=0;i<4;i++) {
		printf(" %02X %02X ",I2C_buf[0],I2C_buf[1]);
//	}
		printf(" %d\n", res2);
	}
//	printf(" #:S%d R:%d ", res1, res2);
	fflush(stdout);

	return res1;
}

/**
 * DoorIsClosed: test a door, if a switch is closed
 *
 * @param doormask specifies what doors to test
 * @returns 1 of open, otherwise 0
 */
int DoorIsClosed(int doormask) {
	int res1,res2;
	I2C_buf[0] = 0x80;	// Setze CMD read port 0
	I2C_buf[1] = 0x00;
	I2C_buf[2] = 0x00;
	res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
//	printf("\nI2C wrote 0x%02X%02X: %d\n", I2C_buf[0], I2C_buf[1], res1);
	I2C_buf[0] = 0x00;	// lese 3 bytes
	res2 = i2c_master_read_from_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 3, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
//	printf("\nI2C read 0x %02X %02X  %02X: %d\n", I2C_buf[0], I2C_buf[1], I2C_buf[2], res2);
	printf(" #:S%d R:%d ", res1, res2);
	fflush(stdout);
	// Open       0x0F = 0000 1111
	// 1 closed     0x0B = 0000 1011
	// 2 closed     0x0E = 0000 1110
	// 3 closed     0x0D = 0000 1101
	// 4 closed     0x07 = 0000 0111
	if ((I2C_buf[LOCKR_PORT] & doormask)==0) {
		return 1;
	} else {
		return 0;
	}

}

/**
 * DoorOpen: open a door lock
 *
 * @param door number of the door to open
 * @returns 0 if successfull
 */
int DoorOpen(int door) {
	int mask=0;
	int maskr=0;
	int res;
	int to=10;
	switch (door) { // we allow only one door to open
	case 1: {
		mask = LOCK1;
		maskr= LOCK1R;
	} break;
	case 2: {
		mask = LOCK2;
		maskr= LOCK2R;
	} break;
	case 3: {
		mask = LOCK3;
		maskr= LOCK3R;
	} break;
	default: {
		mask = LOCK4;
		maskr= LOCK4R;
	}
	}

	I2C_buf[0] = 0x05;
	I2C_buf[1] = mask;
	printf("\nMASTER wrote %02X %02X\n", I2C_buf[0], I2C_buf[1]);fflush(stdout);
	res = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	printf("\nI2C wrote 0x%02X%02X: %d\n", I2C_buf[0], I2C_buf[1], res);fflush(stdout);
	while (to>0) {
		if (DoorIsClosed(maskr)==0) {
			to=0;
		} else {
			to--;
			vTaskDelay(1);
		}
	}
	I2C_buf[0] = 0x05;
	I2C_buf[1] = 0x00;
	printf("\nMASTER wrote 0x%02X%02X\n", I2C_buf[0], I2C_buf[1]);fflush(stdout);
	res = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	printf("\nI2C wrote 0x%02X%02X: %d\n", I2C_buf[0], I2C_buf[1], res);

	return 0;
}



