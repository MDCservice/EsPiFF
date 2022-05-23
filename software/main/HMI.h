/*
 * LCD.h
 *
 *  Created on: 17.08.2021
 *      Author: steffen
 */

#ifndef HMI_H_
#define HMI_H_
#include "main.h"

#define DEBUG
#define ESP32
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

#define PIC_WELCOME		0
#define PIC_OPENDOOR	1
#define PIC_EMPTY		2
#define PIC_KAS_OK		3
#define PIC_KAS_DIS		4
#define PIC_TRS_OK		5
#define PIC_TRS_DIS		6

#define BUT0			p0
#define TXT0			t0
#define BUT1			p1
#define TXT1			t1
#define BUT2			p2
#define TXT2			t2
#define BUT3			p3
#define TXT3			t3
#define BUT4			p4
#define TXT4			t4
#define BUT5			p5
#define TXT5			t5

#ifdef ESP32
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
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

#define ESP32
#define HMIResultOK			0
#define HMIResultWriteErr	-1
#define HMIResultReadErr	-2
#define HMIResultProtErr	-3

typedef struct {
  int page;
  int id;
  int state;
} HMI_t;

class HMI {
public:

#ifdef ESP32
	HMI(const int fh);
#else
	HMI(const char *dev = "/dev/ttyUSB0", const uint16_t baud = 57600);
#endif
	~HMI(void);

	int iReadProt(int ms);
	int iReadValue(char *name, int *res);
	int iReadValue(char *name,char *res);
	int iWriteValue(char *name,int value);
	int iWriteValue(char *name,char *value);
	int iSendProt(int len, char *data); // done
	int iSendProt(int len, char *data, int wait); // done

private:
	int _devh;
	unsigned char SendBuf[ProtBufLen];
	unsigned char RecvBuf[ProtBufLen];
	int serial_read(int fh, char *buf);
	int serial_write(int fh, char * buf,int len);
	void idumpbuf(void *buf, int len);

};
#endif /* HMI_H_ */
