# expose public headers from this module
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

# pull driver library cmake
add_subdirectory(drivers)

