/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include "../drivers/my_uart_peripheral/api.h"

struct counter
{
    int strings;
    int overflows;
};

static struct counter tracker = {0};

void peripheral_callback(const struct device *dev, char *data, size_t length, bool is_string, void *user_data)
{
    struct counter *c = (struct counter *)user_data;
    if (is_string)
    {
        printk("Recieved string \"%s\"\n", data);
        c->strings++;
    } else {
        printk("Buffer full. Recieved fragment %.*s\n", length, data);
        c->overflows++;
    }
    printk("Strings: %d\nOverflows: %d\n", c->strings, c->overflows);
}

void main_my_uart_peripheral(void)
{
    const struct device *dev2 = device_get_binding(DT_PROP(DT_NODELABEL(my_device),label));
	if (!device_is_ready(dev2)) {
		printk("DEV2 device not found!");
		return;
	}
    my_uart_set_callback(dev2, peripheral_callback, &tracker);
}
