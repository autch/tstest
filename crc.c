#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include "crc.h"

uint32_t crc32table[256];

void mk_crc32_table(void)
{
    uint32_t i, j;
    uint32_t r;

    for (i = 0; i < 256; i++) {
        r = (uint32_t)i << 24;
        for (j = 0; j < 8; j++)
            if (r & 0x80000000) r = (r << 1) ^ CRCPOLY1;
            else                  r <<= 1;
        crc32table[i] = r & 0xFFFFFFFF;
    }
}

uint32_t crc32(uint8_t* c, int n)
{
    uint32_t r;

    r = 0xFFFFFFFF;
    while (--n >= 0)
        r = (r << 8) ^ crc32table[((r >> 24) & 0xff) ^ *c++];
    return r & 0xFFFFFFFF;
}
