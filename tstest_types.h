
#ifndef tstest_types_h
#define tstest_types_h

#include <stdint.h>
#include "mpegts_types.h"

struct bitstream
{
    uint8_t* p;
    uint8_t* pe;
    size_t size;

    uint8_t value;

    int bits_avail;
};
typedef struct bitstream BITS;

struct tstest_packet_ctx
{
    uint8_t* buffer;
    BITS* b;

    struct mpegts_header header;
    struct mpegts_adaptation_field af;
};
typedef struct tstest_packet_ctx PACKET;

struct tstest_pidmap;
typedef struct tstest_pidmap PIDMAP;

struct tstest_ctx
{
    int hexdump;

    PIDMAP* pidmap;
    size_t pidmap_count;

    uint8_t* buffer;

    int fd;
    int packet_size;
    int packet_head_offset;
};
typedef struct tstest_ctx TSTEST;

typedef int (*tstest_pid_fn)(TSTEST* ctx, PACKET* pkt);

#define PIDMAP_NULL 0xffff
struct tstest_pidmap
{
    uint16_t pid;
    tstest_pid_fn fn;
};

#endif // !tstest_types_h
