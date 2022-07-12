#include <string.h>
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_log.h"

#ifndef AIRTAGBT_H_INCLUDED
#define AIRTAGBT_H_INCLUDED


static const char* LOG_TAG = "XlieeTag";

// static esp_ble_adv_params_t ble_adv_params;




/** Callback function for BT events */
void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);


void set_addr_from_key(esp_bd_addr_t addr, uint8_t *public_key);

void set_payload_from_key(uint8_t *payload, uint8_t *public_key);

#endif // AIRTAGBT_H_INCLUDED