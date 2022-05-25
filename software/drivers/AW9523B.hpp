/*
 * AW9523B.h
 *
 *  Created on: 26.02.2022
 *      Author: michi
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _AW9523B_H_
#define _AW9523B_H_

//#include <Arduino.h>
//#include <stdint.h>
//#include <stdbool.h>

#ifdef STM32
   #include "stm32l4xx_hal.h"
#endif


/** Registers */
#define AW9523B_I2C_ADDRESS 0x58   ///< I2C base address for AW9523B
#define AW9523B_REG_ID 0x10        ///< id register
#define AW9523B_ID 0x23            ///< id value
#define AW9523B_P0_IN_STATE 0x00   ///< P0 port input state
#define AW9523B_P1_IN_STATE 0x01   ///< P1 port input state
#define AW9523B_P0_OUT_STATE 0x02  ///< P0 port output state
#define AW9523B_P1_OUT_STATE 0x03  ///< P1 port output state
#define AW9523B_P0_CONF_STATE 0x04 ///< P0 port config state
#define AW9523B_P1_CONF_STATE 0x05 ///< P1 port config state
#define AW9523B_REG_GLOB_CTR 0x11  ///< Global control register
#define AW9523B_P0_LED_MODE 0x12   ///< P0 port led mode switch register
#define AW9523B_P1_LED_MODE 0x13   ///< P1 port led mode switch register
#define AW9523B_REG_SOFT_RST 0x7F  ///< Soft reset register



/** AW9523B Port constants */
enum AW9523BPort
{
    P0 = 0x00, // Port 0
    P1 = 0x01, // Port 1
};

enum AW9523BPortMode
{
    OPEN_DRAIN = 0x00, // Port 0 open drain mode
    PUSH_PULL = 1 << 4 // Port 0 push pull mode
};

/** AW9523B Port0 LED dimmer constants: 256 step dimming range select*/
enum AW9523BLedsDim
{
    DIM_MAX = 0x00,//B00, // 0~IMAX 37mA(typical)
    DIM_MED = 0x01,//B01, // 0~(IMAX×3/4)
    DIM_LOW = 0x02,//B10, // 0~(IMAX×2/4)
    DIM_MIN = 0x03,//B11, // 0~(IMAX×1/4)
};

/** AW9523B LED dimm current control registers*/
enum AW9523BLedDimCtrl
{
    P1_0 = 0x20, // DIM0
    P1_1 = 0x21, // DIM1
    P1_2 = 0x22, // DIM2
    P1_3 = 0x23, // DIM3
    P0_0 = 0x24, // DIM4
    P0_1 = 0x25, // DIM5
    P0_2 = 0x26, // DIM6
    P0_3 = 0x27, // DIM7
    P0_4 = 0x28, // DIM8
    P0_5 = 0x29, // DIM9
    P0_6 = 0x2A, // DIM10
    P0_7 = 0x2B, // DIM11
    P1_4 = 0x2C, // DIM12
    P1_5 = 0x2D, // DIM13
    P1_6 = 0x2E, // DIM14
    P1_7 = 0x2F, // DIM15
};

// Uncomment to enable debug messages
//#define AW9523B_DEBUG

// Define where debug output will be printed
#define DEBUG_PRINTER Serial

// Setup debug printing macros
#ifdef AW9523B_DEBUG
#define DEBUG_PRINT(...)                  \
    {                                     \
        DEBUG_PRINTER.print(__VA_ARGS__); \
    }
#define DEBUG_PRINTLN(...)                  \
    {                                       \
        DEBUG_PRINTER.println(__VA_ARGS__); \
    }
#else
#define DEBUG_PRINT(...) \
    {                    \
    }
#define DEBUG_PRINTLN(...) \
    {                      \
    }
#endif

/**************************************************************************/
/*!
    @brief  AW9523B I2C 16bit GPIO expander and LED driver
*/
/**************************************************************************/
class AW9523B
{

public:
#ifdef STM32
    AW9523B(uint8_t address, I2C_HandleTypeDef _i2c_handle); //AD[1:0] address offset
#else
    AW9523B(uint8_t address, int _i2c_num); //AD[1:0] address offset
#endif    
    bool begin();
    void configInOut(AW9523BPort port, uint8_t inout);
    void configLedGPIO(AW9523BPort port, uint8_t ledGpio);
    void setPort0Mode(AW9523BPortMode mode);
    void setLedsDim(AW9523BLedsDim value);
    void setLed(AW9523BLedDimCtrl led, uint8_t value);
    void portIn(AW9523BPort port);
    void portOut(AW9523BPort port);
    uint8_t read();
    uint8_t read(AW9523BPort port);
    uint8_t write(uint8_t data);
    uint8_t write(AW9523BPort port, uint8_t data);
    void reset();
    void scanAllAddress(void);
private:
    uint8_t writeI2C(uint8_t reg, uint8_t val);
    uint8_t readI2C(uint8_t reg);
    uint8_t _addr;
    uint8_t _portIn = AW9523B_P0_IN_STATE;
    uint8_t _portOut = AW9523B_P0_OUT_STATE;
    AW9523BPortMode portMode = OPEN_DRAIN;
    AW9523BLedsDim ledsDim = DIM_MAX;
#ifdef STM32    
    I2C_HandleTypeDef i2c_handle;//Michi
#else
    /*i2c_port_t*/int i2c_num;    
#endif    
};

#endif // _AW9523B_H_
