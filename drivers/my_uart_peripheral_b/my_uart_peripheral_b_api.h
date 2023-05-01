#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifndef MY_UART_PERIPHERAL_B_H
#define MY_UART_PERIPHERAL_B_H

// Callback
typedef void (*my_uart_peripheral_b_callback_t)(const struct device *dev, char *data, size_t length);

// Set the data callback function for the device
typedef void (*my_uart_peripheral_b_set_callback_t)(const struct device *dev, my_uart_peripheral_b_callback_t callback);

struct my_uart_peripheral_b_api
{
    my_uart_peripheral_b_set_callback_t set_callback;
};

/**
 * @brief Set the data callback function for the device
 * 
 * @param dev Pointer to the device structure.
 * @param callback Callback function pointer.
 */
static inline void my_uart_set_callback(const struct device *dev, my_uart_peripheral_b_callback_t callback)
{
    struct my_uart_peripheral_b_api *api = (struct my_uart_peripheral_b_api *)dev->api;
    return api->set_callback(dev, callback);
}

#endif //MY_UART_PERIPHERAL_B_H