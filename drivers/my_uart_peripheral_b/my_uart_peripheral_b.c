
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
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

    struct ring_buf tx_rb;
    uint32_t tx_rb_buffer[CONFIG_MY_UART_PERIPHERAL_B_TX_BUFFERING_BUF_WORDS];

    // A mechanism waiting for a specific response before sending next data
    bool sending_guard;
    struct k_timer sending_guard_timer;
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

static bool check_sending_guard(const char *data, size_t length)
{
    if (length < 2) {return false;}
    if (memcmp(data, "OK", 2) == 0) {return true;}
    return false;
}

static void sending_guard_timer_expired_function(struct k_timer *timer_id)
{
    const struct device *dev = (const struct device *)k_timer_user_data_get(timer_id);
    struct my_uart_conf *conf = (struct my_uart_conf *)dev->config;

    // Release transmission guard
    conf->data->sending_guard = false;
    
	uart_irq_tx_enable(conf->uart_dev);
}

static size_t build_user_send_data(uint8_t *buf, size_t buf_size, const char *data, size_t length) {
    // do something
    memcpy(buf+0, "SEND:", 5);
    memcpy(buf+5, data, length);
    return length + 5;
}

/**
 * @brief Send data by the UART peripheral.
 * 
 * @param dev UART peripheral device.
 * @param data data to be transmitted.
 * @param length Length of data.
 */
static void user_send_data(const struct device *dev, const char *data, size_t length)
{
    struct my_uart_conf *conf = (struct my_uart_conf *)dev->config;
    {
    	uint16_t type = 0; // dummy
    	uint8_t value;
    	uint32_t buf[CONFIG_MY_UART_PERIPHERAL_B_TX_BUF_WORDS];
        size_t buf_size = CONFIG_MY_UART_PERIPHERAL_B_TX_BUF_WORDS * sizeof(uint32_t);
		int ret;

        // do somrthing
        size_t size = build_user_send_data((uint8_t *)buf, buf_size, data, length);

        uint8_t size32 = (size / sizeof(uint32_t)) + 1;
        value = size32 * sizeof(uint32_t) - size; // A variable that indicates how much less than the total size managed by the ring buffer of item mode // tell length for ring_buf_item_get

		ret = ring_buf_item_put(&(conf->data->tx_rb), type, value, buf, size32);
		if (ret == -EMSGSIZE) {
		    /* not enough room for the data item */
            LOG_DBG("My UART peripheral b called user_send_data, not enough buffer space");
			return;
		}
    }
    LOG_DBG("My UART peripheral b called user_send_data");
	uart_irq_tx_enable(conf->uart_dev);
}

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
    .send_data = user_send_data,
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
    const struct device *dev = (const struct device *)user_data; // Uart peripheral device.
    struct my_uart_conf *conf = (struct my_uart_conf *)dev->config;
    struct my_uart_data *data = conf->data;

	while (uart_irq_update(uart_dev) && uart_irq_is_pending(uart_dev)) {

		while (uart_irq_rx_ready(uart_dev))
        {
	        char c;
		    my_uart_peripheral_b_callback_t callback = data->callback;
			uart_fifo_read(uart_dev, &c, 1);
            data->rx_buf[data->rx_data_len] = c;
            data->rx_data_len++;
            size_t rx_buf_capacity = CONFIG_MY_UART_PERIPHERAL_B_RX_BUF_SIZE - data->rx_data_len;
            if ( (c == '\n') || (c == '\r') ) // terminate found.
            {
                if (check_sending_guard(data->rx_buf, data->rx_data_len)){
                    data->sending_guard = false;
                	uart_irq_tx_enable(conf->uart_dev);
                }
                
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
        } // while

		if (uart_irq_tx_ready(uart_dev)) {
            if (data->sending_guard) {
                LOG_DBG("My UART peripheral b, uart_int_handler, tx, sending_guard, hold sending data");
                uart_irq_tx_disable(uart_dev);
            } else {
                uint32_t my_data[CONFIG_MY_UART_PERIPHERAL_B_TX_BUF_WORDS];
                uint16_t my_type;
                uint8_t  my_value; // told length by ring_buf_item_put
                uint8_t  my_size;
                int ret;
                my_size = CONFIG_MY_UART_PERIPHERAL_B_TX_BUF_WORDS;
                ret = ring_buf_item_get(&(data->tx_rb), &my_type, &my_value, my_data, &my_size);
                if (ret == -EMSGSIZE) {
                    printk("Buffer is too small, need %d uint32_t\n", my_size);
                    uart_irq_tx_disable(uart_dev);
                } else if (ret == -EAGAIN) {
                    printk("Ring buffer is empty\n");
                    uart_irq_tx_disable(uart_dev);
                } else {
                    printk("Got item of type %u value %u of size %u dwords\n",
                            my_type, my_value, my_size);
                    // my_size: A variable that indicates how much less than the total size managed by the ring buffer of item mode
                    size_t size = my_size * sizeof(uint32_t) - my_value; // size indicates data length in the item
                    uart_fifo_fill(uart_dev, (uint8_t *)my_data, size);

                    data->sending_guard = true;
                    /* start one shot timer that expires after 5000 ms */
                    k_timer_start(&(data->sending_guard_timer), K_MSEC(5000), K_NO_WAIT);
                }
            }
		}
	} // while
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
    ring_buf_item_init(&(conf->data->tx_rb), CONFIG_MY_UART_PERIPHERAL_B_TX_BUFFERING_BUF_WORDS, conf->data->tx_rb_buffer);
    k_timer_init(&(conf->data->sending_guard_timer), sending_guard_timer_expired_function, NULL);
    k_timer_user_data_set(&(conf->data->sending_guard_timer), (void *)dev); // Associate user-specific data with a timer to reference "dev" in the timer handler.
    LOG_DBG("My UART peripheral b initialized");
    return 0;
}

#define INIT_MY_UART_PERIPHERAL_B(inst)                           \
    static struct my_uart_data my_uart_peripheral_b_data_##inst = { \
        .callback = NULL,                                         \
        .rx_data_len = 0,                                         \
        .rx_buf = {0},                                            \
        .sending_guard = false,                                   \
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
