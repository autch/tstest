#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <inttypes.h>

struct bitstream
{
    uint8_t* p;
    uint8_t* pe;
    size_t size;

    uint8_t value;

    int bits_avail;
};

typedef struct bitstream BITS;

BITS* bits_new(void* p, size_t size);
void bits_reset(BITS* b, void* p, size_t size);
void bits_free(BITS* b);

int bits_getbit(BITS* b);
int64_t bits_get(BITS* b, int bits);


#endif // !BITSTREAM_H
