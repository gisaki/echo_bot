/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for CDC ACM class driver
 *
 * Sample app for USB CDC ACM class driver. The received data is echoed back
 * to the serial port.
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/kernel.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdc_acm, LOG_LEVEL_INF);

static const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart1));

#define RING_BUF_SIZE	(128)
struct send_buf {
    struct ring_buf rb;
    uint32_t buffer[RING_BUF_SIZE];
};
static struct send_buf sb;

void send_cdc_acm(const char *data, size_t length)
{
	size_t wrote;
	wrote = ring_buf_put(&(sb.rb), data, length);
	if (wrote < length) {
		LOG_ERR("Drop %zu bytes", length - wrote);
	}
	if (wrote) {
		uart_irq_tx_enable(dev);
	}
}

static void interrupt_handler(const struct device *dev, void *user_data)
{
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		while (uart_irq_rx_ready(dev))
		{
	        char c;
			uart_fifo_read(dev, &c, 1);

			send_cdc_acm(&c, 1);
		}
		if (uart_irq_tx_ready(dev)) {
			uint8_t buf[16];
			size_t wrote, len;
			len = ring_buf_get(&(sb.rb), buf, sizeof(buf));
			if (!len) {
				LOG_DBG("dev %p TX buffer empty", dev);
				uart_irq_tx_disable(dev);
			} else {
				wrote = uart_fifo_fill(dev, buf, len);
				LOG_DBG("dev %p wrote len %zu", dev, wrote);
			}
		}
	} // while
}

static void uart_line_set(const struct device *dev)
{
	int ret;

	/* They are optional, we use them to test the interrupt endpoint */
	ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DCD, 1);
	if (ret) {
		LOG_DBG("Failed to set DCD, ret code %d", ret);
	}

	ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DSR, 1);
	if (ret) {
		LOG_DBG("Failed to set DSR, ret code %d", ret);
	}
}

// Wait for DTR
static struct k_work my_work;
static struct k_timer my_timer;
static void my_work_handler(struct k_work *work)
{
	uint32_t dtr = 0U;
	uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
	if (!dtr) {
		k_timer_start(&my_timer, K_MSEC(100), K_NO_WAIT);
		return;
	}

	LOG_INF("DTR set");

	uart_line_set(dev);

	uart_irq_callback_user_data_set(dev, interrupt_handler, NULL);

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);

	LOG_INF("CDC ACM device ready to use");
}
static void my_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&my_work);
}

void main_cdc_acm(void)
{
	int ret;

	if (!device_is_ready(dev)) {
		LOG_ERR("CDC ACM device not ready");
		return;
	}

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	ring_buf_init(&(sb.rb), RING_BUF_SIZE, sb.buffer);

	LOG_INF("Wait for DTR");

	{
		k_timer_init(&my_timer, my_timer_handler, NULL);
		k_work_init(&my_work, my_work_handler);
		/* start one shot timer that expires after 100 ms */
		k_timer_start(&my_timer, K_MSEC(100), K_NO_WAIT);
	}
}
