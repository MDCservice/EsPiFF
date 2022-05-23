#ifndef _BOARD_REV_B_H_
#define _BOARD_REV_B_H_

//board definitions for Rev B board (LAN8720 on board)
#include "schrank_ant_pcb.h" //contains setting of board revision


#define UHF_RXD_MISO  		36  		// pin 4	UART
#define LCD_RXD_MISO  		39  		// pin 5	UART

#define SCL			  	32 //new Michi 01.05.2022
#define SDA			  	33 //new Michi 01.05.2022


#define I2C_MASTER_SCL_IO		SCL
#define I2C_MASTER_SDA_IO           	SDA
#define I2C_MASTER_NUM              	0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          	100000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   	0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   	0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       	1000
#define I2C_PORTEXP			0x22
#define I2Cint				35	
#define EMAC_RXD0_RMII 		25 			// pin 10   ETH
#define EMAC_RXD1_RMII  		26  		// pin 11   ETH  
#define EMAC_RX_CRS_DV  		27 	
#define HS2_CLK  			14     
//#define PHY_PWR is in Rev B handled by I2C port expander, no longer a GPIO
//#define I2C_SDA_40p			13  // the I2C to the RasPi 40 port header is switched with the second port expander 
#define HS2_CMD  			15 
#define HS2_DATA0  			02  
#define ETH_CLKREF 			0 
#define UHF_TXD_MOSI 			04 
#define LCD_TXD_MOSI 			5
//#define EMAC_CLK_OUT_180  	17 // there is no GPIO17 on the WROVER module
#define SPI_CS  			13// was: 05
#define MDIO_RMII  			18
#define EMAC_TXD0_RMII			19
#define EMAC_TX_EN_RMII  		21
#define RDR_RXD_MISO  			03 
#define RDR_TXD_MOSI  			01
#define EMAC_TXD1_RMII  		22
#define MDC_RMII  			23
//#define I2C_PORTEXP			0x22 

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
  
// AW9523 I2C adress P40 1011000 = 88, 0x58
// AW9523 I2C adress 10110 AD1 AD0 RW	
//#define AW9523B_I2C_ADDRESS_MAIN 88 //0x58
#define AW9523B_I2C_ADDRESS_MAIN 	0x58
	
// AW9523 I2C adress P40 10110100 = 90, 0x5A	
#define AW9523B_I2C_ADDRESS_ETH  	0x5A //90dec   
#define I2C_PORTEXP1			0x58
#define I2C_PORTEXP2			0x5A

void io_init(void);   	 
esp_err_t i2c_init(void);
void kolban_i2cscanner(void ); 

#endif

