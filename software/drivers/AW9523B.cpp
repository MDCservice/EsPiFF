/*
 * AW9523B.cpp
 *
 *  Created on: 26.02.2022
 *      Author: michi
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */



#ifdef STM32
   #include "stm32l4xx_hal.h"
#else
   #include <sys/types.h>
   #include <stdio.h>
   #include "driver/i2c.h"
   #define I2C_MASTER_TIMEOUT_MS       1000
#endif
#include "AW9523B.hpp"


//#include <stdint.h>
//#include <stdbool.h>


#define _REG(port, reg) (port == P0 ? reg : reg + 1)

AW9523B *i2c_gpio;

/**
 * readI2C: read a register of the AW9523B
 *
 * @param reg Register to be read.
 * @returns Value of the register.
 * @note The STM32 code need to be removed, because V3 will not use an STM32
 */

uint8_t AW9523B::readI2C(uint8_t reg)
{
	//AW9523B_REG_ID
	uint8_t gelesen = 0;	
	uint16_t MemAddress;
	MemAddress = reg;
//HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
#ifdef STM32
	HAL_StatusTypeDef ret;
	ret = HAL_I2C_Mem_Read( &(this->i2c_handle), _addr, MemAddress, 1, &gelesen, 1, HAL_MAX_DELAY);
     	if (ret != HAL_OK)
#else
	esp_err_t res1;
	printf("I2C read I2C-Bus:%d Addr:%d Register: %d BEGIN \n", this->i2c_num, _addr, reg );
	//res1 = i2c_master_read_from_device(this->i2c_num, _addr, /*i2c_buf*/&gelesen, 1, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
	res1 = i2c_master_write_read_device(this->i2c_num, _addr, &reg, 1, &gelesen, 2, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
	printf(" --> gelesen Byte: %d \n", gelesen );
	if (res1 != ESP_OK)
#endif    
    	{ printf("ERROR I2C read I2C -------------------------- \n"); ESP_ERROR_CHECK_WITHOUT_ABORT(res1); return 0; }
    	else {  printf(" I2C read I2C-Bus OK \n");  return gelesen; }
}

/**
 * writeI2C: read a register of the AW9523B
 *
 * @param reg Register to be written.
 * @param val Value to be written  
 * @returns true if ok. Otherwise false
 * @note The STM32 code need to be removed, because V3 will not use an STM32
 */

uint8_t AW9523B::writeI2C(uint8_t reg, uint8_t val)
{
	//AW9523B_REG_ID
	uint16_t MemAddress;
	MemAddress = reg;
    //HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
    //uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
#ifdef STM32    
	HAL_StatusTypeDef ret;
	ret = HAL_I2C_Mem_Write( &(this->i2c_handle), _addr, MemAddress, 1, &val, 1, HAL_MAX_DELAY);
    if (ret != HAL_OK)
#else
	uint8_t I2C_buf[3];
	esp_err_t res1 = ESP_OK;
	I2C_buf[0] = reg; //0x02;	// write to register 0x02
	I2C_buf[1] = 255;
	I2C_buf[2] = 255;
	//res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 3, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	//ESP_ERROR_CHECK(res1 );
	if (res1 != ESP_OK)
#endif    
    { printf("ERROR I2C write I2C -------------------------- \n"); ESP_ERROR_CHECK_WITHOUT_ABORT(res1); return false; }
    else { printf(" I2C write I2C OK \n");  return true; }
}


#ifdef STM32
AW9523B::AW9523B(uint8_t address, I2C_HandleTypeDef _i2c_handle) //AD[1:0] address offset
{
	i2c_handle = _i2c_handle;
	_addr = AW9523B_I2C_ADDRESS + (address & 0x03);
#else
AW9523B::AW9523B(uint8_t address, int _i2c_num) //AD[1:0] address offset
{   
    i2c_num = _i2c_num; //I2C number, 0 or 1 on ESP32 
    _addr = 0x58;/*AW9523B_I2C_ADDRESS; 0x58 for first AW9523, 0x58 for the second */
#endif     
}

/**
 * begin: initialize the AW9523B
 *
 * @returns true if ok. Otherwise false
 * @note The STM32 code need to be removed, because V3 will not use an STM32
 */

bool AW9523B::begin()
{
//    Wire.begin();
//    return readI2C(AW9523B_REG_ID) == AW9523B_ID;
#ifdef STM32
	HAL_StatusTypeDef ret;
	ret = HAL_I2C_IsDeviceReady (&i2c_handle, _addr, 1, 1000);
    	if (ret == HAL_OK)
#else
	uint8_t I2C_buf[3];
	esp_err_t res1 = ESP_OK, res2 = ESP_OK;
	I2C_buf[0] = 0x7F;	// write to soft-RESET register on AW9523
	I2C_buf[1] = 0x00;	// write 0 to reset the AW9523
//	res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
//	res2 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP2, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	printf("\nMaster wrote %d %d %02X\n", res1,res2,I2C_buf[0]);
	
	if ( readI2C(AW9523B_REG_ID) == AW9523B_ID )
#endif    
    	{
       	return true;
    	} else { return false; }
}


/**
 * scanAllAddress: scan the I2C bus
 *
 * @note writes to the console
 */

void AW9523B::scanAllAddress(void)
{
   esp_err_t res1;
   uint8_t gelesen = 0;
   uint8_t reg_addr = AW9523B_REG_ID;
   printf("Scanning I2C bus for devices:\n");
   fflush(stdout);
   for (int i=0; i<255; i++)
   {
       //i2c_master_write_read_device(I2C_MASTER_NUM, slave_addr, &reg_addr,      1, data,     2, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
       //res1 = i2c_master_read_from_device(0,        i,          AW9523B_REG_ID, 1, &gelesen, 1, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
	res1 = i2c_master_write_read_device(0,        i,	    &reg_addr, 1, &gelesen, 1, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
       if (res1 == ESP_OK) 
       { 
           printf("\ndevice found on I2C-address %d, read value: %d AW9523=16 \n", i, gelesen); 
       
       }
       else { printf(".");  }
       fflush(stdout);
       gelesen = 0;
   }
   printf("\nfinish I2C Bus scan\n");
}

/**
 * configInOut: configure port 0 or 1 as Input or Output
 *
 * @param port port to configure. Ether 0 or 1
 * @param inout input or output, type AW9523B_P0_CONF_STATE
 */

void AW9523B::configInOut(AW9523BPort port, uint8_t inout)
{
    writeI2C(_REG(port, AW9523B_P0_CONF_STATE), inout);
}

/**
 * configLedGPIO: configure port 0 or 1 as Input or Output
 *
 * @param port port to configure. Ether 0 or 1
 * @param inout input or output, type AW9523B_P0_CONF_STATE
 */

void AW9523B::configLedGPIO(AW9523BPort port, uint8_t ledGpio)
{
    writeI2C(_REG(port, AW9523B_P0_LED_MODE), ledGpio);
}

/**
 * setPort0Mode: set the mode of port0
 *
 * @param mode ether GPIO or LED
 */

void AW9523B::setPort0Mode(AW9523BPortMode mode)
{
    writeI2C(AW9523B_REG_GLOB_CTR, mode | ledsDim);
}

/**
 * setLedsDim: set dim value of the LED mode
 *
 * @param dim dimming value
 */

void AW9523B::setLedsDim(AW9523BLedsDim dim)
{
    writeI2C(AW9523B_REG_GLOB_CTR, dim | portMode);
}

/**
 * setLed: set dim value of the LED mode
 *
 * @param led number of LED to set
 * @param value dimming value of the LED
 */


void AW9523B::setLed(AW9523BLedDimCtrl led, uint8_t value)
{
    writeI2C(led, value);
}

/**
 * portIn: set port 0 or 1 as input 
 *
 * @param port number of port to read. Ether 0 or 1
 */

void AW9523B::portIn(AW9523BPort port)
{
    _portIn = _REG(port, AW9523B_P0_IN_STATE);
}

/**
 * portOut:set port 0 or 1 as output 
 *
 * @param port number of port to write. Ether 0 or 1
 */

void AW9523B::portOut(AW9523BPort port)
{
    _portOut = _REG(port, AW9523B_P0_OUT_STATE);
}

/**
 * read: read from port 
 *
 * @param port number of port to read. Use the previous set _portIn.
 * @returns the value read as Byte
 */

uint8_t AW9523B::read()
{
    return readI2C(_portIn);
}

/**
 * read: read from port 0 or 1 
 *
 * @param port number of port to read. Ether 0 or 1
 * @returns the value read as Byte
 */

uint8_t AW9523B::read(AW9523BPort port)
{
    return readI2C(AW9523B_P0_IN_STATE + port);
}

/**
 * read: write to port
 *
 * @param data value to be written to the port, previously set by _portOut.
 * @returns the value read as Byte
 */

uint8_t AW9523B::write(uint8_t data)
{
    return writeI2C(_portOut, data);
}

/**
 * write: write to port
 *
 * @param port the port to write, ether 0 or 1
 * @param data value to be written to the port.
 * @returns the value read as Byte
 */

uint8_t AW9523B::write(AW9523BPort port, uint8_t data)
{
    return writeI2C(AW9523B_P0_OUT_STATE + port, data);
}


/**
 * reset: software reset of the AW9523B
 *
 * @note remove the STM32 code
 */

void AW9523B::reset()
{
    writeI2C(AW9523B_REG_SOFT_RST, 0);
}

#ifdef STM32
  extern "C" bool AW9523B_init(uint8_t address, I2C_HandleTypeDef _i2c_handle)
  {
	i2c_gpio = new AW9523B(address, _i2c_handle);
	if (!i2c_gpio->begin() )
	{
	    //Serial.println("Error: AW9523B not found!");
	    return false;
	} else
	{
		i2c_gpio->reset();
		i2c_gpio->setPort0Mode(PUSH_PULL);
		return true;
	}
  }
  extern "C" void AW9523B_destroy(void)
  {
	delete i2c_gpio;
  }
  extern "C" void AW9523B_setAllOutputHigh(void)
  {
	i2c_gpio->write(P0, 255);
	i2c_gpio->write(P1, 255);
  }
#endif
