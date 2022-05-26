/*
 * MU80X.cpp
 *
 *  Created on: 15.07.2021
 *      Author: steffen
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
* driver for Chafon MU80X (MU801, MU804, MU808, MU816) Rain UHF RFID reader/writer
*/

#include "MU80X.h"

//#define DEBUG
#ifdef ESP32

/**
 * MU80X constructor
 *
 * @param fh device handle
 */

MU80X::MU80X(int fh) {
	ProtStatus = 0;
	ConnectionStatus=0;
	TagsInBuffer=0;
	TagsSeen=0;
	_devid = 0xFF;

	_devh = fh;

	int res = iGetReaderInformation(_devid);
	if (res >= 0) {
//		printf("RDR Info:%d\n",res);
//		DumpBuffer(RecvBuf,res);
//		fflush (stdout);
		_devid = RecvBuf[1];
	}
}
#else

/**
 * MU80X constructor
 *
 * @param dev device as character
 * @param baud baud rate for serial communication
 */


MU80X::MU80X(const char *dev, const uint16_t baud) {
	struct termios t1, t2;

	ProtStatus = 0;
	_devid = 0xFF;

	int bd = B115200;
	if (baud == 57600)
		bd = B57600;

	_devh = open(dev, O_RDWR | O_NONBLOCK);
	if (_devh < 0) {
		fprintf(stderr, "Unable to open serial device %s", dev);
	} else {

		tcgetattr(_devh, &t1);
		tcgetattr(_devh, &t2);
		cfmakeraw(&t1);

		t1.c_cflag = (CLOCAL | CS8 | CREAD);
		cfsetspeed(&t1, bd);
		t1.c_cc[VMIN] = 0;
		t1.c_cc[VTIME] = 0;
		tcsetattr(_devh, TCSADRAIN, &t1);

		int res = iGetReaderInformation(_devid);
		if (res >= 0) {
			fflush (stdout);
			_devid = RecvBuf[1];
		}
	}

}
#endif

/**
 * MU80X destructor
 *
 */

MU80X::~MU80X(void) {
#ifdef ESP32

#else
	if (_devh > 0)
		close(_devh);
#endif
}
// internal

/**
 * MU80X DumpBuffer
 *
 * @param buf buffer to dump
 * @param len length of buffer
 */
void MU80X::DumpBuffer(unsigned char *buf, int len) {
	int i, c;
	char *lb = (char*) buf;
	for (i = 0; i < len; i++) {
		if (i % 32 == 0)
			printf("\n");
		c = (char) *lb++;
		printf("%02X ", (unsigned char) c);
		if (c > 32)
			printf(" %c ", c);
		else
			printf("   ");

	}
	printf("\n");
}

/**
 * MU80X PrintTag
 *
 * @param tag tag structure
 */
void MU80X::PrintTag(Tag_t tag) {
	int i;
	for (i = 0; i < 12; i++) {
		printf("%02X ", tag.b[i]);
	}
	printf("Ant:%d RSSI:%d Count:%d ", tag.ant, tag.rssi, tag.count);
	printf("\n");
}

/**
 * MU80X _DecodeBuffer
 *
 * @param todecode maximal length to decode
 * @param start start position to decode
 * @param buf char buffer
 * @param tagbuf pointer to Tag_t structure
 * @returns position after decoding
 */
int MU80X::_DecodeBuffer(int todecode, int start, unsigned char *buf,
		Tag_t *tagbuf) {
	int i;
	uint8_t len, ant, rssi, count;
	for (i = start; i < start + todecode; i++) {
//		DumpBuffer(buf,s);
		ant = *buf;
		buf++;
		len = *buf;
		buf++;
		tagbuf[i].ant = ant;
#ifdef DEBUG
		printf("Ant: %d Len: %d",ant,len);
#endif
		if (len == 8) {
			tagbuf[i].b[0] = 0;
			tagbuf[i].b[1] = 0;
			tagbuf[i].b[2] = 0;
			tagbuf[i].b[3] = 0;
			memcpy(&tagbuf[i].b[4], buf, len);
		} else if (len == 12) {
			memcpy(&tagbuf[i].b[0], buf, len);
		} else {
			fprintf(stderr, "Unknown Taglength:%d :\n", len);
		}
		buf += len;
		rssi = *buf;
		buf++;
		count = *buf;
		buf++;
		tagbuf[i].rssi = rssi;
		tagbuf[i].count = count;
#ifdef DEBUG
		printf("Ant: %d Len: %d\n",ant,len);
//		PrintTag(tagbuf[i]);
#endif

	}
	return i - start;
}

#define PRESET_VALUE 0xFFFF
#define POLYNOMIAL 0x8408

/**
 * MU80X _ui16CalculateCrc
 *
 * @param data data pointer
 * @param len length of data 
 * @returns CRC calculated from data
 */

uint16_t MU80X::_ui16CalculateCrc(unsigned char *data, unsigned char len) {

	unsigned char ucI, ucJ;
	uint16_t uiCrcValue = PRESET_VALUE;
	for (ucI = 0; ucI < len; ucI++) {
		uiCrcValue = uiCrcValue ^ *(data + ucI);
		for (ucJ = 0; ucJ < 8; ucJ++) {
			if (uiCrcValue & 0x0001) {
				uiCrcValue = (uiCrcValue >> 1) ^ POLYNOMIAL;
			} else {
				uiCrcValue = (uiCrcValue >> 1);
			}
		}
	}
	return uiCrcValue;
}

/**
 * MU80X serial_read
 *
 * @param fh device handle.
 * @param buf buffer to read in. 
 * @returns number of bytes read
 */
int MU80X::serial_read(int fh, unsigned char *buf) {
#ifdef ESP32
	return (uart_read_bytes(fh, buf, 1, 1));
#else
	return (read(fh,buf,1));
#endif
}

/**
 * MU80X serial_write
 *
 * @param fh device handle.
 * @param buf buffer to write to. 
 * @param len number of bytes to write. 
 * @returns number of bytes written.
 */
int MU80X::serial_write(int fh, unsigned char *buf, int len) {
#ifdef ESP32
	return (uart_write_bytes(fh, buf, len));
#else
	return(write(fh,buf,len));
#endif
}

/**
 * MU80X iReadProt
 *
 * @param ms
 * @returns number of bytes written.
 */
// low level read and write
int MU80X::iReadProt(int ms) {
	int cnt = ms;
	int cc = 0;
	uint8_t len = 0;
	int i;
	while (cnt > 0) {
		if ((cc == 0) & (serial_read(_devh, &RecvBuf[0]) == 1)) {
			len = (uint8_t) RecvBuf[0];
#ifdef DEBUG
			printf("\nNeed to receive:%d\n", len+1);
#endif
			fflush (stdout);
			cc = 1;
			cnt = ms;
			i = 1;
			while (i <= len) {
				if (serial_read(_devh, &RecvBuf[i]) == 1) {
					i++;
					cnt = ms;
				} else {
//					printf(" X ");fflush(stdout);
					cnt--;
					if (cnt == 0)
						return ERR_NOANSWER;
#ifdef ESP32
					vTaskDelay(1);
#else
					usleep(1000);
#endif
				}
			}
#ifdef DEBUG
			printf("Received:%d\n", len+1);
			fflush(stdout);
#endif
			return len + 1;
		}

		cnt--;
#ifdef ESP32
		vTaskDelay(1);
#else
		usleep(1000);
#endif
	}
	return -1;
}

/**
 * MU80X iSendProt
 *
 * @param len number of bytes to write. 
 * @param adr address to send. 
 * @param cmd command to send.
 * @param data data to send
 * @returns number of bytes written, or error code.
 */
int MU80X::iSendProt(uint8_t len, uint8_t adr, uint8_t cmd,
		unsigned char *data) {
	int res = 0;
	uint16_t CRC = 0;
	SendBuf[0] = len + 4;
	SendBuf[1] = adr;
	SendBuf[2] = cmd;
	memcpy(&SendBuf[3], data, len);
	CRC = _ui16CalculateCrc(SendBuf, len + 3);
	SendBuf[3 + len] = CRC & 0xFF;
	SendBuf[4 + len] = CRC >> 8;
#ifdef DEBUG
	printf("Send:");
	DumpBuffer(SendBuf, len + 5);
#endif
	res = serial_write(_devh, (unsigned char *)SendBuf, (int)len + 5);
#ifdef DEBUG
	printf("Written:%d\n", res);
#endif
	if (res == len + 5) {
		res = iReadProt(1000);
		if (res > 0) {
			uint8_t LEN = RecvBuf[0];
			uint16_t RCRC = RecvBuf[LEN - 1] + (RecvBuf[LEN] << 8);
			uint16_t CCRC = _ui16CalculateCrc(RecvBuf, LEN - 1);
#ifdef DEBUG
			printf("CRC: %04X  %04X\n", RCRC, CCRC);
#endif
			if (RCRC == CCRC) {
				ProtStatus = RecvBuf[2];
				return res;
			} else
				return ERR_CRC;
		} else
			return ERR_NOANSWER;

	} else
		return ERR_WRITE;
	return res;
}

/**
 * MU80X iSendProt (overloaded version)
 *
 * @param len number of bytes to write. 
 * @param adr address to send. 
 * @param cmd command to send.
 * @param data data to send
 * @param wait number of ms to wait
 * @returns number of bytes written, or error code.
 */
int MU80X::iSendProt(uint8_t len, uint8_t adr, uint8_t cmd, unsigned char *data,
		int wait) {
	int res = 0;
	uint16_t CRC = 0;
	SendBuf[0] = len + 4;
	SendBuf[1] = adr;
	SendBuf[2] = cmd;
	memcpy(&SendBuf[3], data, len);
	CRC = _ui16CalculateCrc(SendBuf, len + 3);
	SendBuf[3 + len] = CRC & 0xFF;
	SendBuf[4 + len] = CRC >> 8;
#ifdef DEBUG
	printf("Send:");
	DumpBuffer(SendBuf, len + 5);
#endif
	res = serial_write(_devh, (unsigned char*)SendBuf, (int)len + 5);
#ifdef DEBUG
	printf("SendProt wrtn:%d  ", res);
#endif
	if (res == len + 5) {
		res = iReadProt(wait);
#ifdef DEBUG
		printf(" rcvd:%d\n",res);fflush(stderr);
#endif
		if (res > 0) {
			uint8_t LEN = RecvBuf[0];
			uint16_t RCRC = RecvBuf[LEN - 1] + (RecvBuf[LEN] << 8);
			uint16_t CCRC = _ui16CalculateCrc(RecvBuf, LEN - 1);
			if (RCRC == CCRC) {
				ProtStatus = RecvBuf[2];
				return res;
			} else
				return ERR_CRC;
		} else
			return ERR_NOANSWER;

	} else
		return ERR_WRITE;
	return res;
}

// protocol implementation

/**
 * MU80X iBufferInventory 
 *
 * @param Q Q value. 
 * @param antenna number of antanna to use. 
 * @param time tbd.
 * @returns number of bytes written, or error code.
 */

int MU80X::iBufferInventory(uint8_t Q, uint8_t antenna, uint8_t time) {
	int res = 0;
	unsigned char param[16];
	param[0] = Q;		// QV
	param[1] = 0xFF;  	// SS = TID:01 EPC:FF
	param[2] = 0x00;	// TG no clue
	param[3] = 0x80 + antenna;
	param[4] = time;
	res = iSendProt(5, _devid, 0x18, param, 20000);
	if (res > 0) {
#ifdef DEBUG
		printf("INV:%d Stat:%d\n",res,ProtStatus);fflush(stdout);
#endif
//		DumpBuffer(RecvBuf, res);
		printf("-------------------------\n");
		fflush (stdout);
		if (RecvBuf[0] == 9) {
			TagsInBuffer = RecvBuf[5] + (RecvBuf[4] << 8);
			TagsSeen = RecvBuf[7] + (RecvBuf[6] << 8);
		}
	}
	return res;

}

/**
 * MU80X iClearBuffer 
 *
 * @returns number of bytes cleared, or error code.
 */
int MU80X::iClearBuffer() {
	int res;
	res = iSendProt(0, _devid, 0x73, NULL);
	return res;
}

/**
 * MU80X iBufferInventory 
 *
 * @param tagbuf buttfer for tags. 
 * @returns number of tags found
 */
int MU80X::iGetBuffer(Tag_t *tagbuf) {
	int res = 0;
	int havedata = 1;
	int tagcount = 0;
	int todecode = 0;
	int decoded = 0;
	int readmore = 0;
	res = iSendProt(0, _devid, 0x72, NULL);
	if (res < 0)
		return res;
	while (havedata) {
		todecode = RecvBuf[4];
		readmore = (RecvBuf[3] == 3); // == 3 !
		decoded = _DecodeBuffer(todecode, tagcount, &RecvBuf[5], tagbuf);
#ifdef DEBUG
		DumpBuffer(RecvBuf,res);
		printf("To decode:%d decoded:%d \n", todecode, decoded);
		fflush(stdout);
#endif
		if (decoded == todecode) {
			tagcount += decoded;
			if (readmore) {
				res = iReadProt(100);
				if (res < 0)
					return tagcount;
				todecode = RecvBuf[4];
				readmore = (RecvBuf[3] == 3); // == 3 !
			} else
				havedata = 0;
		} else {
			// sometingwong
			return ERR_DECODE;
		}
	}
	return tagcount;
}

/**
 * MU80X iGetReaderInformation from address
 *
 * @param adr device address
 * @returns Reader information.
 */
int MU80X::iGetReaderInformation(uint8_t addr) {
	int res;
	res = iSendProt(0, addr, 0x21, NULL);
	return res;
}

/**
 * MU80X iGetReaderInformation from _devid
 *
 * @returns Reader information.
 */
int MU80X::iGetReaderInformation() {
	int res;
	res = iSendProt(0, _devid, 0x21, NULL);
	return res;
}

/**
 * MU80X iGetReaderSerial from _devid
 *
 * @returns Reader information.
 */
int MU80X::iGetReaderSerial() {
	int res;
	res = iSendProt(0, _devid, 0x4C, NULL);
	return res;
}

/**
 * MU80X iSetBaud 
 *
 * @param bd baud rate:  0=9600,1=19200,2=38400,5=57600,115200
 * @returns 0 on success, error code otherwise.
 */
int MU80X::iSetBaud(uint8_t bd) { // 0=9600,1=19200,2=38400,5=57600,115200
	int res;
	unsigned char param[2];
	param[0] = bd;
	param[1] = 0;
	res = iSendProt(1, _devid, 0x28, param);
	return res;
}

/**
 * MU80X iSetPower 
 *
 * @param dbm sending power in dbm
 * @returns 0 on success, error code otherwise.
 */
int MU80X::iSetPower(uint8_t dBm) {
	int res;
	unsigned char param[2];
	param[0] = dBm;
	param[1] = 0;
	res = iSendProt(1, _devid, 0x2F, param);
	return res;
}

/**
 * MU80X iSetRFRegion 
 *
 * @returns 0 on success, error code otherwise.
 */
int MU80X::iSetRFRegion() {
	int res;
	unsigned char param[2];
	param[0] = 0x4E; // 0x01 001110 maximum band = 1 + 865,1 + (14x200KHz=2,8 MHz) = 867,9
	param[1] = 0x00; // 0x00 000000 minimum band = 0 = 865,1 MHz + (0 x 200KHz)
	param[2] = 0;
	res = iSendProt(2, _devid, 0x22, param);
	return res;
}

/**
 * MU80X iSetScantime 
 *
 * @param time integer in 100ms units
 * @returns 0 on success, error code otherwise.
 */
int MU80X::iSetScantime(uint8_t time) { // x * 100ms
	int res;
	unsigned char param[2];
	param[0] = time;
	param[1] = 0;
	res = iSendProt(1, _devid, 0x25, param);
	return res;
}

