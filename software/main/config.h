/*
 * config.h
 *
 *  Created on: 26.02.2022
 *      Author: steffen
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MAIN_CONFIG_H_
#define MAIN_CONFIG_H_
#include "main.h"
#include "HMI.h"

extern const char *TAG;
extern HMI* LCD;

static char netip[16];
static char netma[16];
static char netgw[16];
static char srvip[16];
static char dbnam[16];
static int srvpo;
static int boxid;
static int subbx;

int EditConfig();
int HandleEdit(void);
int ReadConfig();
int SaveConfig(void);
int WriteConfig(char *buff);
void WriteVals(char *name, char *strvals);


#endif /* MAIN_CONFIG_H_ */
