#ifndef BITSTREAM_H
#define BITSTREAM_H

#include "tstest_types.h"

BITS* bits_new(void* p, size_t size);
void bits_reset(BITS* b, void* p, size_t size);
void bits_free(BITS* b);

int bits_getbit(BITS* b);
int64_t bits_get(BITS* b, int bits);

uint8_t* bits_getptr(BITS* b);

#endif // !BITSTREAM_H
