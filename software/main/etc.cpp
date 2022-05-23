/*
 * etc.cpp
 *
 *  Created on: 26.02.2022
 *      Author: steffen
 */
#include "etc.h"

void DumpBuf(void *buf, int len) {
	int i, c;
	char *lb = (char*) buf;
	for (i = 0; i < len; i++) {
		c = (char) *lb++;
		printf("%02X ", c);
	}
	printf("\n");
}
