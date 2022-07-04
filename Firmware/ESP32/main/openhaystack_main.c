#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_partition.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define DELAY_IN_S 4

// Overwrite this with the generate_keypairs.py output
static uint8_t public_keys[][28] = {
    {0x15, 0xfc, 0x71, 0x89, 0x3d, 0x8c, 0x37, 0x4c, 0x45, 0x1b, 0xad, 0xcb, 0xc9, 0x13, 0xc8, 0x91, 0xde, 0x95, 0x23, 0xe1, 0x60, 0x7, 0xe0, 0x18, 0x33, 0x8, 0xe0, 0xa6},
    {0xd5, 0xd9, 0xe9, 0xb3, 0x87, 0xab, 0x79, 0x7b, 0x3f, 0xb8, 0xf8, 0x6d, 0x10, 0x9e, 0x47, 0x65, 0x98, 0x8e, 0x36, 0x81, 0x3b, 0x36, 0xde, 0x3a, 0x60, 0xd1, 0xe1, 0x1d}
};

static const char* LOG_TAG = "open_haystack";

/** Callback function for BT events */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

/** Random device address */
static esp_bd_addr_t rnd_addr = { 0xFF, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };

/** Advertisement payload */
static uint8_t adv_data[31] = {
	0x1e, /* Length (30) */
	0xff, /* Manufacturer Specific Data (type 0xff) */
	0x4c, 0x00, /* Company ID (Apple) */
	0x12, 0x19, /* Offline Finding type and length */
	0x00, /* State */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, /* First two bits */
	0x00, /* Hint (0x00) */
};


/* https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_gap_ble.html#_CPPv420esp_ble_adv_params_t */
static esp_ble_adv_params_t ble_adv_params = {
    // Advertising min interval:
    // Minimum advertising interval for undirected and low duty cycle
    // directed advertising. Range: 0x0020 to 0x4000 Default: N = 0x0800
    // (1.28 second) Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec
    .adv_int_min        = 0x3000, 
    // Advertising max interval:
    // Maximum advertising interval for undirected and low duty cycle
    // directed advertising. Range: 0x0020 to 0x4000 Default: N = 0x0800
    // (1.28 second) Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec
    .adv_int_max        = 0x4000,
    // Advertisement type
    .adv_type           = ADV_TYPE_NONCONN_IND,
    // Use the random address
    .own_addr_type      = BLE_ADDR_TYPE_RANDOM,
    // All channels
    .channel_map        = ADV_CHNL_ALL,
    // Allow both scan and connection requests from anyone. 
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    esp_err_t err;

    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&ble_adv_params);
            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            //adv start complete event to indicate adv start successfully or failed
            if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(LOG_TAG, "advertising start failed: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(LOG_TAG, "advertising has started.");
            }
            break;

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if ((err = param->adv_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(LOG_TAG, "adv stop failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(LOG_TAG, "stop adv successfully");
            }
            break;
        default:
            break;
    }
}

void set_addr_from_key(esp_bd_addr_t addr, uint8_t *public_key) {
	addr[0] = public_key[0] | 0b11000000;
	addr[1] = public_key[1];
	addr[2] = public_key[2];
	addr[3] = public_key[3];
	addr[4] = public_key[4];
	addr[5] = public_key[5];
}

void set_payload_from_key(uint8_t *payload, uint8_t *public_key) {
    /* copy last 22 bytes */
	memcpy(&payload[7], &public_key[6], 22);
	/* append two bits of public key */
	payload[29] = public_key[0] >> 6;
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);

    esp_bluedroid_init();
    esp_bluedroid_enable();

    ESP_LOGI(LOG_TAG, "application initialized");

    uint8_t* public_key;
    uint key_index = 0;
    while(true) {
        esp_err_t status;

	public_key = public_keys[key_index];
        set_addr_from_key(rnd_addr, public_key);
        set_payload_from_key(adv_data, public_key);

        ESP_LOGI(LOG_TAG, "using device address: %02x %02x %02x %02x %02x %02x", rnd_addr[0], rnd_addr[1], rnd_addr[2], rnd_addr[3], rnd_addr[4], rnd_addr[5]);
        //register the scan callback function to the gap module
        if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
            ESP_LOGE(LOG_TAG, "gap register error: %s", esp_err_to_name(status));
            return;
        }
    
        if ((status = esp_ble_gap_set_rand_addr(rnd_addr)) != ESP_OK) {
            ESP_LOGE(LOG_TAG, "couldn't set random address: %s", esp_err_to_name(status));
            return;
        }
        if ((esp_ble_gap_config_adv_data_raw((uint8_t*)&adv_data, sizeof(adv_data))) != ESP_OK) {
            ESP_LOGE(LOG_TAG, "couldn't configure BLE adv: %s", esp_err_to_name(status));
            return;
        }
	ESP_LOGI(LOG_TAG, "Sending beacon (with key index %d)", key_index);
        vTaskDelay(10);
	esp_ble_gap_stop_advertising(); // Stop immediately after first beacon
	vTaskDelay(DELAY_IN_S * 100); // pause after the public key has been broadcasted once
	ESP_LOGI(LOG_TAG, "Woke up from sleep");
	key_index = (key_index + 1) % (sizeof(public_keys)/sizeof(public_keys[0]));
    }   
}
