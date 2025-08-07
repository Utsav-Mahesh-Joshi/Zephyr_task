#check if MY_CONSOLE is turned on
if(CONFIG_MY_CONSOLE)
	# ADD SOURCE DIRECTORY
	#ADD SOURCE FILE YOU WANT TO COMPILE
	zephyr_include_directories(.)
	zephyr_library_sources(my_console.c)
	#target_sources(app PRIVATE my_console.c)
endif()
