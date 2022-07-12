#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>


#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// mbedtls
#include "mbedtls/ecp_internal.h"

#include "mbedtls/platform.h"
#include "mbedtls/md_internal.h"
#include "mbedtls/base64.h"

#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_partition.h"
#include "nvs_flash.h"

// bluetooth
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_log.h"

#include "driver/uart.h"

#include "esp_sleep.h"
#include "esp_event.h"
#include "esp_timer.h"
// #include "esp_gap_ble_api.h"
// #include "esp_gattc_api.h"
// #include "esp_gatt_defs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// custom
#include "utils.h"
#include "airTagBT.h"
#include "crypto.h"

#define DELAY_IN_S 5
#define RESET_INTERVAL 20



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


    size_t dummy = 0;

    unsigned char d_0[Private_Key_Size];
    unsigned char SK_0[Simetric_Key_Size];

    unsigned char d_0_b64[] = "s9J5hI/DxtLlG31A41zil+wKflRixSLKx2M8yA==";
    unsigned char sk_0_b64[] = "C4XtRo98PhhzO9V/jwwIPprCjxk3mSn+O+gRIG7unWc=";

    mbedtls_base64_decode(d_0, Private_Key_Size, &dummy, (const unsigned char *)d_0_b64, strlen((const char *)d_0_b64));
    mbedtls_base64_decode(SK_0, Simetric_Key_Size, &dummy, (const unsigned char *)sk_0_b64, strlen((const char *)sk_0_b64));

    mbedtls_printf("d_0: \n");
    print_hex(d_0, Private_Key_Size, true);

    mbedtls_printf("sk_0: \n");
    print_hex(SK_0, Simetric_Key_Size, true);




    mbedtls_printf("STARTING.... \n");

    // memset(derivedSymetricKey, '\0', Simetric_Key_Size);
    // unsigned char update_shared[] = "update";
    // mbedtls_ansi_x936_kdf(MBEDTLS_MD_SHA256, Simetric_Key_Size, SK_0, strlen((const char*) update_shared), update_shared, Simetric_Key_Size, derivedSymetricKey);
    // copy SK_0 to derivedSymetricKey



    unsigned char derivedSymetricKey[Simetric_Key_Size];
    unsigned char derivedPublicKey[Public_Key_Size];
    unsigned char derivedAdvertismentKey[Advertisement_Key_Size];
    // copy initial simetric key to derivedSymetricKey
    memcpy(derivedSymetricKey, SK_0, Simetric_Key_Size);


    int count = 0;
    while(true){
        esp_err_t status;
	    // public_key = public_keys[key_index];






        printf("\nIteration %d \n", count);
        if(count == 0 || count % RESET_INTERVAL == 0){
            printf("\nDeriving New Keys... \n");
            DeriveKeyPair(derivedPublicKey, derivedSymetricKey, d_0);
            // calculate advertisement key for bluetooth
            memcpy(derivedAdvertismentKey, &derivedPublicKey[1], Advertisement_Key_Size);
            printf("PK: 0x");
            print_hex(derivedPublicKey, Public_Key_Size, false);
        }


        set_addr_from_key(rnd_addr, derivedAdvertismentKey);
        set_payload_from_key(adv_data, derivedAdvertismentKey);
        printf("using device address: %02x:%02x:%02x:%02x:%02x:%02x\n", rnd_addr[0], rnd_addr[1], rnd_addr[2], rnd_addr[3], rnd_addr[4], rnd_addr[5]);

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
        printf( "Sending beacon (Set: %d ,index: %d)\n", count/RESET_INTERVAL, count%RESET_INTERVAL);
        vTaskDelay(10);
        esp_ble_gap_stop_advertising(); // Stop immediately after first beacon
        vTaskDelay(30);

        // Goto sleep for 1.9 seconds
        esp_sleep_enable_timer_wakeup(DELAY_IN_S * 1e6);
        printf("Entering light sleep\n");
        uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);

        // Enter sleep mode 
        esp_light_sleep_start();
        
        // Execution continues here after wakeup
        printf("Returned from light sleep\n");
        

        // vTaskDelay(DELAY_IN_S * 100);

        count++;
        if(count/RESET_INTERVAL >= 100){
            count = 0;
            memcpy(derivedSymetricKey, SK_0, Simetric_Key_Size);

        }



    }
    

    // guardar en la memoria flash el derivedPublicKey



    // Drop first byte to get advertisment key





    // leer sk_i y d_0 de flash

    // if(!sk_i && !d_0){  scanf("%s", &sk_0);scanf("%s", &d_0); }

    // save date on flash

    // while(1){     
    //     if(condicion){
    //         DeriveKeyPair(derivedPublicKey, derivedSymetricKey, (const unsigned char*)d_0);  
    //         flash sk_i
    //     }
    //     send(advertisementKey) por bt; 
       
    //     sleep(cuanto?);         
        
    // }











    

    // mbedtls_printf("DerivedAdvertismentKey: \n");
    // for(int i = 0; i < advKey_size; i++){
    //     printf("%02x ", derivedAdvertismentKey[i]);
    // }
    // printf("\n");

    // unsigned char derivedAdvertisementKeyHash[32];

    // mbedtls_md_context_t ctx;
    // mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    // mbedtls_md_init(&ctx);
    // mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    // mbedtls_md_starts(&ctx);
    // mbedtls_md_update(&ctx, (const unsigned char *) derivedAdvertismentKey, advKey_size);
    // mbedtls_md_finish(&ctx, derivedAdvertisementKeyHash);
    // mbedtls_md_free(&ctx);

    // mbedtls_printf("DerivedAdvertismentKey SHA256 HASH: \n");
    // for(int i = 0; i < advKey_size; i++){
    //     printf("%02x", derivedAdvertisementKeyHash[i]);
    // }
    // printf("\n");




}

// si no existe sk_i ni d_0:
//      leer sk_0 y d_0 de la consola
// else:
//      leer sk_i y d_0 de flash
// sk_i = kdf(sk_0, "update",32)
// flash sk_i
// loop:
//      - adv, sk_i = derivar(sk_i)
//      - send(adv) por bt
//      - sleep de cuanto?
//

// seconds on
// last updated
// sk_i
// d_0

// main pide por consola sk_0 y d_0 y lo pasa a la funcion app_main(cambiar nombre de funcion) (que la ejecuta en bucle) que ejecuta todos los pasos


// void main(void){
    // if(!sk_i && !d_0){  scanf("%s", &sk_0);scanf("%s", &d_0); }
    // else{  leer sk_i y d_0 de flash }
    // save date on flash
    // sk_i = kdf(sk_0, "update",32)
    // flash sk_i
    // while(1){ app_main(sk_i, d_0);  send(adv) por bt; sleep(cuanto?); }
// }
 