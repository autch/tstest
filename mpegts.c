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

int parse_adaptation_field(BITS* b, struct mpegts_header* header,
                           struct mpegts_adaptation_field* af)
{
    memset(af, 0, sizeof af);

    if(!HAS_AF(header))
    {
        return 0;
    }
  
    af->adaptation_field_length = bits_get(b, 8);
    if(af->adaptation_field_length == 0)
    {
        return 0;
    }
  
    af->discontinuity_indicator = bits_getbit(b);
    af->random_access_indicator = bits_getbit(b);
    af->elementary_stream_priority_indicator = bits_getbit(b);
    af->PCR_flag = bits_getbit(b);
    af->OPCR_flag = bits_getbit(b);
    af->splicing_point_flag = bits_getbit(b);
    af->transport_private_data_flag = bits_getbit(b);
    af->adaptation_field_extension_flag = bits_getbit(b);

    if(af->PCR_flag)
    {
        af->program_clock_reference_extension = bits_get(b, 9);
        af->pcr_reserved = bits_get(b, 6);
        af->program_clock_reference_base = bits_get(b, 33);
    }
    if(af->OPCR_flag)
    {
        af->original_program_clock_reference_extension = bits_get(b, 9);
        af->opcr_reserved = bits_get(b, 6);
        af->original_program_clock_reference_base = bits_get(b, 33);
    }
    if(af->splicing_point_flag)
    {
        af->splice_countdown = bits_get(b, 8);
    }
    if(af->transport_private_data_flag)
    {
        int i;

        af->transport_private_data_length = bits_get(b, 8);
        // ダサいが仕方ない…
        for(i = 0; i < af->transport_private_data_length; i++) {
            af->private_data_byte[i] = bits_get(b, 8);
        }
    }
    if(af->adaptation_field_extension_flag)
    {
        af->adaptation_field_extension_length = bits_get(b, 8);

        af->ltw_flag = bits_getbit(b);
        af->piecewise_rate_flag = bits_getbit(b);
        af->seamless_splice_flag = bits_getbit(b);
        af->afe_reserved = bits_get(b, 5);

        if(af->ltw_flag)
        {
            af->ltw_valid_flag = bits_getbit(b);
            af->ltw_offset = bits_get(b, 15);
        }
        if(af->piecewise_rate_flag)
        {
            af->pr_reserved = bits_get(b, 2);
            af->piecewise_rate = bits_get(b, 22);
        }
        if(af->seamless_splice_flag)
        {
            af->splice_type = bits_get(b, 4);
            af->DTS_next_AU_h = bits_get(b, 3);
            af->marker_bit_h = bits_getbit(b);
            af->DTS_next_AU_m = bits_get(b, 15);
            af->marker_bit_m = bits_getbit(b);
            af->DTS_next_AU_l = bits_get(b, 15);
            af->marker_bit_l = bits_getbit(b);
        }
    }

    return 0;
}

int read_pat(BITS* b)
{
    struct mpegts_section_header header;
    uint32_t crc;

    header.table_id = bits_get(b, 8);
    header.section_syntax_indicator = bits_getbit(b);
    header.zero = bits_getbit(b);
    header.b2_reserved = bits_get(b, 2);
    header.section_length = bits_get(b, 12);

    print_section_header(&header);

    switch(header.table_id) {
    case 0x00: {
        // PAT
        struct mpegts_program_association_section_header pat_header;
        struct mpegts_program_association_section_entry e;

        pat_header.transport_stream_id = bits_get(b, 16);
        
        pat_header.reserved = bits_get(b, 2);
        pat_header.version_number = bits_get(b, 5);
        pat_header.current_next_indicator = bits_getbit(b);
        
        pat_header.section_number = bits_get(b, 8);
        pat_header.last_section_number = bits_get(b, 8);
        
        int i, table_size = header.section_length - 5 - 4;
        pat_header.count = table_size / 4;
        for(i = 0; i < pat_header.count; i++) {
            e.program_number = bits_get(b, 16);
            e.reserved = bits_get(b, 3);
            e.program_map_PID = bits_get(b, 13);
            
            printf("PAT: PROG:%04x => PID:%04x\n",
                   e.program_number, e.program_map_PID);
        }
        break;
    }
    case 0x02: {
        // PMT
        struct mpegts_program_map_section_header pmt_header;
        struct mpegts_program_map_section_entry e;
        int i, j;

        pmt_header.program_number = bits_get(b, 16);
        pmt_header.reserved1 = bits_get(b, 2);
        pmt_header.version_number = bits_get(b, 5);
        pmt_header.current_next_indicator = bits_getbit(b);
        pmt_header.section_number = bits_get(b, 8);
        pmt_header.last_section_number = bits_get(b, 8);
        pmt_header.reserved2 = bits_get(b, 3);
        pmt_header.PCR_PID = bits_get(b, 13);
        pmt_header.reserved3 = bits_get(b, 4);
        pmt_header.program_info_length = bits_get(b, 12);
        for(i = 0; i < pmt_header.program_info_length; i++) {
            // 捨てる
            bits_get(b, 8);
        }
        int table_size = header.section_length - 9 - pmt_header.program_info_length - 4;
        for(i = 0; i < table_size; ) {
            e.stream_type = bits_get(b, 8);
            e.reserved1 = bits_get(b, 3);
            e.elementary_PID = bits_get(b, 13);
            e.reserved2 = bits_get(b, 4);
            e.ES_info_length = bits_get(b, 12);
            for(j = 0; j < e.ES_info_length; j++) {
                bits_get(b, 8);
            }

            printf("PMT: STR:%02x => PID:%04x\n", e.stream_type, e.elementary_PID);

            i += 5 + e.ES_info_length;
        }

        break;
    }
    }

    crc = bits_get(b, 32);
    printf("CRC: %08x\n", crc);

    return 0;
}

void print_section_header(struct mpegts_section_header* header)
{
    printf("SH: ID:%02x SSI:%d, Z:%d, size:%d\n",
           header->table_id,
           header->section_syntax_indicator,
           header->zero,
           header->section_length);
}

