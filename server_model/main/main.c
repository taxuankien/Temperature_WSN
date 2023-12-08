#include <sdkconfig.h>
#include "nvs_flash.h"

#include "esp_log.h"

#include "mesh_server.h"
#include "custom_sensor_model_defs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "MESH SERVER";

QueueHandle_t ble_mesh_received_data_queue = NULL;
model_sensor_data_t _server_model_state = {
    .device_name = "esp_3",
};

static void read_received_items(void *arg) {
    ESP_LOGI(TAG, "Task initializing..");

    model_sensor_data_t _received_data;
    

    while (1) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        
        if (xQueueReceive(ble_mesh_received_data_queue, &_received_data, 1000 / portTICK_PERIOD_MS) == pdPASS) {
            ESP_LOGI("PARSED_DATA", "Device Name = %s", _received_data.device_name);
            ESP_LOGI("PARSED_DATA", "Temperature = %f", _received_data.temperature);
            ESP_LOGI("PARSED DATA", "Warning low = %f", _received_data.low_bsline);
            ESP_LOGI("PARSED DATA", "Warning high = %f", _received_data.high_bsline);
        }
        _server_model_state.high_bsline = _received_data.high_bsline;
        _server_model_state.low_bsline = _received_data.high_bsline;
        
    }   
} 

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

    ble_mesh_received_data_queue = xQueueCreate(5, sizeof(model_sensor_data_t));

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // err = ble_mesh_device_init_server();
    err = ble_mesh_device_init_server();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err 0x%06x)", err);
    }
    
    xTaskCreate(server_send_status, "status sending", 1024 * 2, (void *)0, 20, NULL);
    // xTaskCreate(server_send_to_client, "server send", 1024 * 2, (void *)0, 20, NULL);

}