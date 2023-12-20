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
float get_average_temp(void);
void getTempAddresses(DeviceAddress *tempSensorAddresses);
#define TAG "EXAMPLE"
const int DS_PIN = 26; //GPIO where you connected ds18b20
DeviceAddress tempSensors[1];

void mainTask(void *pvParameters){
  ds18b20_init(DS_PIN);
  getTempAddresses(tempSensors);
  ds18b20_setResolution(tempSensors,1,12);
  while (1) {
    float temp = get_average_temp();
    ESP_LOGI(TAG, "Temperature Average: %0.3f\n",temp);
    esp_sleep_enable_timer_wakeup(5 * 1000000);
    esp_light_sleep_start();
  }
}
void app_main()
{
    nvs_flash_init();
    xTaskCreatePinnedToCore(&mainTask, "mainTask", 2048, NULL, 5, NULL, 0);
}

float get_average_temp(void){
  float temp_average = 0;
  int i = 0;
  TickType_t startTime = xTaskGetTickCount();
  while(1){
    float temp = ds18b20_get_temp();
    temp_average += temp;
    ESP_LOGI(TAG, "Temperature: %0.3f\n",temp);
    vTaskDelay(1/ portTICK_RATE_MS);
    i++;
    if ((xTaskGetTickCount() - startTime) >= pdMS_TO_TICKS(3000)) {
      break;
    }
  }
  temp_average = temp_average / i;
  return temp_average;
}

void getTempAddresses(DeviceAddress *tempSensorAddresses) {
	unsigned int numberFound = 0;
	reset_search();
	while (search(tempSensorAddresses[numberFound],true)) {
		numberFound++;
		if (numberFound == 1) break;
	}
	while (numberFound != 1) {
		numberFound = 0;
		reset_search();
		while (search(tempSensorAddresses[numberFound],true)) {
			numberFound++;
			if (numberFound == 1) break;
		}
	}
	return;
}