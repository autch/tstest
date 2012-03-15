#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tstest.h"
#include "pidmap.h"
#include "mpegts.h"
#include "crc.h"
#include "bitstream.h"

void dump_in_hex(uint8_t* p, int size);

int main(int ac, char** av)
{
    TSTEST ctx;
    int ret;

    memset(&ctx, 0, sizeof ctx);

    if(ac != 2)
    {
        printf("Usage: %s filename.ts\n", *av);
        return 1;
    }

    mk_crc32_table();

    av++;
  
    ctx.fd = open(*av, O_RDONLY);
    if(ctx.fd < 0)
    {
        perror("open");
        return 1;
    }

    pidmap_init(&ctx);
    pidmap_register(&ctx, PID_PAT, read_section);

    ctx.packet_size = find_packet_start(ctx.fd);
    if(ctx.packet_size)
    {
        BITS b;

        memset(&b, 0, sizeof b);
        ctx.packet_head_offset = ctx.packet_size - STANDARD_PACKET_LENGTH;
        ctx.buffer = malloc(ctx.packet_size);

        while((ret = read(ctx.fd, ctx.buffer, ctx.packet_size)) == ctx.packet_size)
        {
            PACKET pctx;

            pctx.b = &b;
            pctx.buffer = ctx.buffer;

            uint8_t* packet_head = ctx.buffer + ctx.packet_head_offset;
            uint8_t pes_header_offset = 0;

            bits_reset(&b, ctx.buffer, ctx.packet_size);

            if(ctx.packet_size == TIMESTAMPED_PACKET_LENGTH) {
                uint32_t clock;
                clock = bits_get(&b, 32);
                printf("CLK: %08x: ", clock);
            }

            read_ts_header(&b, &pctx.header);

            if(pctx.header.payload_unit_start_indicator)
                pes_header_offset = bits_get(&b, 8);

            parse_adaptation_field(&b, &pctx.header, &pctx.af);
            print_ts_header(packet_head, &pctx.header);

            {
                PIDMAP* item;

                item = pidmap_findpid(&ctx, pctx.header.pid);
                if(item != NULL)
                {
                    item->fn(&ctx, &pctx);
                }
            }
            if(ctx.hexdump)
                dump_in_hex(ctx.buffer, ctx.packet_size);
        }
        free(ctx.buffer);
    }

    pidmap_destroy(&ctx);

    close(ctx.fd);

    return 0;
}

void dump_in_hex(uint8_t* p, int size)
{
    static const char* cHex = "0123456789abcdef";
    int i, offset = 0;
    unsigned char c;
    char line_buffer[80];
    char* pHex;
    char* pChar;

    while(offset < size)
    {
        sprintf(line_buffer, "%02x: %*s-%*s |%*s|",
                offset, (8 * 3 - 1), " ", (8 * 3), " ", 16, " ");
        pHex = strchr(line_buffer, ':') + 2;
        pChar = strchr(line_buffer, '|') + 1;

        for(i = 0; i < 16 && (offset + i) < size; i++)
        {
            c = p[offset + i];
            *pHex++ = cHex[(c >> 4) & 0x0f];
            *pHex++ = cHex[c & 0x0f];
            pHex++;
            *pChar++ = (c >= ' ' && c < 0x7f) ? c : '.';
        }
        printf("%s\n", line_buffer);
        offset += i;
    }
}

