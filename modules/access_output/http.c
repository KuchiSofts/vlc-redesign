/*****************************************************************************
 * http.c
 *****************************************************************************
 * Copyright (C) 2001-2009 VLC authors and VideoLAN
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Remi Denis-Courmont
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

#include <stdint.h>

#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_plugin.h>
#include <vlc_sout.h>
#include <vlc_block.h>


#include <vlc_httpd.h>
#include <vlc_threads.h>
#include <vlc_tick.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Open ( vlc_object_t * );
static void Close( vlc_object_t * );

#define SOUT_CFG_PREFIX "sout-http-"

#define USER_TEXT N_("Username")
#define USER_LONGTEXT N_("Username that will be " \
                         "requested to access the stream." )
#define PASS_TEXT N_("Password")
#define PASS_LONGTEXT N_("Password that will be " \
                         "requested to access the stream." )
#define MIME_TEXT N_("Mime")
#define MIME_LONGTEXT N_("MIME returned by the server (autodetected " \
                        "if not specified)." )
#define METACUBE_TEXT N_("Metacube")
#define METACUBE_LONGTEXT N_("Use the Metacube protocol. Needed for streaming " \
                             "to the Cubemap reflector.")
#define DLNA_TEXT N_("DLNA content features")
#define DLNA_LONGTEXT N_("Answer with the headers of a DLNA media server, " \
                         "with these content features: a TV reading the " \
                         "stream may refuse it without them.")


vlc_module_begin ()
    set_description( N_("HTTP stream output") )
    set_capability( "sout access", 0 )
    set_shortname( "HTTP" )
    add_shortcut( "http", "https", "mmsh" )
    set_subcategory( SUBCAT_SOUT_ACO )
    add_string( SOUT_CFG_PREFIX "user", "",
                USER_TEXT, USER_LONGTEXT )
    add_password(SOUT_CFG_PREFIX "pwd", "", PASS_TEXT, PASS_LONGTEXT)
    add_string( SOUT_CFG_PREFIX "mime", "",
                MIME_TEXT, MIME_LONGTEXT )
    add_bool( SOUT_CFG_PREFIX "metacube", false,
              METACUBE_TEXT, METACUBE_LONGTEXT )
    add_string( SOUT_CFG_PREFIX "dlna", NULL, DLNA_TEXT, DLNA_LONGTEXT )
        change_private()
    set_callbacks( Open, Close )
vlc_module_end ()


/*****************************************************************************
 * Exported prototypes
 *****************************************************************************/
static const char *const ppsz_sout_options[] = {
    "user", "pwd", "mime", "metacube", "dlna", NULL
};

static ssize_t Write( sout_access_out_t *, block_t * );
static int Control( sout_access_out_t *, int, va_list );

typedef struct
{
    /* host */
    httpd_host_t        *p_httpd_host;

    /* stream */
    httpd_stream_t      *p_httpd_stream;

    /* gather header from stream */
    int                 i_header_allocated;
    int                 i_header_size;
    uint8_t             *p_header;
    bool                b_header_complete;
    bool                b_metacube;
    bool                b_has_keyframes;

    /* a DLNA MPEG-TS stream: padding while it waits (Stuff) */
    bool                b_stuffing;
    vlc_thread_t        stuff_thread;
    vlc_mutex_t         lock;
    vlc_cond_t          wait;
    bool                b_stop;
    bool                b_started;
    vlc_tick_t          i_last_write;
    size_t              i_stuffed;
} sout_access_out_sys_t;

/* A paused TV goes on reading the stream, until it has enough of it
 * ahead, and gives up when nothing comes for about 10 seconds: a Samsung
 * TV paused with less than about 5 seconds of the stream stops with an
 * error (measured on a 2017 UE55MU8000; it reads null packets at once,
 * and stopped 10 s after they did). While the stream waits (a pause), it
 * gets a trickle of null MPEG-TS packets (PID 0x1fff, which every reader
 * drops). At most STUFF_MAX each time: the stream's buffer (5 MB) keeps
 * them for a TV that no longer reads (at this rate, for minutes). */
#define STUFF_IDLE      VLC_TICK_FROM_MS(500)
#define STUFF_PERIOD    VLC_TICK_FROM_MS(100)
#define STUFF_PACKETS   8                   /* 1.5 KB each 100 ms */
#define STUFF_MAX       (64 * 1024 * 1024)

static void *Stuff( void *data )
{
    sout_access_out_t *p_access = data;
    sout_access_out_sys_t *p_sys = p_access->p_sys;

    vlc_thread_set_name( "vlc-http-stuff" );
    vlc_mutex_lock( &p_sys->lock );
    while( !p_sys->b_stop )
    {
        const vlc_tick_t now = vlc_tick_now();
        if( p_sys->b_started && now - p_sys->i_last_write >= STUFF_IDLE
         && p_sys->i_stuffed < STUFF_MAX )
        {
            block_t *p_block = block_Alloc( STUFF_PACKETS * 188 );
            if( p_block != NULL )
            {
                for( int i = 0; i < STUFF_PACKETS; i++ )
                {
                    uint8_t *p = &p_block->p_buffer[188 * i];
                    p[0] = 0x47;
                    p[1] = 0x1f;
                    p[2] = 0xff;
                    p[3] = 0x10;
                    memset( &p[4], 0xff, 184 );
                }
                httpd_StreamSend( p_sys->p_httpd_stream, p_block );
                p_sys->i_stuffed += p_block->i_buffer;
                block_Release( p_block );
            }
        }
        vlc_cond_timedwait( &p_sys->wait, &p_sys->lock, now + STUFF_PERIOD );
    }
    vlc_mutex_unlock( &p_sys->lock );
    return NULL;
}

/* Definitions for the Metacube2 protocol, used to communicate with Cubemap. */

static const uint8_t METACUBE2_SYNC[8] = {'c', 'u', 'b', 'e', '!', 'm', 'a', 'p'};
#define METACUBE_FLAGS_HEADER 0x1
#define METACUBE_FLAGS_NOT_SUITABLE_FOR_STREAM_START 0x2

struct metacube2_block_header
{
    char sync[8];    /* METACUBE2_SYNC */
    uint32_t size;   /* Network byte order. Does not include header. */
    uint16_t flags;  /* Network byte order. METACUBE_FLAGS_*. */
    uint16_t csum;   /* Network byte order. CRC16 of size and flags. */
};

/*
 * Implementation of Metacube2 utility functions. Taken from the Cubemap
 * distribution and then relicensed by the author to LGPL2.1+ for inclusion
 * into VLC.
 */

/*
 * https://www.ece.cmu.edu/~koopman/pubs/KoopmanCRCWebinar9May2012.pdf
 * recommends this for messages as short as ours (see table at page 34).
 */
#define METACUBE2_CRC_POLYNOMIAL 0x8FDB

/* Semi-random starting value to make sure all-zero won't pass. */
#define METACUBE2_CRC_START 0x1234

/* This code is based on code generated by pycrc. */
static uint16_t metacube2_compute_crc(const struct metacube2_block_header *hdr)
{
    static const int data_len = sizeof(hdr->size) + sizeof(hdr->flags);
    const uint8_t *data = (uint8_t *)&hdr->size;
    uint16_t crc = METACUBE2_CRC_START;

    for (int i = 0; i < data_len; ++i) {
        uint8_t c = data[i];
        for (int j = 0; j < 8; j++) {
            int bit = crc & 0x8000;
            crc = (crc << 1) | ((c >> (7 - j)) & 0x01);
            if (bit) {
                crc ^= METACUBE2_CRC_POLYNOMIAL;
            }
        }
    }

    /* Finalize. */
    for (int i = 0; i < 16; i++) {
        int bit = crc & 0x8000;
        crc = crc << 1;
        if (bit) {
            crc ^= METACUBE2_CRC_POLYNOMIAL;
        }
    }

    return crc;
}

/*****************************************************************************
 * Open: open the file
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    sout_access_out_t       *p_access = (sout_access_out_t*)p_this;
    sout_access_out_sys_t   *p_sys;

    char                *psz_user;
    char                *psz_pwd;
    char                *psz_mime;

    if( !( p_sys = p_access->p_sys =
                malloc( sizeof( sout_access_out_sys_t ) ) ) )
        return VLC_ENOMEM ;

    config_ChainParse( p_access, SOUT_CFG_PREFIX, ppsz_sout_options, p_access->p_cfg );

    const char *path = p_access->psz_path;
    path += strcspn( path, "/" );
    if( path > p_access->psz_path )
    {
        const char *port = strrchr( p_access->psz_path, ':' );
        if( port != NULL && strchr( port, ']' ) != NULL )
            port = NULL; /* IPv6 numeral */
        if( port != p_access->psz_path )
        {
            int len = (port ? port : path) - p_access->psz_path;
            msg_Warn( p_access, "\"%.*s\" HTTP host might be ignored in "
                      "multiple-host configurations, use at your own risks.",
                      len, p_access->psz_path );
            msg_Info( p_access, "Consider passing --http-host=IP on the "
                                "command line instead." );

            char host[len + 1];
            strncpy( host, p_access->psz_path, len );
            host[len] = '\0';

            var_Create( p_access, "http-host", VLC_VAR_STRING );
            var_SetString( p_access, "http-host", host );
        }
        if( port != NULL )
        {
            /* int len = path - ++port;
            msg_Info( p_access, "Consider passing --%s-port=%.*s on the "
                                "command line instead.",
                      strcasecmp( p_access->psz_access, "https" )
                      ? "http" : "https", len, port ); */
            port++;

            int bind_port = atoi( port );
            if( bind_port > 0 )
            {
                const char *var = strcasecmp( p_access->psz_access, "https" )
                                  ? "http-port" : "https-port";
                var_Create( p_access, var, VLC_VAR_INTEGER );
                var_SetInteger( p_access, var, bind_port );
            }
        }
    }
    if( !*path )
        path = "/";

    /* TLS support */
    if( p_access->psz_access && !strcmp( p_access->psz_access, "https" ) )
        p_sys->p_httpd_host = vlc_https_HostNew( VLC_OBJECT(p_access) );
    else
        p_sys->p_httpd_host = vlc_http_HostNew( VLC_OBJECT(p_access) );

    if( p_sys->p_httpd_host == NULL )
    {
        msg_Err( p_access, "cannot start HTTP server" );
        free( p_sys );
        return VLC_EGENERIC;
    }

    psz_user = var_GetNonEmptyString( p_access, SOUT_CFG_PREFIX "user" );
    psz_pwd = var_GetNonEmptyString( p_access, SOUT_CFG_PREFIX "pwd" );
    if( p_access->psz_access && !strcmp( p_access->psz_access, "mmsh" ) )
    {
        psz_mime = strdup( "video/x-ms-asf-stream" );
    }
    else
    {
        psz_mime = var_GetNonEmptyString( p_access, SOUT_CFG_PREFIX "mime" );
    }

    p_sys->b_metacube = var_GetBool( p_access, SOUT_CFG_PREFIX "metacube" );
    p_sys->b_has_keyframes = false;
    p_sys->b_stuffing = false;
    const bool b_ts = psz_mime != NULL && ( !strcmp( psz_mime, "video/mpeg" )
                                         || !strcmp( psz_mime, "video/mp2t" ) );

    p_sys->p_httpd_stream =
        httpd_StreamNew( p_sys->p_httpd_host, path, psz_mime,
                         psz_user, psz_pwd );
    free( psz_user );
    free( psz_pwd );
    free( psz_mime );

    if( p_sys->p_httpd_stream == NULL )
    {
        msg_Err( p_access, "cannot add stream %s", path );
        httpd_HostDelete( p_sys->p_httpd_host );

        free( p_sys );
        return VLC_EGENERIC;
    }

    httpd_header headers[3];
    size_t i_headers = 0;
    if( p_sys->b_metacube )
        headers[i_headers++] = (httpd_header){ (char *)"Content-encoding",
                                               (char *)"metacube" };
    /* a DLNA renderer (a TV) reading the stream wants to know it is one,
     * live (no seeking) */
    char *psz_dlna = var_GetNonEmptyString( p_access, SOUT_CFG_PREFIX "dlna" );
    if( psz_dlna != NULL )
    {
        headers[i_headers++] = (httpd_header){ (char *)"transferMode.dlna.org",
                                               (char *)"Streaming" };
        headers[i_headers++] = (httpd_header){ (char *)"contentFeatures.dlna.org",
                                               psz_dlna };
        /* it connects once told to play the stream: from its start */
        httpd_StreamFromStart( p_sys->p_httpd_stream );
    }
    if( i_headers > 0 )
    {
        int err = httpd_StreamSetHTTPHeaders( p_sys->p_httpd_stream, headers,
                                              i_headers );
        free( psz_dlna );
        if( err != VLC_SUCCESS )
        {
            free( p_sys );
            return err;
        }
    }

    p_sys->i_header_allocated = 1024;
    p_sys->i_header_size      = 0;
    p_sys->p_header           = xmalloc( p_sys->i_header_allocated );
    p_sys->b_header_complete  = false;

    p_access->pf_write       = Write;
    p_access->pf_control     = Control;

    /* a TV's MPEG-TS stream: padded while it waits */
    char *psz_tv = b_ts ? var_GetNonEmptyString( p_access, SOUT_CFG_PREFIX "dlna" ) : NULL;
    if( psz_tv != NULL )
    {
        free( psz_tv );
        vlc_mutex_init( &p_sys->lock );
        vlc_cond_init( &p_sys->wait );
        p_sys->b_stop = false;
        p_sys->b_started = false;
        p_sys->i_last_write = VLC_TICK_INVALID;
        p_sys->i_stuffed = 0;
        p_sys->b_stuffing = vlc_clone( &p_sys->stuff_thread, Stuff, p_access ) == 0;
    }

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close: close the target
 *****************************************************************************/
static void Close( vlc_object_t * p_this )
{
    sout_access_out_t       *p_access = (sout_access_out_t*)p_this;
    sout_access_out_sys_t   *p_sys = p_access->p_sys;

    if( p_sys->b_stuffing )
    {
        vlc_mutex_lock( &p_sys->lock );
        p_sys->b_stop = true;
        vlc_cond_signal( &p_sys->wait );
        vlc_mutex_unlock( &p_sys->lock );
        vlc_join( p_sys->stuff_thread, NULL );
    }
    httpd_StreamDelete( p_sys->p_httpd_stream );
    httpd_HostDelete( p_sys->p_httpd_host );

    free( p_sys->p_header );

    msg_Dbg( p_access, "Close" );

    free( p_sys );
}

static int Control( sout_access_out_t *p_access, int i_query, va_list args )
{
    (void)p_access;

    switch( i_query )
    {
        case ACCESS_OUT_CONTROLS_PACE:
            *va_arg( args, bool * ) = false;
            break;

        default:
            return VLC_EGENERIC;
    }
    return VLC_SUCCESS;
}

/*****************************************************************************
 * Write:
 *****************************************************************************/
static ssize_t Write( sout_access_out_t *p_access, block_t *p_buffer )
{
    sout_access_out_sys_t *p_sys = p_access->p_sys;
    int i_err = 0;
    int i_len = 0;

    /* (no padding between the blocks written) */
    if( p_sys->b_stuffing )
        vlc_mutex_lock( &p_sys->lock );

    while( p_buffer )
    {
        block_t *p_next;

        if( p_buffer->i_flags & BLOCK_FLAG_HEADER )
        {
            /* gather header */
            if( p_sys->b_header_complete )
            {
                /* free previously gathered header */
                p_sys->i_header_size = 0;
                p_sys->b_header_complete = false;
            }
            if( (int)(p_buffer->i_buffer + p_sys->i_header_size) >
                p_sys->i_header_allocated )
            {
                p_sys->i_header_allocated =
                    p_buffer->i_buffer + p_sys->i_header_size + 1024;
                p_sys->p_header = xrealloc( p_sys->p_header,
                                                  p_sys->i_header_allocated );
            }
            memcpy( &p_sys->p_header[p_sys->i_header_size],
                    p_buffer->p_buffer,
                    p_buffer->i_buffer );
            p_sys->i_header_size += p_buffer->i_buffer;
        }
        else if( !p_sys->b_header_complete )
        {
            p_sys->b_header_complete = true;

            if ( p_sys->b_metacube )
            {
                struct metacube2_block_header hdr;
                memcpy( hdr.sync, METACUBE2_SYNC, sizeof( METACUBE2_SYNC ) );
                hdr.size = hton32( p_sys->i_header_size );
                hdr.flags = hton16( METACUBE_FLAGS_HEADER );
                hdr.csum = hton16( metacube2_compute_crc( &hdr ) );

                int i_header_size = p_sys->i_header_size + sizeof( hdr );
                block_t *p_hdr_block = block_Alloc( i_header_size );
                if( p_hdr_block == NULL ) {
                    block_ChainRelease( p_buffer );
                    if( p_sys->b_stuffing )
                        vlc_mutex_unlock( &p_sys->lock );
                    return VLC_ENOMEM;
                }
                p_hdr_block->i_flags = 0;
                memcpy( p_hdr_block->p_buffer, &hdr, sizeof( hdr ) );
                memcpy( p_hdr_block->p_buffer + sizeof( hdr ), p_sys->p_header, p_sys->i_header_size );

                /* send the combined header here instead of sending them as regular
                 * data, so that we get them as a single Metacube header block */
                httpd_StreamHeader( p_sys->p_httpd_stream, p_hdr_block->p_buffer, p_hdr_block->i_buffer );
                httpd_StreamSend( p_sys->p_httpd_stream, p_hdr_block );

                block_Release( p_hdr_block );
            }
            else
            {
                httpd_StreamHeader( p_sys->p_httpd_stream, p_sys->p_header,
                                    p_sys->i_header_size );
            }
        }

        i_len += p_buffer->i_buffer;

        if( p_buffer->i_flags & BLOCK_FLAG_TYPE_I )
        {
            p_sys->b_has_keyframes = true;
        }

        p_next = p_buffer->p_next;

        if( p_sys->b_metacube )
        {
            /* header data is combined into one packet and sent earlier */
            if( p_buffer->i_flags & BLOCK_FLAG_HEADER ) {
                block_Release( p_buffer );
                p_buffer = p_next;
                continue;
            }

            /* prepend Metacube header */
            struct metacube2_block_header hdr;
            memcpy( hdr.sync, METACUBE2_SYNC, sizeof( METACUBE2_SYNC ) );
            hdr.size = hton32( p_buffer->i_buffer );
            hdr.flags = hton16( 0 );
            if( p_buffer->i_flags & BLOCK_FLAG_HEADER )
                hdr.flags |= hton16( METACUBE_FLAGS_HEADER );
            if( p_sys->b_has_keyframes && !( p_buffer->i_flags & BLOCK_FLAG_TYPE_I ) )
                hdr.flags |= hton16( METACUBE_FLAGS_NOT_SUITABLE_FOR_STREAM_START );
            hdr.csum = hton16( metacube2_compute_crc( &hdr ) );

            p_buffer = block_Realloc( p_buffer, sizeof( hdr ), p_buffer->i_buffer );
            if( p_buffer == NULL ) {
                block_ChainRelease( p_next );
                if( p_sys->b_stuffing )
                    vlc_mutex_unlock( &p_sys->lock );
                return VLC_ENOMEM;
            }
            memcpy( p_buffer->p_buffer, &hdr, sizeof( hdr ) );
        }

        /* send data */
        i_err = httpd_StreamSend( p_sys->p_httpd_stream, p_buffer );

        block_Release( p_buffer );
        p_buffer = p_next;

        if( i_err < 0 )
        {
            break;
        }
    }

    if( i_err < 0 )
    {
        block_ChainRelease( p_buffer );
    }

    if( p_sys->b_stuffing )
    {
        p_sys->b_started = true;
        p_sys->i_last_write = vlc_tick_now();
        p_sys->i_stuffed = 0;
        vlc_mutex_unlock( &p_sys->lock );
    }

    return( i_err < 0 ? VLC_EGENERIC : i_len );
}
