#include <sdkconfig.h>
#include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "mesh_server.h"
#include "esp_bt.h"
#include "custom_sensor_model_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "ds18b20.h"
#include "LCD1602.h"

static const char* TAG = "MESH SERVER";
const int DS_PIN = 15; // GPIO where you connected ds18b20
DeviceAddress tempSensors[1];
TaskHandle_t MainTask = NULL;
esp_timer_handle_t oneshot_timer;
esp_timer_handle_t periodic_timer;

gpio_num_t ledWaring[3] = {25, 26, 27};
uint16_t count=0;
uint8_t check =1;

// QueueHandle_t ble_mesh_received_data_queue = NULL;
model_sensor_data_t _server_model_state = {
    .device_name = "1",
};

float readDataFromFlash(const char *key);
void saveDataToFlash(float data, const char *key);

void initWarningLed(){
    for(int i = 0; i < 3; i++){
        rtc_gpio_init(ledWaring[i]);
        rtc_gpio_set_direction(ledWaring[i], GPIO_MODE_OUTPUT);
    }
}

void LCD_display(float a){
	
	char str[15];
	lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("Temperature is:");

    lcd_put_cur(1, 0);
    sprintf(str, "%.1f", a);
    lcd_send_string(str);
}

void setLedWarningLevel(uint8_t redLed, uint8_t greenLed, uint8_t yellowLed){
    rtc_gpio_set_level(ledWaring[0], redLed);
    rtc_gpio_hold_en(ledWaring[0]);
    rtc_gpio_set_level(ledWaring[1], greenLed);
    rtc_gpio_hold_en(ledWaring[1]);
    rtc_gpio_set_level(ledWaring[2], yellowLed);
    rtc_gpio_hold_en(ledWaring[2]);
}

void warningLed(model_sensor_data_t data){
    if (data.temperature < data.low_bsline){
        setLedWarningLevel(0, 0, 1);
    }
    else if(data.temperature > data.high_bsline){
        setLedWarningLevel(1, 0, 0);
    }
    else{
        setLedWarningLevel(0, 1, 0);
    }
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
    if ((xTaskGetTickCount() - startTime) >= pdMS_TO_TICKS(2250)) {
      break;
    }
  }
  temp_average = temp_average / i;
  return temp_average;
}

void temperature_sensing(){
    uint64_t time = 0;
    
    // lcd_clear();
	while (1){
        // xLastWakeTime = xTaskGetTickCount();
        if(is_server_provisioned() && check){
            check = 0;
            // esp_bluedroid_enable();
            time = esp_timer_get_time();
            ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer,4000000 - time)); 
            _server_model_state.temperature = get_average_temp();

            warningLed(_server_model_state);
            
            ESP_LOGD(TAG, "Sending ....., time since boot: %lld us", esp_timer_get_time());
            // time = esp_timer_get_time();
            server_send_to_client(_server_model_state);
            vTaskDelay(6000/portTICK_PERIOD_MS);
            // ESP_LOGI(TAG, "Sent!!, count: %d ", count);
            // vTaskDelay(1000/portTICK_PERIOD_MS);
            // LCD_display(_server_model_state.temperature);
            // ESP_LOGI(TAG, "LCD display success! Average Temp: %.1f ", _server_model_state.temperature);
            
            
            // vTaskSuspend(MainTask);
            count = count + 1;
        }
        else{
            vTaskDelay(100/portTICK_PERIOD_MS);
        }
	}

}

static void oneshot_timer_callback(void *arg){
	int64_t time_since_boot = esp_timer_get_time();
    check = 1;
    
	ESP_LOGI(TAG, "One-shot timer called, time since boot: %lld us", time_since_boot);
    esp_timer_stop(oneshot_timer);
    saveDataToFlash(_server_model_state.high_bsline, "high_lim");
    saveDataToFlash(_server_model_state.low_bsline, "low_lim");
	esp_deep_sleep_start();
    vTaskDelay(1000/portTICK_PERIOD_MS);
    // vTaskResume(MainTask);
}


// Hàm để lưu dữ liệu vào flash
void saveDataToFlash(float data, const char *key) {
    // Mở kết nối NVS
    nvs_handle_t my_handle;
    esp_err_t ret;
    ret = nvs_open("nvs", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) return;

    // Lưu dữ liệu vào flash
    ret = nvs_set_blob(my_handle, key, (void*)&data, sizeof(float));
    if (ret == ESP_OK) {
        // printf("Dữ liệu đã được lưu vào flash\n");
    } else {
        // printf("Lỗi khi lưu dữ liệu vào flash\n");
    }
    nvs_close(my_handle);
}

// Hàm để đọc dữ liệu từ flash
float readDataFromFlash(const char *key) {
    // Mở kết nối NVS
    nvs_handle_t my_handle;
    float data ;
    size_t required_size;
    esp_err_t ret = nvs_open("nvs", NVS_READWRITE, &my_handle);
    // if (ret != ESP_OK) return;

    // Đọc dữ liệu từ flash
    
    
    ret = nvs_get_blob(my_handle, key, NULL, &required_size);
    ret = nvs_get_blob(my_handle, key, (void*)&data, &required_size);
    // Đóng kết nối NVS
    nvs_close(my_handle);
    return data;
}

void app_main(void) {
    esp_err_t err;
    esp_err_t check;
    nvs_handle_t nvs_limit;
    float limit_check = 0;
    ESP_LOGI(TAG, "Initializing...");

    // ble_mesh_received_data_queue = xQueueCreate(5, sizeof(model_sensor_data_t));

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(i2c_master_init());
    
	
    // lcd_init();
    
    ESP_LOGD(TAG, "I2C initialized successfully");
	ds18b20_init(5);
    ESP_LOGI(TAG, "1");
    // getTempAddresses(tempSensors);
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
		.name = "deep sleep",
    
	};


	ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));

	ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(16000000));

    _server_model_state.high_bsline = readDataFromFlash("high_lim");
    _server_model_state.low_bsline  = readDataFromFlash("low_lim");

	// server_send_status();
    xTaskCreatePinnedToCore(&temperature_sensing, "main task", 1024 * 2, (void *)0, 1, &MainTask, (int)1);
    // xTaskCreate(read_received_items, "setting message", 1024 * 2, (void *)0, 1, NULL);

}