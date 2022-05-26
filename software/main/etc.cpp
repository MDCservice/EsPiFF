/*
 * etc.cpp
 *
 *  Created on: 26.02.2022
 *      Author: steffen
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "etc.h"


/**
 * DumpBuf: helper function to dump a buffer
 *
 * @param buf the buffer to dump.
 * @param len lenght of the buffer to dump.
 */
void DumpBuf(void *buf, int len) {
	int i, c;
	char *lb = (char*) buf;
	for (i = 0; i < len; i++) {
		c = (char) *lb++;
		printf("%02X ", c);
	}
	printf("\n");
}
