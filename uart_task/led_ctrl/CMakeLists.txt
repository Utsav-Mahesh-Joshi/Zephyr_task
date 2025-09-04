cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_EXTRA_MODULES "${CMAKE_SOURCE_DIR}/../../modules/my_console")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(led_ctrl)

target_sources(app PRIVATE src/main.c)
