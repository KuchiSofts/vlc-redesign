/*****************************************************************************
 * simple.c : simple channel mixer plug-in
 *****************************************************************************
 * Copyright (C) 2002, 2004, 2006-2009 VLC authors and VideoLAN
 *
 * Authors: Gildas Bazin <gbazin@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_aout.h>
#include <vlc_filter.h>
#include <vlc_block.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  OpenFilter( vlc_object_t * );

vlc_module_begin ()
    set_description( N_("Audio filter for simple channel mixing") )
    set_subcategory( SUBCAT_AUDIO_AFILTER )
    set_capability( "audio converter", 10 )
    set_callback( OpenFilter )
vlc_module_end ()

static block_t *Filter( filter_t *, block_t * );

typedef struct
{
    void (*work)( filter_t *, block_t *, block_t * );
    unsigned in_nb, out_nb;
    float coefs[2][AOUT_CHAN_MAX]; /* output channel, input channel */
} filter_sys_t;

/* Stereo and mono from any layout. The per-layout routines read 6.1 in
 * another channel order (the left side went to the centre, the right side
 * and the rear centre to the wrong outputs at full level), put the centre of
 * 3.0 and 4.0 at full level over halved fronts, and others had none: the
 * rears of quad, and everything but the fronts of 4.1 and 6.0, were lost. */
static void DoWork_matrix( filter_t *p_filter, block_t *p_in_buf, block_t *p_out_buf )
{
    const filter_sys_t *p_sys = p_filter->p_sys;
    float *p_dest = (float *)p_out_buf->p_buffer;
    const float *p_src = (const float *)p_in_buf->p_buffer;
    for( unsigned i = p_in_buf->i_nb_samples; i--; )
    {
        for( unsigned o = 0; o < p_sys->out_nb; o++ )
        {
            float v = 0.f;
            for( unsigned c = 0; c < p_sys->in_nb; c++ )
                v += p_sys->coefs[o][c] * p_src[c];
            *p_dest++ = v;
        }
        p_src += p_sys->in_nb;
    }
}

/* ITU-R BS.775 levels: centre and surrounds at -3 dB on their side, the LFE
 * left out. Side and back surrounds together (7.x) are -6 dB each, and a
 * rear centre -6 dB on both sides, as through 5.1 first. Mono is the mean
 * of the two sides. */
static void SetupMatrix( filter_sys_t *p_sys, uint32_t input, bool mono )
{
    const bool both_pairs = ( input & AOUT_CHANS_MIDDLE ) && ( input & AOUT_CHANS_REAR );
    const float surround = both_pairs ? 0.5f : 0.7071f;
    unsigned c = 0;

    for( unsigned i = 0; pi_vlc_chan_order_wg4[i]; i++ )
    {
        const uint32_t chan = pi_vlc_chan_order_wg4[i];
        if( !( input & chan ) )
            continue;

        float l = 0.f, r = 0.f;
        switch( chan )
        {
            case AOUT_CHAN_LEFT:        l = 1.f; break;
            case AOUT_CHAN_RIGHT:       r = 1.f; break;
            case AOUT_CHAN_CENTER:      l = r = 0.7071f; break;
            case AOUT_CHAN_MIDDLELEFT:
            case AOUT_CHAN_REARLEFT:    l = surround; break;
            case AOUT_CHAN_MIDDLERIGHT:
            case AOUT_CHAN_REARRIGHT:   r = surround; break;
            case AOUT_CHAN_REARCENTER:  l = r = 0.5f; break;
            default:                    break; /* LFE */
        }
        if( mono )
            /* a centre alone stays as it is */
            p_sys->coefs[0][c] = ( input & ~AOUT_CHAN_LFE ) == AOUT_CHAN_CENTER
                               ? ( chan == AOUT_CHAN_CENTER ? 1.f : 0.f )
                               : 0.5f * ( l + r );
        else
        {
            p_sys->coefs[0][c] = l;
            p_sys->coefs[1][c] = r;
        }
        c++;
    }
    p_sys->in_nb = c;
    p_sys->out_nb = mono ? 1 : 2;
    p_sys->work = DoWork_matrix;
}

static void DoWork_7_x_to_4_0( filter_t * p_filter,  block_t * p_in_buf, block_t * p_out_buf ) {
    float *p_dest = (float *)p_out_buf->p_buffer;
    const float *p_src = (const float *)p_in_buf->p_buffer;
    for( int i = p_in_buf->i_nb_samples; i--; )
    {
        *p_dest++ = p_src[6] + 0.5f * p_src[0] + p_src[2] / 6;
        *p_dest++ = p_src[6] + 0.5f * p_src[1] + p_src[3] / 6;
        *p_dest++ = p_src[2] / 6 +  p_src[4];
        *p_dest++ = p_src[3] / 6 +  p_src[5];

        p_src += 7;

        if( p_filter->fmt_in.audio.i_physical_channels & AOUT_CHAN_LFE ) p_src++;
    }
}

static void DoWork_5_x_to_4_0( filter_t * p_filter,  block_t * p_in_buf, block_t * p_out_buf ) {
    float *p_dest = (float *)p_out_buf->p_buffer;
    const float *p_src = (const float *)p_in_buf->p_buffer;
    for( int i = p_in_buf->i_nb_samples; i--; )
    {
        float ctr = p_src[4] * 0.7071f;
        *p_dest++ = p_src[0] + ctr;
        *p_dest++ = p_src[1] + ctr;
        *p_dest++ = p_src[2];
        *p_dest++ = p_src[3];

        p_src += 5;

        if( p_filter->fmt_in.audio.i_physical_channels & AOUT_CHAN_LFE ) p_src++;
    }
}

static void DoWork_7_x_to_5_x( filter_t * p_filter,  block_t * p_in_buf, block_t * p_out_buf ) {
    float *p_dest = (float *)p_out_buf->p_buffer;
    const float *p_src = (const float *)p_in_buf->p_buffer;
    for( int i = p_in_buf->i_nb_samples; i--; )
    {
        *p_dest++ = p_src[0];
        *p_dest++ = p_src[1];
        *p_dest++ = (p_src[2] + p_src[4]) * 0.5f;
        *p_dest++ = (p_src[3] + p_src[5]) * 0.5f;
        *p_dest++ = p_src[6];

        p_src += 7;

        if( p_filter->fmt_in.audio.i_physical_channels & AOUT_CHAN_LFE &&
            p_filter->fmt_out.audio.i_physical_channels & AOUT_CHAN_LFE )
            *p_dest++ = *p_src++;
        else if( p_filter->fmt_in.audio.i_physical_channels & AOUT_CHAN_LFE ) p_src++;
    }
}

static void DoWork_6_1_to_5_x( filter_t * p_filter,  block_t * p_in_buf, block_t * p_out_buf ) {
    VLC_UNUSED(p_filter);
    float *p_dest = (float *)p_out_buf->p_buffer;
    const float *p_src = (const float *)p_in_buf->p_buffer;
    for( int i = p_in_buf->i_nb_samples; i--; )
    {
        *p_dest++ = p_src[0];
        *p_dest++ = p_src[1];
        *p_dest++ = (p_src[2] + p_src[4]) * 0.5f;
        *p_dest++ = (p_src[3] + p_src[4]) * 0.5f;
        *p_dest++ = p_src[5];

        p_src += 6;

        /* We always have LFE here */
        *p_dest++ = *p_src++;
    }
}

#if defined (CAN_COMPILE_NEON)
#include "simple_neon.h"
#define GET_WORK(in, out) GET_WORK_##in##_to_##out##_neon()
#else
#define GET_WORK(in, out) DoWork_##in##_to_##out
#endif

/*****************************************************************************
 * OpenFilter:
 *****************************************************************************/
static int OpenFilter( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t *)p_this;
    void (*do_work)(filter_t *, block_t *, block_t *) = NULL;

    if( p_filter->fmt_in.audio.i_format != VLC_CODEC_FL32 ||
        p_filter->fmt_in.audio.i_format != p_filter->fmt_out.audio.i_format ||
        p_filter->fmt_in.audio.i_rate != p_filter->fmt_out.audio.i_rate ||
        p_filter->fmt_in.audio.channel_type != AUDIO_CHANNEL_TYPE_BITMAP ||
        aout_FormatNbChannels( &p_filter->fmt_in.audio) < 2 )
        return VLC_EGENERIC;

    uint32_t input = p_filter->fmt_in.audio.i_physical_channels;
    uint32_t output = p_filter->fmt_out.audio.i_physical_channels;

    /* Short circuit the common case of not remixing */
    if( input == output )
        return VLC_EGENERIC;

    filter_sys_t *p_sys = vlc_obj_malloc( p_this, sizeof(*p_sys) );
    if( unlikely(p_sys == NULL) )
        return VLC_ENOMEM;

    if( output == AOUT_CHAN_CENTER || output == AOUT_CHANS_2_0 )
        SetupMatrix( p_sys, input, output == AOUT_CHAN_CENTER );
    else
        p_sys->work = NULL;

    const bool b_input_6_1 = input == AOUT_CHANS_6_1_MIDDLE;

    input &= ~AOUT_CHAN_LFE;

    const bool b_input_7_x = input == AOUT_CHANS_7_0;
    const bool b_input_5_x = input == AOUT_CHANS_5_0
                          || input == AOUT_CHANS_5_0_MIDDLE;

    /*
     * TODO: We don't support any 8.1 input
     * TODO: We don't support any 6.x input to 4.0
     */
    if( p_sys->work != NULL )
        do_work = p_sys->work;
    else if( output == AOUT_CHANS_4_0 )
    {
        if( b_input_7_x )
            do_work = GET_WORK(7_x,4_0);
        else if( b_input_5_x )
            do_work = GET_WORK(5_x,4_0);
    }
    else if( (output & ~AOUT_CHAN_LFE) == AOUT_CHANS_5_0 ||
             (output & ~AOUT_CHAN_LFE) == AOUT_CHANS_5_0_MIDDLE )
    {
        if( b_input_7_x )
            do_work = GET_WORK(7_x,5_x);
        else if( b_input_6_1 )
            do_work = GET_WORK(6_1,5_x);
    }

    if( do_work == NULL )
    {
        vlc_obj_free( p_this, p_sys );
        return VLC_EGENERIC;
    }

    static const struct vlc_filter_operations filter_ops =
        { .filter_audio = Filter };

    p_sys->work = do_work;
    p_filter->ops = &filter_ops;
    p_filter->p_sys = p_sys;
    return VLC_SUCCESS;
}

/*****************************************************************************
 * Filter:
 *****************************************************************************/
static block_t *Filter( filter_t *p_filter, block_t *p_block )
{
    const filter_sys_t *p_sys = p_filter->p_sys;

    if( !p_block || !p_block->i_nb_samples )
    {
        if( p_block )
            block_Release( p_block );
        return NULL;
    }

    size_t i_out_size = p_block->i_nb_samples *
      p_filter->fmt_out.audio.i_bitspersample *
        p_filter->fmt_out.audio.i_channels / 8;

    block_t *p_out = block_Alloc( i_out_size );
    if( !p_out )
    {
        msg_Warn( p_filter, "can't get output buffer" );
        block_Release( p_block );
        return NULL;
    }

    p_out->i_nb_samples = p_block->i_nb_samples;
    p_out->i_dts = p_block->i_dts;
    p_out->i_pts = p_block->i_pts;
    p_out->i_length = p_block->i_length;

    int i_input_nb = aout_FormatNbChannels( &p_filter->fmt_in.audio );
    int i_output_nb = aout_FormatNbChannels( &p_filter->fmt_out.audio );
    p_out->i_nb_samples = p_block->i_nb_samples;
    p_out->i_buffer = p_block->i_buffer * i_output_nb / i_input_nb;

    p_sys->work( p_filter, p_block, p_out );

    block_Release( p_block );

    return p_out;
}
