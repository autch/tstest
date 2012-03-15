#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpegts.h"
#include "crc.h"
#include "bitstream.h"

void dump_in_hex(uint8_t* p, int size);

int main(int ac, char** av)
{
    int fd;
    int ret;
    int packet_size;
    uint8_t* buffer;

    if(ac != 2)
    {
        printf("Usage: %s filename.ts\n", *av);
        return 1;
    }

    mk_crc32_table();

    av++;
  
    fd = open(*av, O_RDONLY);
    if(fd < 0)
    {
        perror("open");
        return 1;
    }

    packet_size = find_packet_start(fd);
    if(packet_size)
    {
        BITS b;
        int packet_head_offset = packet_size - STANDARD_PACKET_LENGTH;

        memset(&b, 0, sizeof b);

        buffer = malloc(packet_size);

        while((ret = read(fd, buffer, packet_size)) == packet_size)
        {
            struct mpegts_header header;
            struct mpegts_adaptation_field af;
            uint8_t* packet_head = buffer + packet_head_offset;
            uint8_t pes_header_offset = 0;

            bits_reset(&b, buffer, packet_size);

            if(packet_size == TIMESTAMPED_PACKET_LENGTH) {
                uint32_t clock;
                clock = bits_get(&b, 32);
                printf("CLK: %08x: ", clock);
            }

            read_ts_header(&b, &header);

            if(header.payload_unit_start_indicator)
                pes_header_offset = bits_get(&b, 8);

            parse_adaptation_field(&b, &header, &af);
            print_ts_header(packet_head, &header);
            switch(header.pid)
            {
            case PID_PAT:
                // case 0x0050:
                read_pat(&b);
                break;
            case PID_NULL:
            default:
                break;
            }
        }
        free(buffer);
    }

    close(fd);

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
