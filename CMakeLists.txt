cmake_minimum_required(VERSION 3.16)

execute_process(
  COMMAND git rev-parse --short HEAD
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiao_esp32c6_wifi_sensor)

idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)
