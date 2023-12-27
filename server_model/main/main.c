#include <sdkconfig.h>
#include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "mesh_server.h"
#include "custom_sensor_model_defs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ds18b20.h"
#include "LCD1602.h"

static const char* TAG = "MESH SERVER";
const int DS_PIN = 26; //GPIO where you connected ds18b20
DeviceAddress tempSensors[1];
TaskHandle_t MainTask = NULL;
esp_timer_handle_t oneshot_timer;
esp_timer_handle_t periodic_timer;

// QueueHandle_t ble_mesh_received_data_queue = NULL;
model_sensor_data_t _server_model_state = {
    .device_name = "id_3",
};

void LCD_display(float a){
	
	char str[15];
	lcd_clear();
	lcd_put_cur(0, 0);
	lcd_send_string("Temperature is:");
	
	lcd_put_cur(1, 0);
	sprintf(str, "%.1f", a);
	lcd_send_string(str);
	
}

void getTempAddresses(DeviceAddress *tempSensorAddresses) {
	unsigned int numberFound = 0;
	reset_search();
    ESP_LOGI(TAG, "3");
	while (search(tempSensorAddresses[numberFound],true)) {
		numberFound++;
        ESP_LOGI(TAG, "4");
		if (numberFound == 1) break;
	}
	while (numberFound != 1) {
		numberFound = 0;
		reset_search();
        ESP_LOGI(TAG, "5");
		while (search(tempSensorAddresses[numberFound],true)) {
			numberFound++;
			if (numberFound == 1) break;
		}
	}
}

float get_average_temp(void){
  float temp_average = 0;
  int i = 0;
  TickType_t startTime = xTaskGetTickCount();
  while(1){
    float temp = ds18b20_get_temp();
    temp_average += temp;
    ESP_LOGD(TAG, "Temperature: %f\n",temp);
    // vTaskDelay(1/ portTICK_RATE_MS);
    i++;
    if ((xTaskGetTickCount() - startTime) >= pdMS_TO_TICKS(3000)) {
      break;
    }
  }
  temp_average = temp_average / i;
  return temp_average;
}

void temperature_sensing(void *arg){

    while(!is_server_provisioned()){};
    
    lcd_clear();
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 10000000));
	while (1){
        if(is_server_provisioned()){
            ESP_LOGD(TAG, "Periodic timer called, time since boot: %lld us", esp_timer_get_time());
            ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer,4000000));
            _server_model_state.temperature = get_average_temp();
            LCD_display(_server_model_state.temperature);
            ESP_LOGI(TAG, "LCD display success! Average Temp: %.1f ", _server_model_state.temperature);
            ESP_LOGD(TAG, "Sending ....., time since boot: %lld us", esp_timer_get_time());
            server_send_to_client(_server_model_state);
            ESP_LOGD(TAG, "Sent!!, time since boot: %lld us", esp_timer_get_time());
        }
        vTaskSuspend(MainTask);
	}

}

static void periodic_timer_callback(void *arg){
	vTaskResume(MainTask);

}

static void oneshot_timer_callback(void *arg){
	int64_t time_since_boot = esp_timer_get_time();

	ESP_LOGI(TAG, "One-shot timer called, time since boot: %lld us", time_since_boot);
	esp_light_sleep_start();
}

// static void read_received_items(void *arg) {
//     ESP_LOGI(TAG, "Task initializing..");

//     model_sensor_data_t _received_data;
    

//     while (1) {
//         vTaskDelay(500 / portTICK_PERIOD_MS);
        
//         if (xQueueReceive(ble_mesh_received_data_queue, &_received_data, 1000 / portTICK_PERIOD_MS) == pdPASS) {
//             ESP_LOGI("PARSED DATA", "Warning low = %f", _received_data.low_bsline);
//             ESP_LOGI("PARSED DATA", "Warning high = %f", _received_data.high_bsline);
        
//             // _server_model_state.high_bsline = _received_data.high_bsline;
//             // _server_model_state.low_bsline = _received_data.high_bsline;
//             memcpy((void *)& _server_model_state.high_bsline, (void * )&_received_data.high_bsline , sizeof(_received_data.high_bsline));
//             memcpy((void *)& _server_model_state.low_bsline, (void * )&_received_data.low_bsline, sizeof(_received_data.high_bsline) );
//         }
//     }   
// } 

static void server_send_status(void *arg){
    uint8_t tick = 0;
    ESP_LOGI(TAG, "STATUS message is sending.");

    while (1){
        vTaskDelay(1000/ portTICK_PERIOD_MS);

        if(tick++ >= 3 && is_server_provisioned()){
            server_send_to_client(_server_model_state);
            tick = 0;
        }
    }
}

void app_main(void) {
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing...");

    // ble_mesh_received_data_queue = xQueueCreate(5, sizeof(model_sensor_data_t));

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(i2c_master_init());
    
	
    lcd_init();
    
    ESP_LOGD(TAG, "I2C initialized successfully");
	ds18b20_init(DS_PIN);
    ESP_LOGI(TAG, "1");
    getTempAddresses(tempSensors);
    ESP_LOGI(TAG, "2");
    ds18b20_setResolution(tempSensors,1,12);
	ESP_LOGI(TAG, "Sensor initialized successfully");
    // err = ble_mesh_device_init_server();
    err = ble_mesh_device_init_server();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err 0x%06x)", err);
    }
    
    const esp_timer_create_args_t oneshot_timer_args = {
		.callback = &oneshot_timer_callback,
		.name = "light sleep"
	};

	ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));
    
	const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };

    
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    
    

	ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(6000000));
	
    xTaskCreatePinnedToCore(&temperature_sensing, "main task", 1024 * 2, (void *)0, tskIDLE_PRIORITY, &MainTask, (int)1);
    // xTaskCreate(read_received_items, "setting message", 1024 * 2, (void *)0, 1, NULL);

}