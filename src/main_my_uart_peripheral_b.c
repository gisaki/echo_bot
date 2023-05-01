#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include "../drivers/my_uart_peripheral_b/my_uart_peripheral_b_api.h"

void app_callback(const struct device *dev, char *data, size_t length)
{
    printk("my_uart_peripheral_b, app_callback, length=%d\n", length);
}

void main_my_uart_peripheral_b(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(my_device_b));
	if (!device_is_ready(dev)) {
		printk("DEV device not found!");
		return;
	}
    my_uart_set_callback(dev, app_callback);
}
