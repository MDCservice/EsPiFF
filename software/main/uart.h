/*
 * uart.h
 *
 *  Created on: 26.02.2022
 *      Author: steffen
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MAIN_UART_H_
#define MAIN_UART_H_
#include "main.h"

//static char serin[READ_BUF_SIZE + 16];
static char serout[READ_BUF_SIZE + 16];

void UART_init(void);
void UART_debug(char *buf, int len);
int UART_read(int channel, void *buf, int len);
int UART_write(int channel, void *buf, int len);




#endif /* MAIN_UART_H_ */
