#ifndef __CUSTOM_SENSOR_MODEL_DEFS_H__
#define __CUSTOM_SENSOR_MODEL_DEFS_H__

#include <stdio.h>

#include "sdkconfig.h"

#include "esp_ble_mesh_common_api.h"

#define BLE_MESH_DEVICE_NAME    "BLE Mesh" /*!< Device Advertising Name */ 
#define CID_ESP                 0x02E5                  /*!< Espressif Component ID */

//* Definicao dos IDs dos Models (Server e Client)
#define ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_ID_SERVER      0x1414  /*!< Custom Server Model ID */
#define ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_ID_CLIENT      0x1415  /*!< Custom Client Model ID */

//* Definimos os OPCODES das mensagens (igual no server)
#define ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_OP_GET         ESP_BLE_MESH_MODEL_OP_3(0x00, CID_ESP)
#define ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_OP_SET         ESP_BLE_MESH_MODEL_OP_3(0x01, CID_ESP)
#define ESP_BLE_MESH_CUSTOM_SENSOR_MODEL_OP_STATUS      ESP_BLE_MESH_MODEL_OP_3(0x02, CID_ESP)

#define ESP_BLE_MESH_GROUP_PUB_ADDR                     0xC000
#define ESP_BLE_MESH_GROUP_GW_SUB_ADDR                  0xC001
#define ESP_BLE_MESH_ADDR_ALL_NODES                     0xFFFF          

/** 
 * @brief Device Main Data Structure
 */
typedef struct __attribute__((packed)) {
    char device_name[4];
    
    /**< Temperature sensor Data */
    float temperature;  
    float low_bsline;
    float high_bsline;

    /**< Feedback answers */
    // uint8_t feedback;   /*!< Each bit corresponds to an answer: (NEW_DATA | X | X | TEMP_COMF | HIGH_TEMP | SOUND_COMF | LIGHT_COMF | LIGHTNESS) */
} model_sensor_data_t;


#endif  // __CUSTOM_SENSOR_MODEL_DEFS_H__