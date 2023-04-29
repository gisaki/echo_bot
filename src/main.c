#include <zephyr/kernel.h>

/* Workqueue */
#define MY_STACK_SIZE 512
#define MY_PRIORITY 5
K_THREAD_STACK_DEFINE(my_stack_area, MY_STACK_SIZE);
struct k_work_q my_work_q;
struct k_work my_work_item[3];

static void work_handler(struct k_work *work)
{
		printk("work_handler");
		k_sleep(K_MSEC(4000));
}

extern void main_echo_bot(void);
extern void main_my_uart_peripheral(void);
extern void main_cdc_acm_composite(void);
extern void main_shell_module(void);
extern void main_mcp9808(void);

void main(void)
{
	// main_echo_bot();
	main_my_uart_peripheral();
	// main_cdc_acm_composite();
	main_shell_module();
	main_mcp9808();

	k_work_queue_init(&my_work_q);
	k_work_queue_start(&my_work_q, my_stack_area,
		K_THREAD_STACK_SIZEOF(my_stack_area), MY_PRIORITY,
		NULL);
	k_work_init(&(my_work_item[0]), work_handler);
	k_work_init(&(my_work_item[1]), work_handler);
	k_work_init(&(my_work_item[2]), work_handler);
	k_work_submit_to_queue(&my_work_q, &(my_work_item[0]));
	k_work_submit_to_queue(&my_work_q, &(my_work_item[1]));
	k_work_submit_to_queue(&my_work_q, &(my_work_item[2]));
}
