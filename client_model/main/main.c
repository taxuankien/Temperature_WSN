#include <sdkconfig.h>
#include "nvs_flash.h"

#include "esp_log.h"
#include "board.h"

#include "custom_sensor_model_defs.h"
#include "mesh_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "MAIN"

static uint8_t ticks = 0;
model_sensor_data_t device_sensor_data;

static void temp_sensor_task(void *arg) {
    ESP_LOGI(TAG, "Temperature sensor main task initializing...");


    // Read initial baselines 
    float low_baseline = 18, high_baseline = 36;

    
    ESP_LOGI(TAG, "LOW: %f,  HIGH: %f",  low_baseline, high_baseline);


    ESP_LOGI(TAG, "Temperature sensor main task is running...");
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // if (xSemaphoreTake(xSemaphore, portMAX_DELAY ) == pdTRUE ) {
            // sgp30_IAQ_measure(&main_sensor);
            // xSemaphoreGive(xSemaphore);
        // }
        
        if ((ticks++ >= 3) && (is_client_provisioned())) {
        // if (ticks++ >= 5) {
            ESP_LOGI(TAG, "LOW: %.1f,  HIGH: %.1f",  low_baseline, high_baseline);
            device_sensor_data.low_bsline = low_baseline;
            device_sensor_data.high_bsline = high_baseline;
            // strcpy(device_sensor_data.device_name, "TEST");

            // ble_mesh_custom_sensor_client_model_message_set(device_sensor_data);
            // ble_mesh_custom_sensor_client_model_message_get();

            ticks = 0;
        }
    }
}

void app_main(void) {
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing...");

    // esp_log_level_set("*", ESP_LOG_VERBOSE);

    // board_init();

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // err = bluetooth_init();
    // if (err) {
    //     ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
    //     return;
    // }

    // ble_mesh_get_dev_uuid(dev_uuid);

    /* Initialize the Bluetooth Mesh Subsystem */
    // err = ble_mesh_device_init_client();
    err = ble_mesh_device_init_client();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }

    // xTaskCreate(temp_sensor_task, "temp_sensor_main_task", 1024 * 2, (void *)0, 15, NULL);

}