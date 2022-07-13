

#include "utils.h"

void print_hex(const unsigned char *buf, size_t len, bool space)
{
    for (size_t i = 0; i < len; i++) {
        // print with space
        if (space) {
            printf("%02x ", buf[i]);
        } else {
            printf("%02x", buf[i]);
        }
    }
    printf("\n");
}

void print_chip_info(void){
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
}