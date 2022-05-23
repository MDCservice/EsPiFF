/*
 * HMI.cpp
 *
 *  Created on: 17.08.2021
 *      Author: steffen
 */
#include "HMI.h"

#ifdef ESP32
HMI::HMI(int fh) {
	_devh = fh;
}
#else
HMI::HMI(const char *dev, const uint16_t baud) {
	struct termios t1, t2;

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
	}

}
#endif


int HMI::serial_read(int fh, char *buf) {
#ifdef ESP32
	return (uart_read_bytes(fh, buf, 1, 1));
#else
	return (read(fh,buf,1));
#endif
}

int HMI::serial_write(int fh, char *buf, int len) {
#ifdef ESP32
	return (uart_write_bytes(fh, buf, len));
#else
	return(write(fh,buf,len));
#endif
}

int HMI::iReadProt(int ms) {
	int cnt = ms;
	int ec = 0;	// end counter (3 x 0xFF)
	char rc;
	int i=0;
	int res;
	printf("CNT:%d\n",cnt);fflush(stdout);
	while (cnt > 0) {
		if ((res=serial_read(_devh, &rc)) == 1) {
#ifdef DEBUG
			printf(" <%02X> ", rc);
			fflush (stdout);
#endif
			RecvBuf[i++]=rc;
			if (rc==0xFF) {
				ec++;
				if (ec==3) {
					i-=3;
					RecvBuf[i]=0;
#ifdef DEBUG
					idumpbuf(RecvBuf,i);
					printf("OK Received:%d\n", i);
					fflush(stdout);
#endif
					return HMIResultOK;
				}
			}
			cnt = ms;
		} else {
			cnt--;
#ifdef DEBUG
			printf(" - ");
			fflush (stdout);
#endif
#ifdef ESP32
			vTaskDelay(1);
#else
			usleep(1000);
#endif
		}
	}
	if (i==0) return HMIResultReadErr;
	RecvBuf[i]=0;
#ifdef DEBUG
	printf("ERR Received:%d:%s\n", i,RecvBuf);
	fflush(stdout);
#endif
	return HMIResultReadErr;
}

int HMI::iReadValue(char *name,int *result) {
	char cmd[64];
	int res;
	sprintf(cmd,"get %s.txt",name);
	res=iSendProt((int)strlen(cmd),cmd);
	return res;
}
int HMI::iReadValue(char *name, char *result) {
	char cmd[64];
	int res;
	sprintf(cmd,"get %s.txt",name);
	res=iSendProt((int)strlen(cmd),cmd,10);
//	printf(">%s:%d<\n",cmd,res);fflush(stdout);
	if (res==HMIResultOK) {
//		idumpbuf((void *)RecvBuf, (int)strlen((char *)RecvBuf));
		strncpy(result,(char *)&RecvBuf[1],16);
	}
	return res;
}
int HMI::iWriteValue(char *name,int value) {
	char cmd[64];
	int res;
	sprintf(cmd,"%s.txt=\"%d\"",name,value);
#ifdef DEBUG
//	printf(">%s<\n",cmd);fflush(stdout);
#endif
	res=iSendProt((int)strlen(cmd),cmd);
	return res;
}
int HMI::iWriteValue(char *name,char * value) {
	char cmd[64];
	int res;
	sprintf(cmd,"%s.txt=\"%s\"",name,value);
#ifdef DEBUG
//	printf(">%s<\n",cmd);fflush(stdout);
#endif
	res=iSendProt((int)strlen(cmd),cmd);
	return res;
}
int HMI::iSendProt(int len, char *data) {
	int res = 0;
	int res2=0;
	char lbuf[]={0xFF,0xFF,0xFF};
	res = serial_write(_devh, (char *)data, (int)len);
	if (res>0) res2=serial_write(_devh, lbuf, 3);
	else return HMIResultWriteErr;
	if (res2<0) return HMIResultWriteErr;
	return HMIResultOK;;
}
int HMI::iSendProt(int len, char *data, int wait) {
	int res = 0;
	int res2=0;
	char lbuf[]={0xFF,0xFF,0xFF};
	res = serial_write(_devh, (char *)data, (int)len);
	if (res>0) res2=serial_write(_devh, (char *)lbuf, 3);
	else return HMIResultWriteErr;
	if (res2<0) return HMIResultWriteErr;
	if (wait==0) return res;
	res = iReadProt(wait);
	return res;
}
void HMI::idumpbuf(void *buf, int len) {
	int i, c;
	char *lb = (char*) buf;
	for (i = 0; i < len; i++) {
		c = (char) *lb++;
		printf("%02X ", c);
	}
	printf("\n");
}
