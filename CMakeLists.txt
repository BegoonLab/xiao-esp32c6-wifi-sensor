cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS vendor/esp-idf-lib/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiao_esp32c6_wifi_sensor)