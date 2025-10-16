# build as a Zephyr library, conditionally on Kconfig
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_HD44780_PCF8574
	hd44780_pcf8574.c
)

