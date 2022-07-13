
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "esp_system.h"
#include "esp_spi_flash.h"

#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

void print_hex(const unsigned char *buf, size_t len, bool space);

void print_chip_info(void);


#endif