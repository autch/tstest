
#ifndef mpegts_h
#define mpegts_h

#include "mpegts_types.h"
#include "bitstream.h"

int find_packet_start(int fd);
int read_ts_header(BITS* b, struct mpegts_header* header);
void print_ts_header(uint8_t* buffer, struct mpegts_header* header);
void print_section_header(struct mpegts_section_header* header);
int parse_adaptation_field(BITS* b, struct mpegts_header* header,
                           struct mpegts_adaptation_field* af);
int read_pat(BITS* b);

#endif // !mpegts_h
