#include <zephyr/kernel.h>
#include <string.h>

#define MSG_BODY_SIZE 64
struct data_item_with_size_and_type {
    size_t size;
    uint16_t type;
    uint8_t data[MSG_BODY_SIZE];
};
BUILD_ASSERT(sizeof(struct data_item_with_size_and_type) % 4 == 0);

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(app_msgq, sizeof(struct data_item_with_size_and_type), 10, 4);

void send_app(uint8_t *data, size_t size, uint16_t type)
{
	struct data_item_with_size_and_type tx_buf = {0};
    if (size > sizeof(tx_buf.data)){
        printk("send_app, size over, drop");
        return;
    }
    tx_buf.size = size;
    tx_buf.type = type;
    memcpy(tx_buf.data, data, size);
    /* if queue is full, message is silently dropped */
    k_msgq_put(&app_msgq, &tx_buf, K_NO_WAIT);
}

void recv_app(uint8_t *data, size_t size, uint16_t type)
{
    printk("recv_app, size %d, type %d\n", size, type);
    printk("recv_app, data = %s\n", data);
}

#define STACKSIZE             (1024)
#define PRIORITY             (7)
K_THREAD_STACK_DEFINE(thread_app_stack_area, STACKSIZE);
static struct k_thread thread_app_data;
void thread_app(void *dummy1, void *dummy2, void *dummy3)
{
	struct data_item_with_size_and_type rx_buf;

	/* indefinitely wait for input from the user */
	while (k_msgq_get(&app_msgq, &rx_buf, K_FOREVER) == 0) {
        size_t size = rx_buf.size;
        uint16_t type = rx_buf.type;
        recv_app(rx_buf.data, size, type);
	}
}

void main_app(void)
{
    k_thread_create(&thread_app_data, thread_app_stack_area,
            K_THREAD_STACK_SIZEOF(thread_app_stack_area),
            thread_app, NULL, NULL, NULL,
            PRIORITY, 0, K_FOREVER);
	k_thread_start(&thread_app_data);
}
