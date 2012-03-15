/** @file bistream.c
 * MSBは左

 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <stdint.h>
#include <stdlib.h>

#include "bitstream.h"

BITS* bits_new(void* p, size_t size)
{
    BITS* b;

    if(p == NULL || size <= 0) return NULL;

    b = calloc(1, sizeof(BITS));
    if(b == NULL) return NULL;

    bits_reset(b, p, size);

    return b;
}

void bits_reset(BITS* b, void* p, size_t size)
{
    if(p == NULL) return;

    b->p = b->pe = p;
    b->size = size;
    b->pe += b->size;
    b->value = *(b->p);
    b->bits_avail = 8;
}

void bits_free(BITS* b)
{
    free(b);
}

/**
 */
int bits_fillnext(BITS* b)
{
    if(b == NULL || b->p == b->pe) return -1;
    if(b->bits_avail != 0) return 0;

    b->p++;
    if(b->p == b->pe) return -1;

    b->value = *(b->p);
    b->bits_avail = 8;

    return 1;
}

/**
 * 1ビットだけ取ってくる
 */
int bits_getbit(BITS* b)
{
    int ret;

    ret = (b->value >> 7) & 1;
    b->value <<= 1;
    if(--b->bits_avail == 0)
        bits_fillnext(b);
    
    return ret;
}

/**
 * p の現在地から上位 bits ビットを右詰めにして返す
 *
 */
int64_t bits_get(BITS* b, int bits)
{
    int64_t ret = 0;

    while(bits > 0) {
        uint8_t mask;
        int v, bits_to_use;

        if(bits < b->bits_avail) {
            bits_to_use = bits;
            bits = 0;
        } else {
            bits_to_use = b->bits_avail;
            bits -= b->bits_avail;
        }
        mask = (1 << bits_to_use) - 1;
        v = (b->value >> (8 - bits_to_use)) & mask;
        ret <<= bits_to_use;
        ret |= v;
        b->value <<= bits_to_use;
        b->bits_avail -= bits_to_use;
        if(b->bits_avail == 0 && bits_fillnext(b) < 0)
            break;
    }
    return ret;
}
