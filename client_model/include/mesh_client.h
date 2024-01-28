/**
 * @file mesh_client.h
 * 
 * @brief
 * 
 * @author
 * 
 * @date  11/2020
 */

#ifndef __MESH_CLIENT_H__
#define __MESH_CLIENT_H__

#include <stdio.h>
#include <string.h>

#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"

#include "custom_sensor_model_defs.h"

/**
 * @brief Initializes BLE Mesh stack, initializing Models and it's callback functions
 * 
 */
esp_err_t ble_mesh_device_init_client(void);


/**
 * @brief Custom Sensor Client Model SET message that
 *        publishes data to ESP_BLE_MESH_GROUP_PUB_ADDR
 *
 */
typedef struct data_state 
{
    bool active;
    bool receiv;
    bool lim_changed;
} data_state_t;
 
extern model_sensor_data_t device_sensor_data;
extern model_sensor_data_t received_data;
extern QueueHandle_t queue;

esp_err_t ble_mesh_custom_sensor_client_model_message_set(model_sensor_data_t set_data, uint16_t addr);


/**
 * @brief Custom Sensor Client Model GET message that
 *        publishes data to ESP_BLE_MESH_GROUP_PUB_ADDR
 * 
 * @note  Received data will be available on Model Callback function
 */ 
esp_err_t ble_mesh_custom_sensor_client_model_message_get(void);

bool is_client_provisioned(void);

#endif  // __MESH_CLIENT_H__