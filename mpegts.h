
#ifndef mpegts_h
#define mpegts_h

#include "mpegts_types.h"

int find_packet_start(int fd);
void print_ts_header(struct mpegts_header* header);
void print_section_header(struct mpegts_section_header* header);
int parse_adaptation_field(uint8_t* packet, struct mpegts_adaptation_field* af);
int read_pat(uint8_t* payload);

#endif // !mpegts_h
