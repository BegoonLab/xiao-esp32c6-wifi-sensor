cmake_minimum_required(VERSION 3.16)

execute_process(
  COMMAND git rev-parse --short HEAD
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

set(PROJECT_VER "1.0")
set(PROJECT_VER_NUMBER 1)

set(ESP_MATTER_PATH "/opt/esp/esp-matter")
set(MATTER_SDK_PATH ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
    "${ESP_MATTER_PATH}/examples/common"
    "${MATTER_SDK_PATH}/config/esp32/components"
    "${ESP_MATTER_PATH}/components" ${extra_components_dirs_append})

project(xiao_esp32c6_wifi_sensor)

idf_build_set_property(
  CXX_COMPILE_OPTIONS
  "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H;-Wno-overloaded-virtual" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)
# For RISCV chips, project_include.cmake sets -Wno-format, but does not clear
# various flags that depend on -Wformat
idf_build_set_property(COMPILE_OPTIONS
                       "-Wno-format-nonliteral;-Wno-format-security" APPEND)
