# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_echo_bot)

target_sources(app PRIVATE 
src/main.c
src/main_app.c
src/main_echo_bot.c
src/main_my_uart_peripheral.c
src/main_my_uart_peripheral_b.c
# src/main_cdc_acm_composite.c
src/main_cdc_acm.c
src/main_shell_module.c
src/main_mcp9808.c
src/main_dhcpv4_client.c
src/socket_dumb_http.c
src/http_get.c
)

generate_inc_file_for_target(app src/response_small.html.bin src/response_small.html.bin.inc)
generate_inc_file_for_target(app src/response_big.html.bin   src/response_big.html.bin.inc)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_inc_file_for_target(
    app
    src/globalsign_r1.der
    ${gen_dir}/globalsign_r1.der.inc
    )

add_subdirectory(drivers)
