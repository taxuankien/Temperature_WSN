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
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "ds18b20.h"
#include "LCD1602.h"

#define TAG     "MESH SERVER"
#define DS_PIN   15 //GPIO where you connected ds18b20
#define BOARD_FLAG  (1 << 3)
#define TEMP_FLAG   (1 << 1)
#define RX_FLAG     (1 << 2)
#define TX_FLAG     (1)

DeviceAddress tempSensors[1];
TaskHandle_t MainTask = NULL;
EventGroupHandle_t xEventBits;

gpio_num_t ledWaring[3] = {25, 26, 27};
uint16_t count=0;
uint8_t check =1;
uint64_t time_to_restart;
SemaphoreHandle_t xSemaphore;

// QueueHandle_t ble_mesh_received_data_queue = NULL;
model_sensor_data_t _server_model_state = {
    .device_name = "2",
};

float readDataFromFlash(const char *key);
void saveDataToFlash(float data, const char *key);

void LCD_display(float a){
	
	char str[16] = " ";
	lcd_clear();
	lcd_put_cur(0, 0);
    sprintf(str, "Node %s       ", _server_model_state.device_name);
	lcd_send_string(str);
	
	lcd_put_cur(1, 0);
	sprintf(str, "Temp:%.1f     ", a);
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

void initWarningLed(){
    for(int i = 0; i < 3; i++){
        rtc_gpio_init(ledWaring[i]);
        rtc_gpio_set_direction(ledWaring[i], GPIO_MODE_OUTPUT);
    }
}

void disableWarningLed(){
    for(int i = 0; i < 3; i++){
        rtc_gpio_hold_dis(ledWaring[i]);
    }
}

void warningLed(model_sensor_data_t data){
    disableWarningLed();
    if (data.temperature < data.low_bsline){
        setLedWarningLevel(1, 0, 0);
    }
    else if(data.temperature > data.high_bsline){
        setLedWarningLevel(0, 1, 0);
    }
    else if (data.temperature <= data.high_bsline && data.temperature >= data.low_bsline){
        setLedWarningLevel(0, 0, 1);
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

float get_temp(void){
    float temp = 0;
    int i = 0;
    EventBits_t uxBits;

    xSemaphoreTake( xSemaphore, ( TickType_t ) 10 );
    temp = ds18b20_get_temp();
    ESP_LOGW(TAG, "Temperature: %f\n",temp);
    uxBits = xEventGroupSetBits(xEventBits, TEMP_FLAG);
    xSemaphoreGive( xSemaphore );
    return temp;
}

void board_operaton(){
    EventBits_t uxBits;
    LCD_display(_server_model_state.temperature);
    warningLed(_server_model_state);
    saveDataToFlash(_server_model_state.high_bsline, "high_lim");
    saveDataToFlash(_server_model_state.low_bsline, "low_lim");
    
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


void main_task(void *args){
    EventBits_t uxBits;
    TickType_t tick;

    
    while(1){
        tick = xTaskGetTickCount();
        uxBits = xEventGroupWaitBits(xEventBits, RX_FLAG, pdTRUE, pdTRUE, 50/portTICK_PERIOD_MS);
        if((uxBits & RX_FLAG) != 0){

            _server_model_state.temperature = get_temp();
            server_send_to_client(_server_model_state);
            
            vTaskDelayUntil(&tick, 1000 /portTICK_PERIOD_MS);
            board_operaton();
            vTaskDelayUntil(&tick, 1000 /portTICK_PERIOD_MS);
            ESP_LOGI(TAG, "sleep");
            esp_deep_sleep_start();
            
        }
        
    }
}

void app_main(void) {
    esp_err_t err;
    esp_err_t check;
    nvs_handle_t nvs_limit;
    uint64_t time;
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
    
	
    lcd_init();
    
    initWarningLed();
    ESP_LOGD(TAG, "I2C initialized successfully");
	ds18b20_init(DS_PIN);
    ESP_LOGI(TAG, "1");
    getTempAddresses(tempSensors);
    ESP_LOGI(TAG, "2");
    ds18b20_setResolution(tempSensors,1,12);
	ESP_LOGI(TAG, "Sensor initialized successfully");
    
    err = ble_mesh_device_init_server();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err 0x%06x)", err);
    }
    
    _server_model_state.high_bsline = readDataFromFlash("high_lim");
    _server_model_state.low_bsline  = readDataFromFlash("low_lim");

    xEventBits = xEventGroupCreate();
    xSemaphore = xSemaphoreCreateBinary();
    
    esp_sleep_enable_timer_wakeup(16000000);
    
    xTaskCreatePinnedToCore(&main_task, "main task", 1024 * 2, NULL, 1, NULL, (int)1);

}