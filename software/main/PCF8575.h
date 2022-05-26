/*
 * PCF8575.h
 *
 *  Created on: 26.02.2022
 *      Author: steffen
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */


#ifndef PCF8575_h
#define PCF8575_h

#include "Wire.h"

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// Uncomment to enable printing out nice debug messages.
// #define PCF8575_DEBUG

// Uncomment for low memory usage this prevent use of complex DigitalInput structure and free 7byte of memory
// #define PCF8575_LOW_MEMORY

// Define where debug output will be printed.
#define DEBUG_PRINTER Serial

// Define to manage original pinout of pcf8575
// like datasheet but not sequential
//#define NOT_SEQUENTIAL_PINOUT

// Setup debug printing macros.
#ifdef PCF8575_DEBUG
	#define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
	#define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
	#define DEBUG_PRINT(...) {}
	#define DEBUG_PRINTLN(...) {}
#endif

#define READ_ELAPSED_TIME 10

//#define P0  	B00000001
//#define P1  	B00000010
//#define P2  	B00000100
//#define P3  	B00001000
//#define P4  	B00010000
//#define P5  	B00100000
//#define P6  	B01000000
//#define P7  	B10000000
//
#ifdef NOT_SEQUENTIAL_PINOUT
	#define P00  	0
	#define P01  	1
	#define P02  	2
	#define P03  	3
	#define P04  	4
	#define P05  	5
	#define P06  	6
	#define P07  	7
	#define P10  	8
	#define P11  	9
	#define P12  	10
	#define P13  	11
	#define P14  	12
	#define P15  	13
	#define P16  	14
	#define P17  	15
#else
	#define P0  	0
	#define P1  	1
	#define P2  	2
	#define P3  	3
	#define P4  	4
	#define P5  	5
	#define P6  	6
	#define P7  	7
	#define P8  	8
	#define P9  	9
	#define P10  	10
	#define P11  	11
	#define P12  	12
	#define P13  	13
	#define P14  	14
	#define P15  	15
#endif

#include <math.h>


class PCF8575 {
public:

	PCF8575(uint8_t address);
	PCF8575(uint8_t address, uint8_t interruptPin,  void (*interruptFunction)() );

#if !defined(__AVR) && !defined(__STM32F1__)
	PCF8575(uint8_t address, uint8_t sda, uint8_t scl);
	PCF8575(uint8_t address, uint8_t sda, uint8_t scl, uint8_t interruptPin,  void (*interruptFunction)());
#endif

#ifdef ESP32
	///// changes for second i2c bus
	PCF8575(TwoWire *pWire, uint8_t address);
	PCF8575(TwoWire *pWire, uint8_t address, uint8_t sda, uint8_t scl);

	PCF8575(TwoWire *pWire, uint8_t address, uint8_t interruptPin,  void (*interruptFunction)() );
	PCF8575(TwoWire *pWire, uint8_t address, uint8_t sda, uint8_t scl, uint8_t interruptPin,  void (*interruptFunction)());
#endif

	void begin();
	void pinMode(uint8_t pin, uint8_t mode);

	void readBuffer(bool force = true);
	uint8_t digitalRead(uint8_t pin);
	#ifndef PCF8575_LOW_MEMORY
		struct DigitalInput {
#ifdef NOT_SEQUENTIAL_PINOUT
		uint8_t p00;
		uint8_t p01;
		uint8_t p02;
		uint8_t p03;
		uint8_t p04;
		uint8_t p05;
		uint8_t p06;
		uint8_t p07;
		uint8_t p10;
		uint8_t p11;
		uint8_t p12;
		uint8_t p13;
		uint8_t p14;
		uint8_t p15;
		uint8_t p16;
		uint8_t p17;
#else
		uint8_t p0;
		uint8_t p1;
		uint8_t p2;
		uint8_t p3;
		uint8_t p4;
		uint8_t p5;
		uint8_t p6;
		uint8_t p7;
		uint8_t p8;
		uint8_t p9;
		uint8_t p10;
		uint8_t p11;
		uint8_t p12;
		uint8_t p13;
		uint8_t p14;
		uint8_t p15;
#endif
		} digitalInput;


		DigitalInput digitalReadAll(void);
	#else
		uint16_t digitalReadAll(void);
	#endif
	void digitalWrite(uint8_t pin, uint8_t value);

private:
	uint8_t _address;

	#if defined(__AVR) || defined(__STM32F1__)
		uint8_t _sda;
		uint8_t _scl;
	#else
		uint8_t _sda = SDA;
		uint8_t _scl = SCL;
	#endif

	TwoWire *_wire;

	bool _usingInterrupt = false;
	uint8_t _interruptPin = 2;
	void (*_interruptFunction)(){};

	uint16_t writeMode 	= 	0;
	uint16_t readMode 	= 	0;
	uint16_t byteBuffered = 0;
	unsigned long lastReadMillis = 0;

	uint16_t writeByteBuffered = 0;

};

#endif
