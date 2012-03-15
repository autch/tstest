#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <memory.h>

#include "mpegts.h"
#include "crc.h"

// BUFSIZE must be larger than typical packet size
#define BUFSIZE		4096


/**
 * seek to beginning of packet, then return bytes of it
 *
 * @param fd UNIX file descriptor
 * @return size, in bytes, of the packet
 */
int find_packet_start(int fd)
{
    uint8_t buffer[BUFSIZE];
    int ret;
    off_t origin, offset;
    uint8_t* p;
    off_t ofs_to_sync = 0, last_ofs_to_sync = -1;
    int packet_size;
  
    origin = offset = lseek(fd, 0, SEEK_CUR);
    while((ret = read(fd, buffer, sizeof buffer)) > 0) {
        p = buffer;
        while((p = memchr(p, SYNC_BYTE, ret)) != NULL)
        {
            ofs_to_sync = offset + p - buffer;
            // printf("Found SYNC_BYTE at %08lx\n", ofs_to_sync);
            if(last_ofs_to_sync == -1)
            {
                last_ofs_to_sync = ofs_to_sync;
            }

            packet_size = ofs_to_sync - last_ofs_to_sync;
            if(packet_size == STANDARD_PACKET_LENGTH
               || packet_size == TIMESTAMPED_PACKET_LENGTH
               || packet_size == FEC_PACKET_LENGTH)
            {
                int packet_head_offset = packet_size - STANDARD_PACKET_LENGTH;

                printf("SYNC: packet size: %d bytes\n", packet_size);
                printf("Seek to first sync'ed offset: %08lx\n", last_ofs_to_sync - packet_head_offset);
                lseek(fd, last_ofs_to_sync - packet_head_offset, SEEK_SET);
                return packet_size;
            }
            p++;
        }
        offset += ret;
    }

    printf("Cannot find SYNC_BYTE, is this MPEG2-TS?\n");

    return 0;
}

int read_ts_header(BITS* b, struct mpegts_header* header)
{
    memset(header, 0, sizeof(struct mpegts_header));

    header->sync_byte = bits_get(b, 8);

    header->transport_error_indicator = bits_getbit(b);
    header->payload_unit_start_indicator = bits_getbit(b);
    header->transport_priority = bits_getbit(b);
    header->pid = bits_get(b, 13);

    header->transport_scrambling_control = bits_get(b, 2);
    header->adaptation_field_control = bits_get(b, 2);
    header->continuity_counter = bits_get(b, 4);

    return 0;
}

void print_ts_header(uint8_t* buffer, struct mpegts_header* header)
{
    printf("H: RAW:[%02x%02x%02x%02x] S:%02x [%c%c%c%c%c%c] PID:%04x, CC:%02d\n",
           buffer[0], buffer[1], buffer[2], buffer[3],
           header->sync_byte, 
           header->transport_error_indicator ? 'E' : '_',
           header->payload_unit_start_indicator ? 'U' : '_',
           header->transport_priority ? '!' : '_',
           header->transport_scrambling_control ? 'S' : '_',
           HAS_AF(header) ? 'A' : '_',
           HAS_PAYLOAD(header) ? 'P' : '_',
           header->pid,
           header->continuity_counter
        );
}

