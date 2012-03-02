
#ifndef crc_h
#define crc_h

#include <stdint.h>
#include <limits.h>

#define CRCPOLY1 0x04C11DB7
/* x^{32}+x^{26}+x^{23}+x^{22}+x^{16}+x^{12}+x^{11]+
   x^{10}+x^8+x^7+x^5+x^4+x^2+x^1+1 */
//#define CRCPOLY2 0xEDB88320

void mk_crc32_table(void);
uint32_t crc32(uint8_t* c, int n);

#endif // !crc_h
