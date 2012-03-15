#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <memory.h>

#include "mpegts.h"
#include "pidmap.h"
#include "crc.h"

int read_pat_section(TSTEST* ctx, PACKET* pctx, struct mpegts_section_header* sec_header);
int read_pmt_section(TSTEST* ctx, PACKET* pctx, struct mpegts_section_header* sec_header);

int read_section(TSTEST* ctx, PACKET* pctx)
{
    struct mpegts_section_header sec_header;
    BITS* b = pctx->b;
    uint8_t* body;
    uint8_t* body_end;

    body = bits_getptr(b);

    sec_header.table_id = bits_get(b, 8);
    sec_header.section_syntax_indicator = bits_getbit(b);
    sec_header.zero = bits_getbit(b);
    sec_header.b2_reserved = bits_get(b, 2);
    sec_header.section_length = bits_get(b, 12);

    print_section_header(&sec_header);

    switch(sec_header.table_id)
    {
    case TABLE_ID_PAT:
        // PAT
        read_pat_section(ctx, pctx, &sec_header);
        break;
    case TABLE_ID_PMT:
        // PMT
        read_pmt_section(ctx, pctx, &sec_header);
        break;
    }

    body_end = bits_getptr(b);

    uint32_t crc_read, crc;
    crc = crc32(body, body_end - body);

    crc_read = bits_get(b, 32);
    printf("CRC: %s (%08x/%08x)\n",
           (crc == crc_read) ? "[OK]" : "[NG]", crc_read, crc);

    return 0;
}

void print_section_header(struct mpegts_section_header* sec_header)
{
    printf("SH: ID:%02x SSI:%d, Z:%d, size:%d\n",
           sec_header->table_id,
           sec_header->section_syntax_indicator,
           sec_header->zero,
           sec_header->section_length);
}

int read_pat_section(TSTEST* ctx, PACKET* pctx, struct mpegts_section_header* sec_header)
{
    BITS* b = pctx->b;
    struct mpegts_program_association_section_header pat_header;

    pat_header.transport_stream_id = bits_get(b, 16);
        
    pat_header.reserved = bits_get(b, 2);
    pat_header.version_number = bits_get(b, 5);
    pat_header.current_next_indicator = bits_getbit(b);
        
    pat_header.section_number = bits_get(b, 8);
    pat_header.last_section_number = bits_get(b, 8);
        
    int i, table_size = sec_header->section_length - 5 - 4;
    pat_header.count = table_size / 4;
    for(i = 0; i < pat_header.count; i++)
    {
        struct mpegts_program_association_section_entry e;
        e.program_number = bits_get(b, 16);
        e.reserved = bits_get(b, 3);
        e.program_map_PID = bits_get(b, 13);

        pidmap_register(ctx, e.program_map_PID, read_section);

        printf("PAT: PROG:%04x => PMT:%04x\n",
               e.program_number, e.program_map_PID);
    }
    return 0;
}

int read_pmt_section(TSTEST* ctx, PACKET* pctx, struct mpegts_section_header* sec_header)
{
    BITS* b = pctx->b;
    struct mpegts_program_map_section_header pmt_header;
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
    for(i = 0; i < pmt_header.program_info_length; i++)
    {
        // 捨てる
        bits_get(b, 8);
    }
    int table_size = sec_header->section_length - 9 - pmt_header.program_info_length - 4;
    for(i = 0; i < table_size; )
    {
        struct mpegts_program_map_section_entry e;

        e.stream_type = bits_get(b, 8);
        e.reserved1 = bits_get(b, 3);
        e.elementary_PID = bits_get(b, 13);
        e.reserved2 = bits_get(b, 4);
        e.ES_info_length = bits_get(b, 12);
        for(j = 0; j < e.ES_info_length; j++)
        {
            bits_get(b, 8);
        }

        printf("PMT: STR:%02x => PID:%04x\n", e.stream_type, e.elementary_PID);

        i += 5 + e.ES_info_length;
    }

    return 0;
}
