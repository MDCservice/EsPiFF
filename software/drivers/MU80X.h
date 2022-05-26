/*
 * MU80X.h
 *
 *  Created on: 26.02.2022
 *      Author: steffen
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */


#ifndef MU80X_H_
#define MU80X_H_
#include "main.h"

#define ESP32

#ifdef ESP32
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/uart.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#endif

#define HashSize 16
#define	CRC_POLY_16		0x8005
#define ProtBufLen		260

#define ERR_NOERR		0
#define ERR_CRC			-1
#define ERR_WRITE		-2
#define ERR_NOANSWER	-3
#define ERR_DECODE		-4

#define BAUD_9600		0
#define BAUD_19200		1
#define BAUD_38400		2
#define BAUD_57600		5
#define BAUD_115200		6

class MU80X {
public:
#ifdef ESP32
	MU80X(const int fh);
#else
	MU80X(const char *dev = "/dev/ttyUSB0", const uint16_t baud = 57600);
#endif
	~MU80X(void);

	int ConnectionStatus;
	int iBufferInventory(uint8_t Q, uint8_t antenna, uint8_t time);
	int iClearBuffer(void);												// done
	void DumpBuffer(unsigned char *buf, int len);
	int iGetBuffer(Tag_t *tagarr);
	int iGetReaderInformation();
	int iGetReaderInformation(uint8_t addr);
	int iGetReaderSerial(); 											// done
	//	int iGetGPI(void);
//	int iGetRFIDSettings();
//	int iReset(void);
	int iReadProt(int ms);
	int iSendProt(uint8_t len, uint8_t adr, uint8_t cmd, unsigned char *data); // done
	int iSendProt(uint8_t len, uint8_t adr, uint8_t cmd, unsigned char *data, int wait); // done
//	int	iSetAntennas(int antennas);										// done each antenna is represented by its bit
	int iSetBaud(uint8_t Bd);											// BAUD_XXXXX definitions
//	int iSetGPO(uint8_t GPO);											// later
	int iSetPower(uint8_t dBm);
	int iSetRFRegion();													// fixed to ETSI
	int iSetScantime(uint8_t time);
	void PrintTag(Tag_t tag);
	//	int iSetRFIDSettings(int Q);
//	int iStop();

	unsigned char SendBuf[ProtBufLen];
	unsigned char RecvBuf[ProtBufLen];
	int ProtStatus;
	int TagsInBuffer;
	int TagsSeen;

private:
	//	void vCreateTable();

//	uint16_t crc_tab16_init = 0xFFFF;
//	uint16_t crc_tab16[256];
	int serial_read(int fh,unsigned char *buf);
	int serial_write(int fh, unsigned char * buf,int len);
	uint16_t _ui16CalculateCrc(unsigned char *data, unsigned char len);
	int _DecodeBuffer(int tagcount, int start, unsigned char *buf, Tag_t *tagbuf);

	int _devh;
	uint8_t _devid;
//	unsigned int _baud;
};

#endif /* MU80X_H_ */
