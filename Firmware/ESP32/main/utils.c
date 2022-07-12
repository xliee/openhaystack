

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