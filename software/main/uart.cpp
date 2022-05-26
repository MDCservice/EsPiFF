/*
 * uart.cpp
 *
 *  Created on: 26.02.2022
 *      Author: steffen
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */


#include "uart.h"

/**
 * UART_init: initalization of the UARTS
 *
 */
void UART_init(void) {
    int intr_alloc_flags = ESP_INTR_FLAG_IRAM;

 	vTaskDelay(100);
#ifdef DEBUG
	printf("LCD\n");
	fflush(stdout);
#endif
    uart_config_t lcd_uart_config = {
        .baud_rate = LCD_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

	ESP_ERROR_CHECK(uart_driver_install(LCD_UART_CHANNEL, READ_BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
	ESP_ERROR_CHECK(uart_param_config(LCD_UART_CHANNEL, &lcd_uart_config));
	ESP_ERROR_CHECK(uart_set_pin(LCD_UART_CHANNEL, LCD_UART_TX_PIN, LCD_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

#ifdef DEBUG
	printf("UHF\n");
	fflush(stdout);
#endif
    uart_config_t uhf_uart_config = {
        .baud_rate = UHF_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

	ESP_ERROR_CHECK(uart_driver_install(UHF_UART_CHANNEL, READ_BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
	ESP_ERROR_CHECK(uart_param_config(UHF_UART_CHANNEL, &uhf_uart_config));
	ESP_ERROR_CHECK(uart_set_pin(UHF_UART_CHANNEL, UHF_UART_TX_PIN, UHF_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

/**
 * UART_read: read from UART
 *
 * @param channel channel to read from.
 * @param buf result buffer.
 * @param len length of the result buffer.
 * @returns number of bytes read.
 */
int UART_read(int channel, void *buf, int len = READ_BUF_SIZE) {
	return (uart_read_bytes(channel, buf, len, 1));
}

/**
 * UART_write: write to UART
 *
 * @param channel channel to read from.
 * @param buf writing buffer.
 * @param len length of the writing buffer.
 * @returns number of bytes written.
 */
int UART_write(int channel, void *buf, int len) {
	return (uart_write_bytes(channel, serout, len));
}

/**
 * UART_debug: write a debug msg to UART
 *
 * @param buf writing buffer.
 * @param len length of the writing buffer.
 */
void UART_debug(char *buf, int len) {
	UART_write(0, (void*) buf, len);
}


