/*
 * board_rev_A.cpp
 *
 *  Created on: 26.02.2022
 *      Author: michi
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "schrank_ant_pcb.h" //contains setting of board revision

#ifndef ESP32_BOARD_REV_B

/**
 * io_init: initialize Rev. A board
 *
 */

void io_init(void)
{

#define GPO_BIT_MASK (1ULL << PHY_PWR)
	gpio_config_t o_conf;
	o_conf.intr_type = GPIO_INTR_DISABLE;
	o_conf.mode = GPIO_MODE_OUTPUT;
	o_conf.pin_bit_mask = GPO_BIT_MASK;
	o_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	o_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&o_conf);
	gpio_set_level((gpio_num_t) PHY_PWR, 1);

	// inputs
	/*
	 #define GPI_BIT_MASK ((1ULL << SWITCH)|(1ULL << SWITCH))
	 gpio_config_t i_conf;
	 i_conf.intr_type = GPIO_INTR_DISABLE;
	 i_conf.mode = GPIO_MODE_INPUT;
	 i_conf.pin_bit_mask = GPI_BIT_MASK;
	 i_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	 i_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	 gpio_config(&i_conf);
	 */
}

#endif
