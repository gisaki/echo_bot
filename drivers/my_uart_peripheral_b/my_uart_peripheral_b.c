
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

#include <errno.h>

#include "my_uart_peripheral_b_api.h"

// Compatible with "mycompany,my_uart_peripheral_b"
#define DT_DRV_COMPAT mycompany_my_uart_peripheral_b
#define MY_UART_PERIPHERAL_B_INIT_PRIORITY 41

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(my_uart_peripheral_b, CONFIG_MY_UART_PERIPHERAL_B_LOG_LEVEL);

/**
 * @brief Contains runtime mutable data for the UART peripheral.
 */
struct my_uart_data
{
    uint8_t rx_buf[CONFIG_MY_UART_PERIPHERAL_B_RX_BUF_SIZE]; // Buffer to hold received data.
    size_t rx_data_len;                                      // Length of currently recieved data.
    my_uart_peripheral_b_callback_t callback;                // Callback function to be called when a string is received.
};

/**
 * @brief Build time configurations for the UART peripheral.
 */
struct my_uart_conf
{
    struct my_uart_data *data;           // Pointer to runtime data.
    const struct device *uart_dev;       // UART device.
    const struct gpio_dt_spec gpio_spec; // GPIO spec for pin used to start transmitting.
};

/**
 * @brief Set callback function to be called when a string is received.
 * 
 * @param dev UART peripheral device.
 * @param callback New callback function.
 */
static void user_set_callback(const struct device *dev, my_uart_peripheral_b_callback_t callback)
{
    struct my_uart_data *data = (struct my_uart_data *)dev->data;
    data->callback = callback;
}

const static struct my_uart_peripheral_b_api api_b = {
    .set_callback = user_set_callback,
};

/**
 * @brief Handles UART interrupts.
 * 
 * @param uart_dev UART bus device.
 * @param user_data Custom data. Should be a pointer to a my_uart_peripheral device.
 */
static void uart_int_handler(const struct device *uart_dev, void *user_data)
{
    uart_irq_update(uart_dev);
    const struct device *dev = (const struct device *)user_data; // Uart peripheral device.
    struct my_uart_data *data = dev->data;
    if (uart_irq_rx_ready(uart_dev))
    {
        my_uart_peripheral_b_callback_t callback = data->callback;
        char c;
        while (!uart_poll_in(uart_dev, &c))
        {
            data->rx_buf[data->rx_data_len] = c;
            data->rx_data_len++;
            size_t rx_buf_capacity = CONFIG_MY_UART_PERIPHERAL_B_RX_BUF_SIZE - data->rx_data_len;
            if ( (c == '\n') || (c == '\r') ) // terminate found.
            {
                if (callback != NULL)
                {
                    callback(dev, data->rx_buf, data->rx_data_len);
                }
                data->rx_data_len = 0;
                memset(data->rx_buf, 0, sizeof(data->rx_buf));
            }
            else if (rx_buf_capacity == 0) // Buffer full. No string found.
            {
                // skip received data (no action)
                data->rx_data_len = 0;
                memset(data->rx_buf, 0, sizeof(data->rx_buf));
            }
        }
    }
}

/**
 * @brief Initializes the uart bus for the UART peripheral.
 * 
 * @param dev UART peripheral device.
 * @return 0 if successful, negative errno value otherwise.
 */
static int init_uart(const struct device *dev)
{
    struct my_uart_conf *conf = (struct my_uart_conf *)dev->config;
    uart_irq_callback_user_data_set(conf->uart_dev, uart_int_handler, (void *)dev);
    uart_irq_rx_enable(conf->uart_dev);
    return 0;
}

/**
 * @brief Initializes GPIO for the UART peripheral.
 * 
 * @param dev UART peripheral device.
 * @return 0 on success, negative errno value on failure.
 */
static int init_gpio(const struct device *dev)
{
    struct my_uart_conf *conf = (struct my_uart_conf *)dev->config;
    int ret;
	ret = gpio_pin_configure_dt(&conf->gpio_spec, GPIO_OUTPUT_ACTIVE);
    if (ret)
        return ret;
    return ret;
}

/**
 * @brief Initializes UART peripheral.
 * 
 * @param dev UART peripheral device.
 * @return 0 on success, negative error code otherwise.
 */
static int init_my_uart_peripheral_b(const struct device *dev)
{
    struct my_uart_conf *conf = (struct my_uart_conf *)dev->config;
    if (!device_is_ready(conf->uart_dev))
    {
        __ASSERT(false, "UART device not ready");
        return -ENODEV;
    }
    if (init_uart(dev))
    {
        __ASSERT(false, "Failed to initialize UART device");
        return -ENODEV;
    }
    if (!device_is_ready(conf->gpio_spec.port))
    {
        __ASSERT(false, "GPIO device not ready");
        return -ENODEV;
    }
    if (init_gpio(dev))
    {
        __ASSERT(false, "Failed to initialize GPIO device.");
        return -ENODEV;
    }
    LOG_DBG("My UART peripheral b initialized");
    return 0;
}

#define INIT_MY_UART_PERIPHERAL_B(inst)                           \
    static struct my_uart_data my_uart_peripheral_b_data_##inst = { \
        .callback = NULL,                                         \
        .rx_data_len = 0,                                         \
        .rx_buf = {0},                                            \
    };                                                            \
    static struct my_uart_conf my_uart_peripheral_b_conf_##inst = { \
        .data = &my_uart_peripheral_b_data_##inst,                \
        .uart_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),             \
        .gpio_spec = GPIO_DT_SPEC_INST_GET(inst, ctrl_gpios),     \
    };                                                            \
    DEVICE_DT_INST_DEFINE(inst,                                   \
                          init_my_uart_peripheral_b,              \
                          NULL,                                   \
                          &my_uart_peripheral_b_data_##inst,      \
                          &my_uart_peripheral_b_conf_##inst,      \
                          POST_KERNEL,                            \
                          MY_UART_PERIPHERAL_B_INIT_PRIORITY,     \
                          &api_b);

DT_INST_FOREACH_STATUS_OKAY(INIT_MY_UART_PERIPHERAL_B);
