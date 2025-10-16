cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(wifi_con)
include_directories(include)
include_directories(/home/uj/zephyrproject/zephyr/drivers/wifi/eswifi)
#target_sources(app PRIVATE src/main.c src/wifi.c )
target_sources(app PRIVATE src/main.c src/ping.c)
