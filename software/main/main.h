/*
 * main.h
 *
 *  Created on: 26.02.2022
 *      Author: steffen
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */


#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_netif.h>
#include <esp_eth.h>
#include <esp_log.h>
#include <esp_spiffs.h>
#include <esp_err.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include <lwip/err.h>
#include <lwip/sockets.h>


//    +3,3V              				// Pin 1
//    GND                				// Pin 2
//    ESP_EN              				// Pin 3
#define UHF_RXD_MISO  		36  		// pin 4	UART
#define LCD_RXD_MISO  		39  		// pin 5	UART
// NC pin 6
#define I2Cint				35	  		// pin 7	EXP
#define SCL			  		32 			// pin 8	EXP
#define SDA			  		33 			// pin 9	EXP
#define EMAC_RXD0_RMII 		25 			// pin 10   ETH
#define EMAC_RXD1_RMII  	26  		// pin 11   ETH
#define EMAC_RX_CRS_DV  	27  		// pin 12   ETH
#define HS2_CLK  			14  		// pin 13   ETH
#define PHY_PWR  			12  		// pin 14   ETH
//  									// pin 15	GND
#define I2C_SDA_40p			13  		// pin 16	40p
// NC pin 17 bis 22
#define HS2_CMD  			15  		// pin 23   ETH
#define HS2_DATA0  			02  		// pin 24   ETH
#define ETH_CLKREF 			0 			// pin 25   ETH
#define UHF_TXD_MOSI 		04  		// pin 26	UART
#define LCD_TXD_MOSI 		16  		// pin 27	UART
#define EMAC_CLK_OUT_180  	17 			// pin 28   ETH
#define SPI_CS  			05  		// pin 29	40p
#define MDIO_RMII  			18			// pin 30	ETH
#define EMAC_TXD0_RMII		19  		// pin 31	ETH
//NC pin 32
#define EMAC_TX_EN_RMII  	21  		// pin 33	ETH
#define RDR_RXD_MISO  		03  		// pin 34	UART
#define RDR_TXD_MOSI  		01  		// pin 35	UART
#define EMAC_TXD1_RMII  	22  		// pin 36	ETH
#define MDC_RMII  			23			// pin 37	ETH
//GND pin 38, 39
#define RDR_UART_CHANNEL			0
#define RDR_UART_BAUD_RATE     		9600
//#define RDR_UART_BAUD_RATE     		115200
#define RDR_UART_RX_PIN				RDR_RXD_MISO
#define RDR_UART_TX_PIN				RDR_TXD_MOSI

#define LCD_UART_CHANNEL			1
#define LCD_UART_BAUD_RATE     		115200
#define LCD_UART_RX_PIN				LCD_RXD_MISO
#define LCD_UART_TX_PIN				LCD_TXD_MOSI

#define UHF_UART_CHANNEL			2	// for easy adaptation in the future
#define UHF_UART_BAUD_RATE        	115200
#define UHF_UART_RX_PIN				UHF_RXD_MISO
#define UHF_UART_TX_PIN				UHF_TXD_MOSI

#define I2C_MASTER_SCL_IO			SCL
#define I2C_MASTER_SDA_IO           SDA
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          100000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define I2C_PORTEXP1				0x58
#define I2C_PORTEXP2				0x5A

#define ADC_DEFAULT_IIC_ADDR 		0x48
#define ADC_CHAN_NUM 				8
#define ADC_REG_RAW_DATA_START 		0x10
#define ADC_REG_VOL_START 			0x20
#define ADC_REG_RTO_START 			0x30
#define ADC_REG_SET_ADDR 			0xC0

// GPA	Output
// GPB  Input

#define LOCK1						0x40
#define LOCK2						0x10
#define LOCK3						0x20
#define LOCK4						0x80
#define LOCK_MASK					LOCK1+LOCK2+LOCK3+LOCK4
#define LOCKPORT					0x01

#define LOCK1R						0x04
#define LOCK2R						0x01
#define LOCK3R						0x02
#define LOCK4R						0x08
#define LOCKR_MASK					LOCK1R+LOCK2R+LOCK3R+LOCK4R
#define LOCKR_PORT					0x02

#define TASK_STACK_SIZE         	2048
#define READ_BUF_SIZE           	1024

#define STX	0x02
#define ETX 0x03
#define EOT 0x04

#define MAX_TAGS 256

typedef struct {
  unsigned char b[12];
  unsigned char ant;
  unsigned char rssi;
  unsigned char count;
} Tag_t;




#endif /* MAIN_MAIN_H_ */
