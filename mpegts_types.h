
#ifndef mpegts_types_h
#define mpegts_types_h

#include <stdint.h>

#define STANDARD_PACKET_LENGTH 188
#define TIMESTAMPED_PACKET_LENGTH 192
#define FEC_PACKET_LENGTH 204

#define SYNC_BYTE 0x47

struct mpegts_header
{
    uint8_t sync_byte; // 0x47

    int transport_error_indicator; // 1 if there is at least one uncorrectable error(s)
    int payload_unit_start_indicator; // if this is PES/PSI, 1: this has 1st byte of PES/PSI
    int transport_priority; // 1 means this has higher priority than other same PID packets

    uint16_t pid;

    int transport_scrambling_control; // 00: not scrambled, others: user-defined
    int adaptation_field_control; // see IS_AFC() / IS_DATA macro, 00 is reserved for future use
    int continuity_counter; //
};

#define PID_PAT		0x0000
#define PID_CAT		0x0001
#define PID_TSDT	0x0002
#define PID_NULL	0x1fff

// is there an adaptation field?
#define HAS_AF(h)	((h)->adaptation_field_control & 2)

// is there any data?
#define HAS_PAYLOAD(h)	((h)->adaptation_field_control & 1)

struct mpegts_adaptation_field
{
    uint8_t adaptation_field_length;

    int discontinuity_indicator;
    int random_access_indicator;
    int elementary_stream_priority_indicator;
    int PCR_flag;
    int OPCR_flag;
    int splicing_point_flag;
    int transport_private_data_flag;
    int adaptation_field_extension_flag;

    int program_clock_reference_extension;
    int pcr_reserved;
    uint64_t program_clock_reference_base;

    int original_program_clock_reference_extension;
    int opcr_reserved;
    uint64_t original_program_clock_reference_base;

    uint8_t splice_countdown;

    uint8_t transport_private_data_length;
    uint8_t private_data_byte[256];

    uint8_t adaptation_field_extension_length;

    int ltw_flag;
    int piecewise_rate_flag;
    int seamless_splice_flag;
    int afe_reserved;
    
    int ltw_valid_flag;
    uint16_t ltw_offset;

    int pr_reserved;
    uint32_t piecewise_rate;

    int splice_type;
    int DTS_next_AU_h;
    int marker_bit_h;
    uint16_t DTS_next_AU_m;
    int marker_bit_m;
    uint16_t DTS_next_AU_l;
    int marker_bit_l;
};

struct mpegts_section_header
{
    uint8_t table_id;

    int section_syntax_indicator;
    int zero;
    int b2_reserved;

    uint16_t section_length;
};

struct mpegts_program_association_section_entry;
struct mpegts_program_association_section_header
{
    uint16_t transport_stream_id;

    int reserved;
    int version_number;
    int current_next_indicator;

    uint8_t section_number;
    uint8_t last_section_number;

    size_t count;
    struct mpegts_program_association_section_entry* entries;
};

struct mpegts_program_association_section_entry
{
    uint16_t program_number;
    int reserved;

    union
    {
        uint16_t network_PID;
        uint16_t program_map_PID;
    };
};

struct mpegts_program_map_section_entry;
struct mpegts_program_map_section_header
{
    uint16_t program_number;
    int reserved1;
    int version_number;
    int current_next_indicator;
    uint8_t section_number;
    uint8_t last_section_number;
    int reserved2;
    uint16_t PCR_PID;
    int reserved3;
    uint16_t program_info_length;

    // descriptor()

    size_t count;
    struct mpegts_program_map_section_entry* entries;
};

struct mpegts_program_map_section_entry
{
    uint8_t stream_type;
    int reserved1;
    uint16_t elementary_PID;
    int reserved2;
    uint16_t ES_info_length;

    // descriptor()
};

#endif // !mpegts_types_h
