/* 
   This is example for my DS18B20 library
   https://github.com/feelfreelinux/ds18b20
   

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "ds18b20.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "driver/rtc_io.h"
float get_average_temp(void);
void getTempAddresses(DeviceAddress *tempSensorAddresses);
#define TAG "EXAMPLE"
const int DS_PIN = 26; //GPIO where you connected ds18b20
DeviceAddress tempSensors[1];
gpio_num_t ledWaring = 25;
void mainTask(void *pvParameters){
  rtc_gpio_init(ledWaring);
  rtc_gpio_set_direction(ledWaring, GPIO_MODE_OUTPUT);
  while (1) {
    rtc_gpio_set_level(ledWaring, 1);
    rtc_gpio_hold_en(ledWaring);
    esp_sleep_enable_timer_wakeup(5 * 1000000);
    esp_deep_sleep_start();
  }
}
void app_main()
{
    nvs_flash_init();
    xTaskCreatePinnedToCore(&mainTask, "mainTask", 2048, NULL, 5, NULL, 0);
}
