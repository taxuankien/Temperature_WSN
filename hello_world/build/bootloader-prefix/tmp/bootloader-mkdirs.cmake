# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/bootloader/subproject"
  "D:/ESP_Project/Temperature_WSN/hello_world/build/bootloader"
  "D:/ESP_Project/Temperature_WSN/hello_world/build/bootloader-prefix"
  "D:/ESP_Project/Temperature_WSN/hello_world/build/bootloader-prefix/tmp"
  "D:/ESP_Project/Temperature_WSN/hello_world/build/bootloader-prefix/src/bootloader-stamp"
  "D:/ESP_Project/Temperature_WSN/hello_world/build/bootloader-prefix/src"
  "D:/ESP_Project/Temperature_WSN/hello_world/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/ESP_Project/Temperature_WSN/hello_world/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/ESP_Project/Temperature_WSN/hello_world/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
