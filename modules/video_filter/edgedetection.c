/******************************************************************************
 * edgedetection.c : edge detection plugin for VLC
  *****************************************************************************
 * Copyright (C) 2016 VLC authors and VideoLAN
 *
 * Authors: Odd-Arild Kristensen <oddarildkristensen@gmail.com>
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
#include <vlc_filter.h>
#include <vlc_picture.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define EDGE_DETECTION_DESCRIPTION N_( "Edge detection video filter" )
#define EDGE_DETECTION_TEXT N_( "Edge detection" )
#define EDGE_DETECTION_LONGTEXT N_( \
    "Detects edges in the frame and highlights them in white." )

#define WHITE 255

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int Open( filter_t * );
static void Close( filter_t * );
static picture_t *new_frame( filter_t * );
static picture_t *Filter( filter_t *, picture_t * );
static uint8_t sobel( const uint8_t *, const int, const int, int, int);

/* Kernel for X axis */
static const signed char pi_kernel_x[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

/* Kernel for Y axis */
static const signed char pi_kernel_y[3][3] = {
    {-1, -2, -1},
    {0, 0, 0},
    {1, 2, 1}
};

vlc_module_begin ()

    set_description( EDGE_DETECTION_DESCRIPTION )
    set_shortname( EDGE_DETECTION_TEXT )
    set_help( EDGE_DETECTION_LONGTEXT )
    set_subcategory( SUBCAT_VIDEO_VFILTER )
    set_callback_video_filter( Open )

vlc_module_end ()

static const struct filter_video_callbacks filter_video_edge_cbs =
{
    new_frame, NULL,
};

static void Flush( filter_t *p_filter )
{
    filter_chain_t *p_sys = p_filter->p_sys;
    filter_chain_VideoFlush( p_sys );
}

static const struct vlc_filter_operations filter_ops =
{
    .filter_video = Filter, .flush = Flush, .close = Close,
};

/*****************************************************************************
 * Opens the filter.
 * Allocates and initializes data needed by the filter. The image needs to
 * be black-and-white in order to detect any edges. The Gaussian blur is
 * needed so that the Sobel operator does not give a high response for noise,
 * or small changes in the image.
 *****************************************************************************/
static int Open( filter_t *p_filter  )
{
    int i_ret;
    filter_owner_t owner = {
        .video = &filter_video_edge_cbs,
        .sys = p_filter,
    };
    /* Pictures in memory, the same out as in: its filters write into the
     * pictures of this one (with hardware decoding it took D3D11 pictures,
     * and the blur wrote into pictures without planes) */
    const vlc_chroma_description_t *desc =
        vlc_fourcc_GetChromaDescription( p_filter->fmt_in.video.i_chroma );
    if ( desc == NULL || desc->plane_count == 0
      || !es_format_IsSimilar( &p_filter->fmt_in, &p_filter->fmt_out ) )
        return VLC_EGENERIC;
    /* Store the filter chain in p_sys */
    filter_chain_t *sys = filter_chain_NewVideo( p_filter, true, &owner );
    if ( sys == NULL)
    {
        msg_Err( p_filter, "Could not allocate filter chain" );
        return VLC_EGENERIC;
    }
    /* Clear filter chain */
    filter_chain_Reset( sys, &p_filter->fmt_in, p_filter->vctx_in, &p_filter->fmt_in);
    /* Add adjust filter to turn frame black-and-white */
    i_ret = filter_chain_AppendFromString( sys, "adjust{saturation=0}" );
    if ( i_ret == -1 )
    {
        msg_Err( p_filter, "Could not append filter to filter chain" );
        filter_chain_Delete( sys );
        return VLC_EGENERIC;
    }
    /* Add gaussian blur to the frame so to remove noise from the frame */
    i_ret = filter_chain_AppendFromString( sys, "gaussianblur{deviation=1}" );
    if ( i_ret == -1 )
    {
        msg_Err( p_filter, "Could not append filter to filter chain" );
        filter_chain_Delete( sys );
        return VLC_EGENERIC;
    }
    /* Set callback function */
    p_filter->ops = &filter_ops;
    p_filter->p_sys = sys;
    return VLC_SUCCESS;
}

/******************************************************************************
 * Closes the filter and cleans up all dynamically allocated data.
 ******************************************************************************/
static void Close( filter_t *p_filter )
{
    filter_chain_Delete( (filter_chain_t *)p_filter->p_sys );
}

/* *****************************************************************************
 * Allocates a new buffer for the filter chain.
 ******************************************************************************/
static picture_t *new_frame( filter_t *p_filter )
{
    // the last filter of the internal chain gets pictures in memory of its own
    // format: those of this filter are the next one's (the display's D3D11
    // pictures with hardware decoding, and the blur wrote into pictures
    // without planes). This filter makes its output picture itself.
    return picture_NewFromFormat( &p_filter->fmt_out.video );
}

/******************************************************************************
 * Callback function.
 * This function implements the sobel operator which can be used to detect
 * edges in a black-and-white image. The sobel operator is applied to
 * every pixel in the frame for both X and Y axis.
 ******************************************************************************/
static picture_t *Filter( filter_t *p_filter, picture_t *p_pic )
{
    picture_t *p_filtered_frame =
        filter_chain_VideoFilter( (filter_chain_t *)p_filter->p_sys, p_pic );
    if ( p_filtered_frame == NULL )
        return NULL;
    picture_t *p_out_frame = picture_NewFromFormat( &p_filtered_frame->format );
    if ( p_out_frame == NULL )
    {
        picture_Release( p_filtered_frame );
        msg_Err( p_filter, "Could not allocate memory for new frame" );
        return NULL;
    }
    /* its date (without it, no picture was shown), and no color: the
     * edges are white on black */
    picture_CopyProperties( p_out_frame, p_filtered_frame );
    for ( int i = 1; i < p_out_frame->i_planes; i++ )
        memset( p_out_frame->p[i].p_pixels, 0x80,
                (size_t)p_out_frame->p[i].i_pitch * p_out_frame->p[i].i_lines );
    const int i_lines = p_filtered_frame->p[Y_PLANE].i_visible_lines;
    const int i_pitch = p_filtered_frame->p[Y_PLANE].i_pitch;
    /* Loop through every pixel in the frame */
    for ( int i_line = 0; i_line < i_lines; i_line++ )
    {
        for ( int i_col = 0; i_col < i_pitch; i_col++ )
        {
            /* Set the new value for the pixel */
            *( p_out_frame->p[Y_PLANE].p_pixels + ((i_pitch * i_line) + i_col) ) =
                sobel( p_filtered_frame->p[Y_PLANE].p_pixels,
                       i_pitch, i_lines, i_col, i_line );
        }
    }
    picture_Release( p_filtered_frame );
    return p_out_frame;
}

/******************************************************************************
 * Sobel Operator.
 * Calculates the gradients for both X and Y directions in a frame.
 ******************************************************************************/
static uint8_t sobel( const uint8_t *p_pixels, const int i_pitch,
                      const int i_lines, int i_col, int i_line )
{
    int i_x_val = 0;
    int i_y_val = 0;
    int i_col_offset;
    int i_line_offset;
    for ( int i_x = 0; i_x < 3; i_x++ )
    {
        /* Check if we are at the first pixel on the scanline */
        if (i_x == 0 && i_col == 0)
        {
            /* Duplicate the first pixel on the scanline */
            i_col_offset = 0;
        }
        /* Check if we are on the last pixel on the scanline */
        else if (i_x == 2 && i_col == i_pitch - 1)
        {
            /* Duplicate the last pixel on the scanline */
            i_col_offset = i_pitch - 1;
        }
        else
        {
            i_col_offset = i_col + i_x - 1;
        }
        for ( int i_y = 0; i_y < 3; i_y++ )
        {
            /* Check if we are at the top scanline */
            if (i_y == 0 && i_line == 0)
            {
                /* Duplicate the top pixel */
                i_line_offset = 0;
            }
            /* Check if we are the bottom scanline */
            else if (i_y == 2 && i_line == i_lines - 1 )
            {
                /* Duplicate the bottom pixel */
                i_line_offset = i_pitch * (i_lines - 1);
            }
            else {
                i_line_offset = i_pitch * (i_line + i_y - 1);
            }
            /* Apply the kernel to the pixel */
            i_x_val += pi_kernel_x[i_x][i_y] * p_pixels[i_line_offset + i_col_offset];
            i_y_val += pi_kernel_y[i_x][i_y] * p_pixels[i_line_offset + i_col_offset];
        }
    }
    int i_ret = abs(i_x_val) + abs(i_y_val);
    return (i_ret > WHITE) ? WHITE : i_ret;
}
