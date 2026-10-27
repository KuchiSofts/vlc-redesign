/*****************************************************************************
 * display.c: display stream output module
 *****************************************************************************
 * Copyright (C) 2001-2011 VLC authors and VideoLAN
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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
#include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_configuration.h>

#include <vlc_decoder.h>
#include <vlc_frame.h>
#include <vlc_plugin.h>
#include <vlc_sout.h>
#include <vlc_vector.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define AUDIO_TEXT N_( "Enable audio" )
#define AUDIO_LONGTEXT N_( "Enable/disable audio rendering." )
#define VIDEO_TEXT N_( "Enable video" )
#define VIDEO_LONGTEXT N_( "Enable/disable video rendering." )
#define DELAY_TEXT N_( "Delay (ms)" )
#define DELAY_LONGTEXT N_( "Introduces a delay in the display of the stream." )

static int  Open ( vlc_object_t * );
static void Close( sout_stream_t * );

#define SOUT_CFG_PREFIX "sout-display-"

vlc_module_begin()
    set_shortname( N_("Display") )
    set_description( N_("Display stream output") )
    set_capability( "sout output", 50 )
    add_shortcut( "display" )
    set_subcategory( SUBCAT_SOUT_STREAM )

    add_bool( SOUT_CFG_PREFIX "audio", true, AUDIO_TEXT,
              AUDIO_LONGTEXT )
    add_bool( SOUT_CFG_PREFIX "video", true, VIDEO_TEXT,
              VIDEO_LONGTEXT )
    add_integer( SOUT_CFG_PREFIX "delay", 100, DELAY_TEXT,
                 DELAY_LONGTEXT )
    set_callback( Open )
vlc_module_end()


/*****************************************************************************
 * Exported prototypes
 *****************************************************************************/
static const char *const ppsz_sout_options[] = {
    "audio", "video", "delay", NULL
};

typedef struct sout_stream_id_sys_t sout_stream_id_sys_t;

typedef struct
{
    bool b_audio;
    bool b_video;

    vlc_tick_t i_delay;
    input_resource_t *p_resource;

    /* Casting with the picture here too: it shows what the renderer shows
     * when it does. The renderer's output says by how much the renderer is
     * behind the times of its stream ("renderer-latency" of libvlc): the
     * frames here are that much later, less what this display waits by
     * itself (Send). It pauses with the renderer ("renderer-paused"). The
     * lock guards these and the decoders' list. */
    vlc_object_t *libvlc;
    vlc_mutex_t lock;
    vlc_tick_t latency;
    /* What the frames get of it: a new measure (every 30 s or so) goes in
     * gradually, so that the picture does not jump or stall; the first one,
     * and at a seek (SetPCR), at once. */
    vlc_tick_t applied_latency;
    vlc_tick_t applied_date;
    bool latency_jump;
    bool paused;
    struct VLC_VECTOR(sout_stream_id_sys_t *) ids;

    struct vlc_sout_clock_bus *main_clock;
    bool first_pcr_signaled;
    bool reanchor;
    bool error;
} sout_stream_sys_t;

struct sout_stream_id_sys_t
{
    vlc_input_decoder_t *dec;
    vlc_clock_t *clock;
};

/* What a frame waits here before it shows, past its time: the clock's
 * margin for the input (DEFAULT_PTS_DELAY) and the delay of the decoders */
static vlc_tick_t LocalDelay( const sout_stream_sys_t *p_sys )
{
    return DEFAULT_PTS_DELAY
         + ( p_sys->i_delay != VLC_TICK_INVALID ? p_sys->i_delay : 0 );
}

static int LatencyChanged( vlc_object_t *obj, char const *var, vlc_value_t old,
                           vlc_value_t cur, void *data )
{
    VLC_UNUSED(obj); VLC_UNUSED(var);
    sout_stream_sys_t *p_sys = data;

    vlc_mutex_lock( &p_sys->lock );
    p_sys->latency = VLC_TICK_FROM_MS( cur.i_int );
    if( old.i_int == 0 )
        p_sys->latency_jump = true; /* the first measure */
    vlc_mutex_unlock( &p_sys->lock );
    return VLC_SUCCESS;
}

/* the decoders and their clock stop and go on with the renderer */
static int PausedChanged( vlc_object_t *obj, char const *var, vlc_value_t old,
                          vlc_value_t cur, void *data )
{
    VLC_UNUSED(obj); VLC_UNUSED(var); VLC_UNUSED(old);
    sout_stream_sys_t *p_sys = data;

    vlc_mutex_lock( &p_sys->lock );
    if( cur.b_bool != p_sys->paused )
    {
        const vlc_tick_t now = vlc_tick_now();
        p_sys->paused = cur.b_bool;
        if( !p_sys->paused )
            sout_ClockMainChangePause( p_sys->main_clock, now, false );
        sout_stream_id_sys_t *id;
        vlc_vector_foreach( id, &p_sys->ids )
            vlc_input_decoder_ChangePause( id->dec, p_sys->paused, now );
        if( p_sys->paused )
            sout_ClockMainChangePause( p_sys->main_clock, now, true );
    }
    vlc_mutex_unlock( &p_sys->lock );
    return VLC_SUCCESS;
}

static void *Add( sout_stream_t *p_stream, const es_format_t *p_fmt, const char *es_id )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    if( (p_fmt->i_cat == AUDIO_ES && !p_sys->b_audio) ||
        (p_fmt->i_cat == VIDEO_ES && !p_sys->b_video) )
    {
        return NULL;
    }

    sout_stream_id_sys_t *id = malloc( sizeof(*id) );
    if( unlikely(id == NULL) )
        return NULL;

    id->clock = sout_ClockCreate( p_sys->main_clock, p_fmt );
    if( unlikely(id->clock == NULL) )
    {
        free( id );
        return NULL;
    }

    id->dec = vlc_input_decoder_Create(
        VLC_OBJECT(p_stream), p_fmt, es_id, id->clock, p_sys->p_resource );
    if( id->dec == NULL )
    {
        msg_Err( p_stream, "cannot create decoder for fcc=`%4.4s'",
                 (char*)&p_fmt->i_codec );
        sout_ClockDelete( id->clock );
        free( id );
        return NULL;
    }

    if( p_sys->i_delay != VLC_TICK_INVALID )
        vlc_input_decoder_ChangeDelay( id->dec, p_sys->i_delay );

    vlc_mutex_lock( &p_sys->lock );
    if( p_sys->paused )
        vlc_input_decoder_ChangePause( id->dec, true, vlc_tick_now() );
    if( !vlc_vector_push( &p_sys->ids, id ) )
        msg_Warn( p_stream, "this decoder will not pause with the renderer" );
    vlc_mutex_unlock( &p_sys->lock );

    return id;
}

static void Del( sout_stream_t *p_stream, void *id )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;
    sout_stream_id_sys_t *id_sys = id;

    vlc_mutex_lock( &p_sys->lock );
    for( size_t i = 0; i < p_sys->ids.size; ++i )
        if( p_sys->ids.data[i] == id_sys )
        {
            vlc_vector_remove( &p_sys->ids, i );
            break;
        }
    vlc_mutex_unlock( &p_sys->lock );

    vlc_input_decoder_Flush(id_sys->dec);
    vlc_input_decoder_Delete( id_sys->dec );
    sout_ClockDelete( id_sys->clock );
    free( id_sys );
}

static int Send( sout_stream_t *p_stream, void *id, vlc_frame_t *frames )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;
    sout_stream_id_sys_t *id_sys = id;

    if( !p_sys->first_pcr_signaled )
    {
        if( !p_sys->error )
        {
            msg_Err( p_stream,
                     "Missing first pcr event. Forced to drop early frames." );
            p_sys->error = true;
        }
        vlc_frame_ChainRelease( frames );
        return VLC_EGENERIC;
    }

    vlc_mutex_lock( &p_sys->lock );
    const vlc_tick_t target = p_sys->latency > LocalDelay( p_sys )
                            ? p_sys->latency - LocalDelay( p_sys ) : 0;
    const vlc_tick_t now = vlc_tick_now();
    if( p_sys->applied_latency == VLC_TICK_INVALID || p_sys->latency_jump )
        p_sys->applied_latency = target;
    else if( p_sys->applied_latency != target )
    {
        /* at most 5% of the time since the last frames (a pause counts
         * as 100 ms): the frames here go slightly slower or faster */
        vlc_tick_t elapsed = now - p_sys->applied_date;
        if( elapsed < 0 )
            elapsed = 0;
        else if( elapsed > VLC_TICK_FROM_MS(100) )
            elapsed = VLC_TICK_FROM_MS(100);
        const vlc_tick_t step = __MAX( elapsed / 20, 1 );
        if( target > p_sys->applied_latency )
            p_sys->applied_latency = __MIN( target, p_sys->applied_latency + step );
        else
            p_sys->applied_latency = __MAX( target, p_sys->applied_latency - step );
    }
    p_sys->latency_jump = false;
    p_sys->applied_date = now;
    const vlc_tick_t latency = p_sys->applied_latency;
    vlc_mutex_unlock( &p_sys->lock );

    while( frames )
    {
        vlc_frame_t *p_next = frames->p_next;

        frames->p_next = NULL;

        /* later by the renderer's delay (the clock goes on: the decoders
         * wait that much) */
        if( latency > 0 )
        {
            if( frames->i_pts != VLC_TICK_INVALID )
                frames->i_pts += latency;
            if( frames->i_dts != VLC_TICK_INVALID )
                frames->i_dts += latency;
        }

        if( id != NULL && frames->i_buffer > 0 )
            vlc_input_decoder_Decode( id_sys->dec, frames, false );

        frames = p_next;
    }

    return VLC_SUCCESS;
}

/* a seek: what the decoders have is dropped, the next PCR starts again */
static void Flush( sout_stream_t *p_stream, void *id )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;
    sout_stream_id_sys_t *id_sys = id;

    vlc_input_decoder_Flush( id_sys->dec );
    p_sys->reanchor = true;
}

static int Control( sout_stream_t *p_stream, int i_query, va_list args )
{
    switch( i_query )
    {
        case SOUT_STREAM_ID_SPU_HIGHLIGHT:
        {
            sout_stream_id_sys_t *id = va_arg( args, void * );
            const vlc_spu_highlight_t *spu_hl = va_arg( args, const vlc_spu_highlight_t * );
            return vlc_input_decoder_SetSpuHighlight( id->dec, spu_hl );
        }

        case SOUT_STREAM_IS_SYNCHRONOUS:
            *va_arg( args, bool * ) = true;
            break;

        default:
           return VLC_EGENERIC;
    }
    (void) p_stream;
    return VLC_SUCCESS;
}

static void SetPCR( sout_stream_t *p_stream, vlc_tick_t pcr )
{
    sout_stream_sys_t *sys = p_stream->p_sys;

    if( sys->first_pcr_signaled && !sys->reanchor )
        return;

    sout_ClockMainSetFirstPcr( sys->main_clock, pcr );
    sys->first_pcr_signaled = true;
    sys->reanchor = false;

    /* the clock starts again: the renderer's delay as it is now */
    vlc_mutex_lock( &sys->lock );
    sys->applied_latency = VLC_TICK_INVALID;
    vlc_mutex_unlock( &sys->lock );
}

static const struct sout_stream_operations ops = {
    .add = Add,
    .del = Del,
    .send = Send,
    .flush = Flush,
    .control = Control,
    .set_pcr = SetPCR,
    .close = Close,
};

/*****************************************************************************
 * Open:
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    sout_stream_t *p_stream = (sout_stream_t*)p_this;
    sout_stream_sys_t *p_sys;

    p_sys = malloc( sizeof(sout_stream_sys_t) );
    if( p_sys == NULL )
        return VLC_ENOMEM;

    /* Under a player (its picture here too while it casts): the player's
     * resources, so the picture goes into the player's video window */
    input_resource_t *p_player_resource = var_InheritAddress( p_this, "player-resource" );
    p_sys->p_resource = p_player_resource != NULL ? input_resource_Hold( p_player_resource )
                                                  : input_resource_New( p_this );
    if( unlikely(p_sys->p_resource == NULL) )
    {
        free( p_sys );
        return VLC_ENOMEM;
    }

    p_sys->main_clock = sout_ClockMainCreate(p_stream);
    if( unlikely(p_sys->main_clock == NULL) )
    {
        input_resource_Release( p_sys->p_resource );
        free( p_sys );
        return VLC_ENOMEM;
    }

    config_ChainParse( p_stream, SOUT_CFG_PREFIX, ppsz_sout_options,
                       p_stream->p_cfg );

    p_sys->b_audio = var_GetBool( p_stream, SOUT_CFG_PREFIX "audio" );
    p_sys->b_video = var_GetBool( p_stream, SOUT_CFG_PREFIX "video" );
    p_sys->i_delay =
        VLC_TICK_FROM_MS( var_GetInteger(p_stream, SOUT_CFG_PREFIX "delay") );
    p_sys->first_pcr_signaled = false;
    p_sys->reanchor = false;
    p_sys->error = false;

    vlc_mutex_init( &p_sys->lock );
    vlc_vector_init( &p_sys->ids );
    p_sys->latency = 0;
    p_sys->applied_latency = VLC_TICK_INVALID;
    p_sys->applied_date = VLC_TICK_INVALID;
    p_sys->latency_jump = false;
    p_sys->paused = false;
    p_sys->libvlc = VLC_OBJECT( vlc_object_instance( p_stream ) );
    if( var_Type( p_sys->libvlc, "renderer-latency" ) )
    {
        p_sys->latency =
            VLC_TICK_FROM_MS( var_GetInteger( p_sys->libvlc, "renderer-latency" ) );
        var_AddCallback( p_sys->libvlc, "renderer-latency", LatencyChanged, p_sys );
        var_AddCallback( p_sys->libvlc, "renderer-paused", PausedChanged, p_sys );
        /* made while it is paused: paused too */
        vlc_value_t paused = { .b_bool = var_GetBool( p_sys->libvlc, "renderer-paused" ) };
        PausedChanged( p_sys->libvlc, "renderer-paused", paused, paused, p_sys );
    }
    else
        p_sys->libvlc = NULL;

    p_stream->ops = &ops;
    p_stream->p_sys = p_sys;
    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close:
 *****************************************************************************/
static void Close( sout_stream_t *p_stream )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    if( p_sys->libvlc != NULL )
    {
        var_DelCallback( p_sys->libvlc, "renderer-latency", LatencyChanged, p_sys );
        var_DelCallback( p_sys->libvlc, "renderer-paused", PausedChanged, p_sys );
    }
    vlc_vector_destroy( &p_sys->ids );
    input_resource_Release( p_sys->p_resource );
    sout_ClockMainDelete( p_sys->main_clock );
    free( p_sys );
}
