@echo off
cd /D D:\ESP_Project\Temperature_WSN\ds18b20_test\build\esp-idf\esp_system || (set FAIL_LINE=2& goto :ABORT)
D:\esp8266\esp32BleMesh\myMesh\Espressif\python_env\idf5.1_py3.11_env\Scripts\python.exe D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/tools/ldgen/ldgen.py --config D:/ESP_Project/Temperature_WSN/ds18b20_test/sdkconfig --fragments-list D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/xtensa/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_ringbuf/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/driver/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_pm/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_mm/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/spi_flash/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_system/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_system/app.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_rom/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/hal/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/log/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/heap/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/soc/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_hw_support/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/freertos/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/freertos/linker_common.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/newlib/newlib.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/newlib/system_libs.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_common/common.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_common/soc.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/app_trace/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_event/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_phy/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/lwip/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_netif/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_wifi/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/bt/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_adc/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_gdbstub/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_psram/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_lcd/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/espcoredump/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/ieee802154/linker.lf;D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/openthread/linker.lf --input D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/components/esp_system/ld/esp32/sections.ld.in --output D:/ESP_Project/Temperature_WSN/ds18b20_test/build/esp-idf/esp_system/ld/sections.ld --kconfig D:/esp8266/esp32BleMesh/myMesh/Espressif/frameworks/esp-idf-v5.1.1/Kconfig --env-file D:/ESP_Project/Temperature_WSN/ds18b20_test/build/config.env --libraries-file D:/ESP_Project/Temperature_WSN/ds18b20_test/build/ldgen_libraries --objdump D:/esp8266/esp32BleMesh/myMesh/Espressif/tools/xtensa-esp32-elf/esp-12.2.0_20230208/xtensa-esp32-elf/bin/xtensa-esp32-elf-objdump.exe || (set FAIL_LINE=3& goto :ABORT)
goto :EOF

:ABORT
set ERROR_CODE=%ERRORLEVEL%
echo Batch file failed at line %FAIL_LINE% with errorcode %ERRORLEVEL%
exit /b %ERROR_CODE%