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

int parse_adaptation_field(BITS* b, struct mpegts_header* header,
                           struct mpegts_adaptation_field* af)
{
    memset(af, 0, sizeof *af);

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
        for(i = 0; i < af->transport_private_data_length; i++)
        {
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
