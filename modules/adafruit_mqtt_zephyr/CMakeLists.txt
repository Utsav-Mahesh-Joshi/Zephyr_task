# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

# This library adapts the Adafruit MQTT API to Zephyr’s native MQTT client.
project(adafruit_mqtt_zephyr)

# Build only if enabled in Kconfig (defaults to y in the upcoming Kconfig file).
if(NOT CONFIG_ADAFRUIT_MQTT_ZEPHYR)
  return()
endif()

# Create a named Zephyr library for the adapter
zephyr_library_named(adafruit_mqtt_zephyr)

# Public headers
zephyr_include_directories(
  include
)

# Core adapter sources (will be provided in subsequent files)
# - adafruit_mqtt_zephyr.c: thin adapter mapping Adafruit API -> Zephyr MQTT
# - adafruit_mqtt_client_zephyr.c: connection/socket glue, keepalive, callbacks
# - adafruit_mqtt_topics.c: small helpers for topic and packet utilities
zephyr_library_sources(
  src/adafruit_mqtt_zephyr.c
  src/adafruit_mqtt_client_zephyr.c
  src/adafruit_mqtt_topics.c
)

# Link against Zephyr’s networking/MQTT stack
# (headers: <zephyr/net/mqtt.h>, sockets, logging)
target_link_libraries(adafruit_mqtt_zephyr PUBLIC zephyr_interface)

