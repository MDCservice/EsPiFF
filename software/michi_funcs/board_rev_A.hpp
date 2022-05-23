#ifndef _BOARD_REV_A_H_
#define _BOARD_REV_A_H_

#include "schrank_ant_pcb.h" //contains setting of board revision

//board definitions for Rev A board (LAN8720 module)

//    +3,3V              				// Pin 1
//    GND                				// Pin 2
//    ESP_EN              				// Pin 3
#define UHF_RXD_MISO  		36  		// pin 4	UART
#define LCD_RXD_MISO  		39  		// pin 5	UART
// NC pin 6
#define I2Cint				35	  		// pin 7	EXP
//#define SCL			  		33 			// pin 8	EXP disabled Michi, all boards have SCL on 32
//#define SDA			  		32 			// pin 9	EXP disabled Michi, all boards have SDA on 33
#define SCL			  		32 //new Michi 01.05.2022
#define SDA			  		33 //new Michi 01.05.2022
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
#define I2C_PORTEXP					0x22


void io_init(void);

#endif

