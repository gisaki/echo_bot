#include <zephyr/kernel.h>

extern void main_echo_bot(void);
extern void main_my_uart_peripheral(void);
extern void main_cdc_acm_composite(void);
extern void main_mcp9808(void);

void main(void)
{
	main_echo_bot();
	main_my_uart_peripheral();
	main_cdc_acm_composite();
	main_mcp9808();
}
