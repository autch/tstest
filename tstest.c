
#define _LARGEFILE64_SOURCE
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpegts.h"
#include "crc.h"

void dump_in_hex(uint8_t* p, int size);

int main(int ac, char** av)
{
  int fd;
  int ret;
  int packet_size, ofs_to_payload;
  uint8_t* buffer;
  struct mpegts_header header;

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
    buffer = malloc(packet_size);

    while((ret = read(fd, buffer, packet_size)) == packet_size)
    {
      header = *(struct mpegts_header*)(buffer
                                        + (packet_size - STANDARD_PACKET_LENGTH));
      ofs_to_payload = parse_adaptation_field(buffer, NULL);
      print_ts_header(&header);
      switch(PID_OF(header))
      {
      case PID_PAT:
        read_pat(buffer + ofs_to_payload);
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
