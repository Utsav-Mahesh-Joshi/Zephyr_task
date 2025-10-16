cmake_minimum_required(VERSION 3.20.0)
set(ZEPHYR_EXTRA_MODULES "${CMAKE_SOURCE_DIR}/../../modules/my_hd44780_pcf8574_mod")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lcd_sens)

#include_directories(include)
target_sources(app PRIVATE src/main.c)
#include_directories(include)

