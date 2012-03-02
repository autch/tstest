
#define _LARGEFILE64_SOURCE
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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
  off64_t origin, offset;
  uint8_t* p;
  off64_t ofs_to_sync = 0, last_ofs_to_sync = -1;
  int packet_size;
  
  origin = offset = lseek64(fd, 0, SEEK_CUR);
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
        printf("SYNC: packet size: %d bytes\n", packet_size);
        printf("Seek to first sync'ed offset: %08lx\n", last_ofs_to_sync);
        lseek64(fd, last_ofs_to_sync, SEEK_SET);
        return packet_size;
      }
      p++;
    }
    offset += ret;
  }

  printf("Cannot find SYNC_BYTE, is this MPEG2-TS?\n");

  return 0;
}

void print_ts_header(struct mpegts_header* header)
{
  printf("H: RAW:[%02x%02x%02x%02x] S:%02x [%c%c%c%c%c%c] PID:%04x, CC:%02d\n",
         header->sync_byte, header->b2, header->PID_lo, header->b4,
         header->sync_byte, 
         header->transport_error_indicator ? 'E' : '_',
         header->payload_unit_start_indicator ? 'U' : '_',
         header->transport_priority ? '!' : '_',
         header->transport_scrambling_control ? 'S' : '_',
         HAS_AF(*header) ? 'A' : '_',
         HAS_PAYLOAD(*header) ? 'P' : '_',
         PID_OF(*header),
         header->continuity_counter
    );
}

int parse_adaptation_field(uint8_t* packet, struct mpegts_adaptation_field* af_p)
{
  struct mpegts_header* header;
  struct mpegts_adaptation_field af;
  uint8_t* p;

  memset(&af, 0, sizeof af);
  header = (struct mpegts_header*)packet;
  p = packet + sizeof(struct mpegts_header);

  if(header->payload_unit_start_indicator)
  {
    p++;
  }

  if(!HAS_AF(*header))
  {
    if(af_p) *af_p = af;
    return p - packet;
  }
  
  af.adaptation_field_length = *(uint8_t*)p++;
  if(af.adaptation_field_length == 0)
  {
    if(af_p) *af_p = af;
    return p - packet;
  }
  
  af.adaptation_field_header = *(uint8_t*)p++;

  if(af.PCR_flag)
  {
    memcpy(af.pcr_bytes, p, sizeof af.pcr_bytes);
    p += sizeof af.pcr_bytes;
  }
  if(af.OPCR_flag)
  {
    memcpy(af.opcr_bytes, p, sizeof af.opcr_bytes);
    p += sizeof af.opcr_bytes;
  }
  if(af.splicing_point_flag)
  {
    af.splice_countdown = *p++;
  }
  if(af.transport_private_data_flag)
  {
    af.transport_private_data_length = *p++;
    memcpy(af.private_data_byte, p, af.transport_private_data_length);
    p += af.transport_private_data_length;
  }
  if(af.adaptation_field_extension_flag)
  {
    af.adaptation_field_extension_length = *p++;
    af.afe_b1 = *p++;
    if(af.ltw_flag)
    {
      memcpy(&af.ltw, p, sizeof af.ltw);
      p += sizeof af.ltw;
    }
    if(af.piecewise_rate_flag)
    {
      memcpy(af.piecewise_bytes, p, sizeof af.piecewise_bytes);
      p += sizeof af.piecewise_bytes;
    }
    if(af.seamless_splice_flag)
    {
      memcpy(af.seamless_splice_bytes, p, sizeof af.seamless_splice_bytes);
      p += sizeof af.seamless_splice_bytes;
    }
  }

  if(af_p) *af_p = af;
  return p - packet;
}

int read_pat(uint8_t* payload)
{
  struct mpegts_section_header* header;
  struct mpegts_program_association_section_entry* p;
  struct mpegts_program_association_section_entry* pe;

  uint32_t size, crc;

  header = (struct mpegts_section_header*)payload;

  size = SECTION_LENGTH_OF(*header) + 3 - 4;
  crc = crc32(payload, size);
  printf("CRC: %08x\n", crc);

  FIX_ORDER_S(header->transport_stream_id);
  print_section_header(header);

  p = payload + sizeof(struct mpegts_section_header);
  pe = payload + SECTION_LENGTH_OF(*header) + 3 - 4;
  while(p != pe)
  {
    FIX_ORDER_S(p->program_number);
    FIX_ORDER_S(p->program_map_PID);
    p->network_PID &= 0x1fff;

    printf("PAT: PROG:%04x => PID:%04x\n",
           p->program_number, p->program_map_PID);
    p++;
  }

  return 0;
}

void print_section_header(struct mpegts_section_header* header)
{
  printf("SH: ID:%02x SSI:%d, Z:%d, size:%d\n",
         header->table_id,
         header->section_syntax_indicator,
         header->zero,
         SECTION_LENGTH_OF(*header));
}
