#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include "../drivers/my_uart_peripheral_b/my_uart_peripheral_b_api.h"

static const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(my_device_b));

void app_send_data(char *data, size_t length)
{
    my_uart_send_data(dev, data, length);
}

void app_callback(const struct device *dev, char *data, size_t length)
{
    printk("my_uart_peripheral_b, app_callback, length=%d\n", length);
	app_send_data(data, length);
}

void main_my_uart_peripheral_b(void)
{
	if (!device_is_ready(dev)) {
		printk("DEV device not found!");
		return;
	}
    my_uart_set_callback(dev, app_callback);
}
