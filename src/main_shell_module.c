/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <version.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <ctype.h>

#ifdef CONFIG_ARCH_POSIX
#include <unistd.h>
#else
#include <zephyr/posix/unistd.h>
#endif

LOG_MODULE_REGISTER(app);

static int cmd_demo_ping(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "pong");

	return 0;
}

static int cmd_demo_board(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, CONFIG_BOARD);

	return 0;
}

static int cmd_demo_params(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "argc = %zd", argc);
	for (size_t cnt = 0; cnt < argc; cnt++) {
		shell_print(shell, "  argv[%zd] = %s", cnt, argv[cnt]);
	}

	return 0;
}

static int cmd_demo_hexdump(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "argc = %zd", argc);
	for (size_t cnt = 0; cnt < argc; cnt++) {
		shell_print(shell, "argv[%zd]", cnt);
		shell_hexdump(shell, argv[cnt], strlen(argv[cnt]));
	}

	return 0;
}

static int cmd_version(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Zephyr version %s", KERNEL_VERSION_STRING);

	return 0;
}

static int cmd_dict(const struct shell *shell, size_t argc, char **argv,
		    void *data)
{
	int val = (intptr_t)data;

	shell_print(shell, "(syntax, value) : (%s, %d)", argv[0], val);

	return 0;
}

SHELL_SUBCMD_DICT_SET_CREATE(sub_dict_cmds, cmd_dict,
	(value_0, 0, "value 0"), (value_1, 1, "value 1"),
	(value_2, 2, "value 2"), (value_3, 3, "value 3")
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_demo,
	SHELL_CMD(dictionary, &sub_dict_cmds, "Dictionary commands", NULL),
	SHELL_CMD(hexdump, NULL, "Hexdump params command.", cmd_demo_hexdump),
	SHELL_CMD(params, NULL, "Print params command.", cmd_demo_params),
	SHELL_CMD(ping, NULL, "Ping command.", cmd_demo_ping),
	SHELL_CMD(board, NULL, "Show board name command.", cmd_demo_board),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(demo, &sub_demo, "Demo commands", NULL);

SHELL_CMD_ARG_REGISTER(version, NULL, "Show kernel version", cmd_version, 1, 0);

/* Create a set of commands. Commands to this set are added using @ref SHELL_SUBCMD_ADD
 * and @ref SHELL_SUBCMD_COND_ADD.
 */
SHELL_SUBCMD_SET_CREATE(sub_section_cmd, (section_cmd));

static int cmd1_handler(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "cmd1 executed");

	return 0;
}

/* Create a set of subcommands for "section_cmd cm1". */
SHELL_SUBCMD_SET_CREATE(sub_section_cmd1, (section_cmd, cmd1));

/* Add command to the set. Subcommand set is identify by parent shell command. */
SHELL_SUBCMD_ADD((section_cmd), cmd1, &sub_section_cmd1, "help for cmd1", cmd1_handler, 1, 0);

SHELL_CMD_REGISTER(section_cmd, &sub_section_cmd,
		   "Demo command using section for subcommand registration", NULL);

#define STACKSIZE             (1024)
#define PRIORITY             (7)
K_THREAD_STACK_DEFINE(thread_shell_module_stack_area, STACKSIZE);
static struct k_thread thread_shell_module_data;
void thread_shell_module(void *dummy1, void *dummy2, void *dummy3)
{
	printk("start !");
	if (IS_ENABLED(CONFIG_SHELL_START_OBSCURED)) {
		login_init();
	}

#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
	const struct device *dev;
	uint32_t dtr = 0;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	if (!device_is_ready(dev) || usb_enable(NULL)) {
		return;
	}

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}
#endif
}

void main_shell_module(void)
{
    k_thread_create(&thread_shell_module_data, thread_shell_module_stack_area,
            K_THREAD_STACK_SIZEOF(thread_shell_module_stack_area),
            thread_shell_module, NULL, NULL, NULL,
            PRIORITY, 0, K_FOREVER);
	k_thread_start(&thread_shell_module_data);
}
