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
#define ACCESS_TOKEN "6n009mfxxuq4sc4fisu7"

#define MAX_KEY_LENGTH 100
#define MAX_VALUE_LENGTH 100
int check, node, lastValue, k = 3;

QueueHandle_t queue;
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


// model_sensor_data_t data1 = {
//     .device_name = "v1/devices/me/telemetry",
//     .temperature = 20
// };

// model_sensor_data_t data2 = {
//     .device_name = "v1/devices/me/telemetry",
//     .temperature = 30
// };

// model_sensor_data_t data3 = {
//     .device_name = "v1/devices/me/telemetry",
//     .temperature = 40
// };

// model_sensor_data_t data4 = {
//     .low_bsline = 0,
//     .high_bsline = 100
// };

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
        //float val = atof(jsonObjects.value);
        double val;
        sscanf((char*)jsonObjects.value, "%lf", &val);
        int count = 0;
        // for (int i = 0; i < k; i++)
        // {
        //     if(!strcmp(jsonObjects.key, "LowLimit")) data[i].low_bsline = val;
        //     else if(!strcmp(jsonObjects.key, "AmountOfNode")){
        //         count = (int) val;
        //         check = 1;
        //     } 
        //     else data[i].high_bsline = val;
        //     //ESP_LOGI(TAG,"LowLimit = %lf\n", data[i].low_bsline);
        //     //ESP_LOGI(TAG,"HighLimit = %lf\n", data[i].high_bsline);
        // }
        if(check == 1){
            lastValue = k;
            k = count;
            node = count;
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

void app_main(void){
    esp_err_t err;
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

    err = ble_mesh_device_init_client();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }
    // vTaskDelay(10000/portTICK_PERIOD_MS);
    // while(!is_client_provisioned()){
    //     vTaskDelay(1000/portTICK_PERIOD_MS);
    // }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect()); // Connect WiFi
    // vTaskDelay(1000/portTICK_PERIOD_MS);
    
    
    
    mqtt_app_start();

   
    
    // for (int i = 0; i < k; i++){ 
    //     char esp[30];
    //     sprintf(esp, "esp%d", i);

    //     strcpy(data[i].device_name , esp);
    //     data[i].low_bsline = 0;
    //     data[i].high_bsline = 100;
    //     data[i].temperature = 10*(i+1);
    // }
    model_sensor_data_t rxBuffer;
    while (1){ 
         ESP_LOGI(" "," ");
          if( xQueueReceive(queue, &(rxBuffer), (TickType_t)5))
            {   
                ESP_LOGI(TAG, "Startup..");
                int number = atoi(rxBuffer.device_name);
                char payload[30] ;
                snprintf(payload, sizeof(payload), "{temperature%d:%f}", number,rxBuffer.temperature);
                esp_mqtt_client_publish(client1, "v1/devices/me/telemetry"  , payload, 0, 1, 0);
            }
        // for (int i = 0; i < k; i++)
        // {
        //     if(check == 1 && lastValue < node){
        //          for (int x = lastValue; x < node; x++){ 
        //             char esp[30];
        //             sprintf(esp, "esp%d", x);
        //             strcpy(data[x].device_name , esp);
        //             data[x].low_bsline = 0;
        //             data[x].high_bsline = 100;
        //             data[x].temperature = 10*(x+1);
        //         }
        //         check = 0;
        //     }
        //     char key[30];
        //     if((data[i].device_name))
        //     sprintf(key, "{temperature%d:%f}", i+1, data[i].temperature);
            // esp_mqtt_client_publish(client1, "v1/devices/me/telemetry"  , key, 0, 1, 0);
        // }
        
        // char payload1[30] ;
        // snprintf(payload1, sizeof(payload1), "{temperature1:%f}", data1.temperature);
        // esp_mqtt_client_publish(client1, "v1/devices/me/telemetry"  , payload1, 0, 1, 0);

        // char payload2[30];
        // snprintf(payload2, sizeof(payload2), "{temperature2:%f}", data2.temperature);
        // esp_mqtt_client_publish(client1, "v1/devices/me/telemetry" , payload2, 0, 1, 0);

        // char payload3[30];
        // snprintf(payload3, sizeof(payload3), "{temperature3:%f}", data3.temperature);
        // esp_mqtt_client_publish(client1, "v1/devices/me/telemetry" , payload3, 0, 1, 0);

        vTaskDelay(500/portTICK_PERIOD_MS);
    }
}


