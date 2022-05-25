/*
 * board_rev_B.cpp
 *
 *  Created on: 26.02.2022
 *      Author: michi
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "schrank_ant_pcb.h" //contains setting of board revision

#ifdef ESP32_BOARD_REV_B
#include "board_rev_B.hpp"

/**
 * io_init: initalize Rev. B boards
 *
 */

void io_init(void)
{
	#define GPO_BIT_MASK 0 //we dont have GPIO as outputs, all output is done via I2C Port Expander
/* as we have not ouput, leave default settings
	gpio_config_t o_conf;
	o_conf.intr_type = GPIO_INTR_DISABLE;
	o_conf.mode = GPIO_MODE_OUTPUT;
	o_conf.pin_bit_mask = GPO_BIT_MASK;
	o_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	o_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&o_conf);
*/	
	//gpio_set_level((gpio_num_t) PHY_PWR, 1);

	// inputs

	#define IRQ2Esp 34
	#define I2Cext_INT 35
	 #define GPI_BIT_MASK ((1ULL<<IRQ2Esp) | (1ULL<<I2Cext_INT)) 
	 //io_conf.pin_bit_mask = (1ULL<<15);//bit mask of the pins that you want to set,e.g.GPIO15
	 gpio_config_t i_conf;
	 i_conf.intr_type = GPIO_INTR_DISABLE;
	 i_conf.mode = GPIO_MODE_INPUT;
	 i_conf.pin_bit_mask = GPI_BIT_MASK;
	 i_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	 i_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	 gpio_config(&i_conf);
	 
}




esp_err_t i2c_init(void) {	
	int res1,res2;
	int I2C_master_port = I2C_MASTER_NUM;
	uint8_t I2C_buf[5];
	i2c_config_t I2C_conf = { .mode = I2C_MODE_MASTER, .sda_io_num = I2C_MASTER_SDA_IO, .scl_io_num = I2C_MASTER_SCL_IO, .sda_pullup_en = GPIO_PULLUP_ENABLE, .scl_pullup_en = GPIO_PULLUP_ENABLE, .master { .clk_speed = I2C_MASTER_FREQ_HZ, } };
	i2c_param_config(I2C_master_port, &I2C_conf);
		
	printf("\nInstall i2c_driver: Mode=%d sda_io_num=%d scl_io_num=%d\n", I2C_MODE_MASTER, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
	res1=i2c_driver_install(I2C_master_port, I2C_conf.mode,
	I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);	
	ESP_ERROR_CHECK_WITHOUT_ABORT(res1);
	printf("After i2c_driver_install\n");
	vTaskDelay(500 / portTICK_PERIOD_MS);

//1. Init AW9523 on address 0x00
	I2C_buf[0] = 0x7F;	// write to AW9523 Soft Reset register
	I2C_buf[1] = 0x00;	// write data to output port 1
	I2C_buf[2] = 0x00;	// write data to output port 1
	printf("I2C write to bus %d to I2C-Adress %d byte0 = %d byte 1 = %d\n", I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf[0], I2C_buf[1] );
	res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 3, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	printf("I2C write to bus %d to I2C-Adress %d byte0 = %d byte 1 = %d\n", I2C_MASTER_NUM, I2C_PORTEXP2, I2C_buf[0], I2C_buf[1] );
	res2 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP2, I2C_buf, 3, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	printf("\nMaster wrote %d %d %02X\n", res1,res2,I2C_buf[0]);
	printf("After AW9523 soft-RESET\n");
	vTaskDelay(500 / portTICK_PERIOD_MS);

//2. Set outputs push pull in AW9523 Global Control Register	
	I2C_buf[0] = 0x11;	// write data to Global Control Register
	I2C_buf[1] = 0x10;	// write data to Global Control Register, set Bit4 to Push Pull
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
	printf("After AW9523 all GPIOs HIGH\n");
	vTaskDelay(500 / portTICK_PERIOD_MS);


	
//2. Set AW9523-PhyPower to high	
/*	I2C_buf[0] = 0x02;	// write to register 0x02
	I2C_buf[1] = 255;	// set all pins high
	I2C_buf[2] = 255;	// set all pins high
	res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP2, I2C_buf, 3, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);	
*/	
	return res1;
}

#endif
