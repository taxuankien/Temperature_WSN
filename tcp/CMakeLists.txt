
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common
                         $ENV{IDF_PATH}/examples/bluetooth/esp_ble_mesh/common_components/example_init
                         $ENV{IDF_PATH}/examples/bluetooth/esp_ble_mesh/common_components/example_nvs)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(project)
