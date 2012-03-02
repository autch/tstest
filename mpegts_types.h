
#ifndef mpegts_types_h
#define mpegts_types_h

#include <stdint.h>
#include <arpa/inet.h>

#define STANDARD_PACKET_LENGTH 188
#define TIMESTAMPED_PACKET_LENGTH 192
#define FEC_PACKET_LENGTH 204

#define SYNC_BYTE 0x47

#define PID_OF(h)	((h).PID_hi << 8 | (h).PID_lo)

#define FIX_ORDER_S(m) ((m) = htons(m))
#define FIX_ORDER_L(m) ((m) = htonl(m))

struct mpegts_header
{
  uint8_t sync_byte;
  union
  {
    struct
    {
      uint8_t PID_hi : 5;
      uint8_t transport_priority : 1; // 1 means this has higher priority than other same PID packets
      uint8_t payload_unit_start_indicator : 1; // if this is PES/PSI, 1: this has 1st byte of PES/PSI
      uint8_t transport_error_indicator : 1; // 1 if there is at least one uncorrectable error(s)
    } __attribute__((packed));
    uint8_t b2;
  };
  uint8_t PID_lo;
  union
  {
    struct
    {
      uint32_t continuity_counter : 4; //
      uint32_t adaptation_field_control : 2; // see IS_AFC() / IS_DATA macro, 00 is reserved for future use
      uint32_t transport_scrambling_control : 2; // 00: not scrambled, others: user-defined
    } __attribute__((packed));
    uint8_t b4;
  };
} __attribute__((packed));

#define PID_PAT		0x0000
#define PID_CAT		0x0001
#define PID_TSDT	0x0002
#define PID_NULL	0x1fff

// is there an adaptation field?
#define HAS_AF(h)	((h).adaptation_field_control & 2)

// is there any data?
#define HAS_PAYLOAD(h)	((h).adaptation_field_control & 1)

struct mpegts_adaptation_field
{
  uint8_t adaptation_field_length;
  union
  {
    struct
    {
      uint8_t adaptation_field_extension_flag : 1;
      uint8_t transport_private_data_flag : 1;
      uint8_t splicing_point_flag : 1;
      uint8_t OPCR_flag : 1;
      uint8_t PCR_flag : 1;
      uint8_t elementary_stream_priority_indicator : 1;
      uint8_t random_access_indicator : 1;
      uint8_t discontinuity_indicator : 1;
    } __attribute__((packed));
    uint8_t adaptation_field_header;
  };

  union
  {
    struct
    {
      uint64_t program_clock_reference_base : 33;
      uint64_t pcr_reserved : 6;
      uint64_t program_clock_reference_extension : 9;
    } __attribute__((packed));
    uint8_t pcr_bytes[6];
  };

  union
  {
    struct
    {
      uint64_t original_program_clock_reference_base : 33;
      uint64_t opcr_reserved : 6;
      uint64_t original_program_clock_reference_extension : 9;
    } __attribute__((packed));
    uint8_t opcr_bytes[6];
  };

  uint8_t splice_countdown;

  uint8_t transport_private_data_length;
  uint8_t private_data_byte[256];

  struct
  {
    uint8_t adaptation_field_extension_length;
    union
    {
      struct
      {
        uint8_t afe_reserved : 5;
        uint8_t seamless_splice_flag : 1;
        uint8_t piecewise_rate_flag : 1;
        uint8_t ltw_flag : 1;
      } __attribute__((packed));
      uint8_t afe_b1;
    };

    union
    {
      struct
      {
        uint16_t ltw_offset : 15;
        uint16_t ltw_valid_flag : 1;
      } __attribute__((packed));
      uint16_t ltw;
    };

    union
    {
      struct
      {
        uint32_t piecewise_rate : 22;
        uint32_t pr_reserved : 2;
      } __attribute__((packed));
      uint8_t piecewise_bytes[3];
    };

    union
    {
      struct
      {
        uint32_t marker_bit_l : 1;
        uint32_t DTS_next_AU_l : 15;
        uint32_t marker_bit_m : 1;
        uint32_t DTS_next_AU_m : 15;
        uint32_t marker_bit_h : 1;
        uint32_t DTS_next_AU_h : 3;
        uint32_t splice_type : 4;
      } __attribute__((packed));
      uint8_t seamless_splice_bytes[5];
    };
  };
} __attribute__((packed));


#define SECTION_LENGTH_OF(h)	((h).section_length_hi << 8 | (h).section_length_lo)
struct mpegts_section_header
{
  uint8_t table_id;
  union
  {
    struct
    {
      uint8_t section_length_hi : 4;
      uint8_t b2_reserved : 2;
      uint8_t zero : 1;
      uint8_t section_syntax_indicator : 1;
    } __attribute__((packed));
    uint8_t b2;
  };
  uint8_t section_length_lo;
  uint16_t transport_stream_id;
  union
  {
    struct
    {
      uint8_t current_next_indicator : 1;
      uint8_t version_number : 5;
      uint8_t b6_reserved : 2;
    } __attribute__((packed));
    uint8_t b6;
  };
  uint8_t section_number;
  uint8_t last_section_number;
} __attribute__((packed));

struct mpegts_program_association_section_entry
{
  uint16_t program_number;
  union
  {
    uint16_t network_PID;
    uint16_t program_map_PID;
  };
} __attribute__((packed));

#endif // !mpegts_types_h
