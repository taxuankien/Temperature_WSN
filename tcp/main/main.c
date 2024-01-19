#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "esp_ble_mesh_common_api.h"
#include "custom_sensor_model_defs.h"
#include "esp_ble_mesh_defs.h"
#include "mesh_client.h"
// Access Token của device Temp trên Thingsboard
#define ACCESS_TOKEN "VArxbVvDhxEDhpWre49N"

#define MAX_KEY_LENGTH 100
#define MAX_VALUE_LENGTH 100
#define MAX_DEVICE_NUM  20
#define MQTT_FLAG   (1<<3)
#define TIME_FLAG   (1<<2)


int check, node, lastValue, k = 3;

QueueHandle_t queue;
EventGroupHandle_t xEventBits;
data_state_t received_data_state[MAX_DEVICE_NUM];
int current_device_num = 0;

model_sensor_data_t device_sensor_data = {
    .high_bsline = 38,
    .low_bsline = 16,
};
// Label:
// Định nghĩa cấu trúc cho đối tượng JSON
typedef struct {
    char *key;
    char *value;
} JsonObject;

// Hàm kiểm tra xem một ký tự có phải là ký tự trắng hay không
int isWhitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

// Hàm phân tích chuỗi JSON
void parseJson(const char *jsonString, JsonObject *jsonObjects) {
    int i = 0;

    // Bỏ qua ký tự trắng
    while (isWhitespace(jsonString[i])) {
        i++;
    }

    // Bắt đầu đọc key
    if (jsonString[i] == '{') {
        i++;                                                            // Bỏ qua dấu ngoặc nhọn mở
        if(jsonString[i] == '\"') i++;                                  // Bỏ qua nếu là dấu nháy kép
        // Đọc key
        int keyIndex = 0;
        jsonObjects->key = malloc(MAX_KEY_LENGTH);
        while (jsonString[i] != ':') {
            if (!isWhitespace(jsonString[i]) && jsonString[i] != '\"') {
                jsonObjects->key[keyIndex++] = jsonString[i];
            }
            i++;
        }
        jsonObjects->key[keyIndex] = '\0'; 
        // Kết thúc key

        i++;                                                            // Bỏ qua dấu hai chấm

                                                                        
        while (isWhitespace(jsonString[i]) || jsonString[i] == '\"') {
            i++;                                                        // Bỏ qua ký tự trắng và dấu nháy mở
        }

        // Bắt đầu đọc value
        int valueIndex = 0;
        jsonObjects->value = malloc(MAX_VALUE_LENGTH);
        while ((jsonString[i] >= '0' && jsonString[i] <= '9') || jsonString[i] == '.') {
            jsonObjects->value[valueIndex++] = jsonString[i++];
        }
        jsonObjects->value[valueIndex] = '\0'; 
        // Kết thúc value

        
        while (isWhitespace(jsonString[i]) || jsonString[i] == '\"' || jsonString[i] == '}') {
            i++;                                                        // Bỏ qua ký tự trắng và dấu nháy đóng
        }
    }
}

// Hàm giải phóng bộ nhớ của đối tượng JSON
void freeJsonObject(JsonObject *jsonObjects) {
    free(jsonObjects->key);
    free(jsonObjects->value);
}


JsonObject jsonObjects;   
esp_mqtt_client_handle_t client1;
static const char *TAG = "MQTT";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    int msg_id;
    esp_mqtt_event_handle_t event  = event_data;
    esp_mqtt_client_handle_t client = event->client;
    client1 =  event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "v1/devices/me/attributes", 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG,"TOPIC = %.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG,"DATA = %.*s\n", event->data_len, event->data);

        parseJson((char *)event->data, &jsonObjects);

        if(!strcmp(jsonObjects.key, "LowLimit")){
            sscanf((char*)jsonObjects.value, "%f", &device_sensor_data.low_bsline);
            ESP_LOGI(TAG,"LowLimit = %lf\n", device_sensor_data.low_bsline);
        }

        if(!strcmp(jsonObjects.key, "HighLimit")){
            sscanf((char*)jsonObjects.value, "%f", &device_sensor_data.high_bsline);
            ESP_LOGI(TAG,"HighLimit = %lf\n", device_sensor_data.high_bsline);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


static void mqtt_app_start(void){
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://mqtt.thingsboard.cloud:1883",
        .session.keepalive = 60,
        .credentials.username = ACCESS_TOKEN
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
      /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void state_update(model_sensor_data_t data){
    int deviceIdx = atoi(data.device_name);
    if(!received_data_state[deviceIdx].active){
        current_device_num ++;
    }
    received_data_state[deviceIdx].active = true;
    received_data_state[deviceIdx].receiv = true;
    received_data_state[deviceIdx].lim_changed = ((int)(data.high_bsline * 10) != (int)(device_sensor_data.high_bsline * 10)||(int)(data.low_bsline * 10) != (int)(device_sensor_data.low_bsline * 10))?true:false;
}



void mqtt_sending(){
    model_sensor_data_t rxBuffer;
    TickType_t time;
    char payload[30] ;
    int number = 0;
    
    while( xQueueReceive(queue, &(rxBuffer), (TickType_t)5)){   
            ESP_LOGI(TAG, "Startup..");
            number = atoi(rxBuffer.device_name);
            snprintf(payload, sizeof(payload), "{temperature%d:%f}", number,rxBuffer.temperature);
            esp_mqtt_client_publish(client1, "v1/devices/me/telemetry"  , payload, 0, 1, 0);
            state_update(rxBuffer); 
                
    }  
    xEventGroupSetBits(xEventBits, MQTT_FLAG); 
}

void main_task(void *arg){
    TickType_t time;
    while (1)
    {   if(is_client_provisioned()){
            time = xTaskGetTickCount();
            ble_mesh_custom_sensor_client_model_message_get();
            vTaskDelayUntil(&time, 1000/portTICK_PERIOD_MS);
            mqtt_sending();
            vTaskDelayUntil(&time, 19000/portTICK_PERIOD_MS);
        }
        else
            vTaskDelay(20000/portTICK_PERIOD_MS);
    }
    
}

void state_process(void *arg){
    int total_message ;
    char payload[30] ;
    TickType_t time;
    EventBits_t uxBits;
    bool limit_check = false;

    while(1){
        time = xTaskGetTickCount();
        uxBits = xEventGroupWaitBits(xEventBits, MQTT_FLAG, pdTRUE, pdTRUE, (TickType_t)100);
        if((uxBits & MQTT_FLAG) != 0){
            total_message = 0;
            for(int i = 0; i < MAX_DEVICE_NUM; i++){
                switch (received_data_state[i].active)
                {
                case true:
                    switch(received_data_state[i].receiv){
                    case true:
                        total_message++;
                        if(received_data_state[i].lim_changed){
                            limit_check = true;
                            
                        }
                        break;
                    default:
                        snprintf(payload, sizeof(payload), "{temperature%d:-1}", i);
                        esp_mqtt_client_publish(client1, "v1/devices/me/telemetry"  , payload, 0, 1, 0);
                        break;
                    }
                    break;
                default:
                    break;
                }
                
                received_data_state[i].receiv = false;
            }
            if(limit_check){
                ble_mesh_custom_sensor_client_model_message_set(device_sensor_data, 0xC000);
                limit_check = false;
            }
            ESP_LOGW(TAG, "Total received messages: %d / device: %d", total_message, current_device_num);
            vTaskDelayUntil(&time, 20000/portTICK_PERIOD_MS);
        }
    }
}


void app_main(void){
    esp_err_t err;
    TickType_t tick;
    queue = xQueueCreate(5, sizeof(model_sensor_data_t)); 

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

   
    
    xEventBits = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect()); // Connect WiFi
     err = ble_mesh_device_init_client();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }
    
    mqtt_app_start();
    xTaskCreatePinnedToCore(&main_task, "main task", 3072, NULL, 2, NULL, (int)1);
    xTaskCreatePinnedToCore(&state_process, "state processing", 2048, (void*)0, 0, NULL, (int)1);
}


