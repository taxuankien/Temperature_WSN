#include "mesh_server.h"

#include <sdkconfig.h>

#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_ble_mesh_defs.h"

#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "MESH_SERVER";

static uint8_t dev_uuid[16] = { 0xdd, 0xdd };   /**< Device UUID */
int64_t time = 0;

static esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_ENABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
    .default_ttl = 7,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(0, 10),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(0, 10),
};

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
};


static esp_ble_mesh_model_op_t custom_sensor_op[] = {
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_OP_GET, 0),  // OP_GET  0 bytes
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_OP_SET, 4),  // OP_SET  4 bytes
    ESP_BLE_MESH_MODEL_OP_END,
};


static esp_ble_mesh_model_t custom_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_ID_SERVER, 
    custom_sensor_op, NULL, &_server_model_state),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, custom_models),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP, 
    .elements = elements,
    .element_count = ARRAY_SIZE(elements)
};

static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
    .output_size = 0,
    .output_actions = 0
};

static bool is_server_provisioning = false;
static bool check = false;

/**
 * @brief Provisioning routine callback function
 * 
 * @param  event  Provision event
 * @param  param   Pointer to Provision parameter
 */
static void ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                        esp_ble_mesh_prov_cb_param_t *param);


/**
 * @brief Configuration Server Model callback function
 * 
 * @param  event  Config Server Model event
 * @param  param   Pointer to Config Server Model parameter
 */
static void ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event, 
                                        esp_ble_mesh_cfg_server_cb_param_t *param);

/**
 * @brief Custom Sensor Client Model callback function
 * 
 * @param  event  Sensor Client Model event
 * @param  param   Pointer to Sensor Server Model parameter
 */
static void ble_mesh_custom_sensor_server_model_cb(esp_ble_mesh_model_cb_event_t event,
                                                esp_ble_mesh_model_cb_param_t *param);


/**
 * @brief Parses received Sensor Model raw data and stores it on appropriate structure
 * 
 * @param  recv_param   Pointer to model callback received parameter
 * @param  parsed_data  Pointer to where the parsed data will be stored
 */
static void parse_received_data(esp_ble_mesh_model_cb_param_t *recv_param, model_sensor_data_t *parsed_data);

bool is_server_provisioned(void)
{
    return is_server_provisioning && check;
}

void server_send_to_client(model_sensor_data_t server_model_state)
{
    esp_ble_mesh_msg_ctx_t _ctx = {0};
    _ctx.send_ttl = 5;
    _ctx.addr = 0xc001;   
    // _ctx.addr = 0x000d;          //gateway
    // _ctx.recv_dst = 0x0058;         // me
    _ctx.srv_send = 0;

    esp_err_t err = esp_ble_mesh_server_model_send_msg(custom_models, &_ctx, ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_OP_STATUS, sizeof(server_model_state), &server_model_state);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Send STATUS failed err 0x%04x", err);
    }
}


/*******************************************
 ****** Private Functions Definitions ******
 *******************************************/

static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index) {
    ESP_LOGI(TAG, "net_idx: 0x%04x, addr: 0x%04x", net_idx, addr);
    ESP_LOGI(TAG, "flags: 0x%02x, iv_index: 0x%08lx", flags, iv_index);
    
    //! Precisa salvar o netkey idx????
    // node_net_idx = net_idx;  // Netkey index saved
    is_server_provisioning  = true;

    ESP_LOGI(TAG, "Device provisioned");

}

static void ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                                esp_ble_mesh_prov_cb_param_t *param) {
    switch (event) {
        case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
            ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
        break;
        
        case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
            
            ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code %d", param->node_prov_enable_comp.err_code);
            check = (param->node_prov_enable_comp.err_code == -120)?true:false;
        break;
        
        case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
            ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer %s",
                param->node_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
        
        case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
            ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer %s",
                param->node_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
        
        case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT");
            prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr,
                param->node_prov_complete.flags, param->node_prov_complete.iv_index);
        break;
        
        case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
            ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_RESET_EVT");
        break;
        
        case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
            ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, Name: %s, err_code %d", BLE_MESH_DEVICE_NAME, param->node_set_unprov_dev_name_comp.err_code);
        break;
        
        default:
        break;
    }
}

static void ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                                              esp_ble_mesh_cfg_server_cb_param_t *param) {
    if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT) {
        switch (param->ctx.recv_op) {
            //* Application key added to device (on provisioning routine)
            case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
                ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
                ESP_LOGI(TAG, "net_idx 0x%04x, app_idx 0x%04x",
                    param->value.state_change.appkey_add.net_idx,
                    param->value.state_change.appkey_add.app_idx);
                ESP_LOG_BUFFER_HEX("AppKey", param->value.state_change.appkey_add.app_key, 16);
            break;

            //* Application key bound to Model
            case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
                check = true;
                ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
                ESP_LOGI(TAG, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%06x",
                    param->value.state_change.mod_app_bind.element_addr,
                    param->value.state_change.mod_app_bind.app_idx,
                    param->value.state_change.mod_app_bind.company_id,
                    param->value.state_change.mod_app_bind.model_id);
                ESP_LOGI(TAG, "Application key bound to model 0x%06x", param->value.state_change.mod_app_bind.model_id);
            break;

            //* Model Subscription group add
            case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD:
                ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD");
                ESP_LOGI(TAG, "elem_addr 0x%04x, sub_addr 0x%04x, cid 0x%04x, mod_id 0x%04x",
                    param->value.state_change.mod_sub_add.element_addr,
                    param->value.state_change.mod_sub_add.sub_addr,
                    param->value.state_change.mod_sub_add.company_id,
                    param->value.state_change.mod_sub_add.model_id);
                ESP_LOGI(TAG, "Model 0x%06x subscribed to Group 0x%04x !",
                    param->value.state_change.mod_sub_add.model_id,
                    param->value.state_change.mod_sub_add.sub_addr);
            break;


            /** @TODO: Adicionar publication address tbm */

            default:
            break;
        }
    }
}

static void ble_mesh_custom_sensor_server_model_cb(esp_ble_mesh_model_cb_event_t event,
                                                    esp_ble_mesh_model_cb_param_t *param) {

    switch (event) {
        //* Recebimento de msg direta pra esse node
        case ESP_BLE_MESH_MODEL_OPERATION_EVT:
            switch (param->model_operation.opcode) {

                case ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_OP_GET:
                    if (param->model_operation.length > 0) {
                        ESP_LOGI(TAG, "OP_GET -- Opcode 0x%06lx,  tid 0x%04x", param->model_operation.opcode, *param->model_operation.msg);
                    } else {
                        ESP_LOGW(TAG, "OP_GET -- Opcode 0x%06lx  -- empty message", param->model_operation.opcode);
                    }

                    //* Responde com o estado atual do Model (OP_STATUS)
                    // model_sensor_data_t response = *(model_sensor_data_t *)param->model_operation.model->user_data;

                    // esp_err_t err = esp_ble_mesh_server_model_send_msg(param->model_operation.model, param->model_operation.ctx, 
                    //                 ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_OP_STATUS, sizeof(response), (uint8_t *)&response);
                    // if (err) {
                    //     ESP_LOGE(TAG, "%s -- Failed to send response with OPCODE 0x%06x", __func__, ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_OP_STATUS);
                    // }
                    // _server_model_state.temperature ++;
                    // server_send_to_client(_server_model_state);
                break;

                case ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_OP_SET:
                    ESP_LOGI("TIME", "%lld", esp_timer_get_time() - time);
                    ESP_LOGI(TAG, "OP_SET -- Received HEX message: ");
                    ESP_LOG_BUFFER_HEX(TAG, (uint8_t*)param->model_operation.msg, param->model_operation.length);
                    
                    //* Salva os dados recebidos no State do Model
                    parse_received_data(param, (model_sensor_data_t*)&param->model_operation.model->user_data);
                break;

                default:
                    ESP_LOGW(TAG, "Unknown OPCODE message received: 0x%06lx", param->model_operation.opcode);
                break;
            }
        break;

        //* Evento chamado apos tentar enviar uma msg pra outro lugar
        case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
            if (param->model_send_comp.err_code) {
                ESP_LOGE(TAG, "Failed to send message 0x%06lx", param->model_send_comp.opcode);
            } else {
                
                ESP_LOGI(TAG, "%s -- SEND_COMPLETE -- Send message opcode 0x%08lx success", __func__, param->model_send_comp.opcode);
                time = esp_timer_get_time();
            }
        break;

        default:
            ESP_LOGW(TAG, "%s - Unknown Custom Sensor Server event: 0x%04x", __func__, event);
        break;
    }
}

static void parse_received_data(esp_ble_mesh_model_cb_param_t *recv_param, model_sensor_data_t *parsed_data) {
    if (recv_param->client_recv_publish_msg.length < sizeof(parsed_data)) {
        ESP_LOGE(TAG, "Invalid received message lenght: %d", recv_param->client_recv_publish_msg.length);
        return;
    }

    parsed_data = (model_sensor_data_t *)recv_param->client_recv_publish_msg.msg;

    ESP_LOGW("PARSED_DATA", "Device Name = %s", parsed_data->device_name);
    ESP_LOGW("PARSED_DATA", "Temperature = %f", parsed_data->temperature);
    ESP_LOGW("PARSED DATA", "Warning low temperature = %f", parsed_data->low_bsline);
    ESP_LOGW("PARSED DATA", "Warning high temperature = %f", parsed_data->high_bsline);
    
    memcpy((void *)& _server_model_state.high_bsline, (void * )&parsed_data->high_bsline , sizeof(parsed_data->high_bsline));
    memcpy((void *)& _server_model_state.low_bsline, (void * )&parsed_data->low_bsline, sizeof(parsed_data->low_bsline) );
    // xQueueSendToBack(ble_mesh_received_data_queue, parsed_data, portMAX_DELAY);
}


static void ble_mesh_get_dev_uuid(uint8_t *dev_uuid) {
    if (dev_uuid == NULL) {
        ESP_LOGE(TAG, "%s, Invalid device uuid", __func__);
        return;
    }

    /* Copy device address to the device uuid with offset equals to 2 here.
     * The first two bytes is used for matching device uuid by Provisioner.
     * And using device address here is to avoid using the same device uuid
     * by different unprovisioned devices.
     */
    memcpy(dev_uuid + 2, esp_bt_dev_get_address(), BD_ADDR_LEN);
}

static esp_err_t bluetooth_init(void) {
    esp_err_t ret;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s initialize controller failed", __func__);
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed", __func__);
        return ret;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed", __func__);
        return ret;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed", __func__);
        return ret;
    }

    return ret;
}


/*******************************************
 ****** Public Functions Definitions ******
 *******************************************/

esp_err_t ble_mesh_device_init_server(void) {
    esp_err_t err = ESP_OK;

    //* Initializes esp32 bluetooth stack
    err = bluetooth_init();
    if (err) {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return err;
    }

    //* Get full device UUID
    ble_mesh_get_dev_uuid(dev_uuid);

    //* Register Model's callback functions
    esp_ble_mesh_register_prov_callback(ble_mesh_provisioning_cb);
    esp_ble_mesh_register_config_server_callback(ble_mesh_config_server_cb);
    esp_ble_mesh_register_custom_model_callback(ble_mesh_custom_sensor_server_model_cb);

    //* Initializes BLE Mesh stack 
    err = esp_ble_mesh_init(&provision, &composition);
    if (err) {
        ESP_LOGE(TAG, "Initializing mesh failed (err %d)", err);
        return err;
    }

    //* Set device name 
    esp_ble_mesh_set_unprovisioned_device_name(_server_model_state.device_name);

    //* Enable provisioning
    esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);

    ESP_LOGI(TAG, "BLE Mesh Node initialized!");

    return err;
}




