/*****************************************************************************
 * dlna.cpp : DLNA/UPNP (renderer) sout module
 *****************************************************************************
 * Copyright © 2018 VLC authors and VideoLAN
 *
 * Authors: Shaleen Jain <shaleen@jain.sh>
 *          William Ung <william1.ung@epitech.eu>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "dlna.hpp"

#include <algorithm>
#include <atomic>
#include <deque>
#include <vector>
#include <string>
#include <sstream>

#include <vlc_configuration.h>
#include <vlc_cxx_helpers.hpp>
#include <vlc_dialog.h>
#include <vlc_rand.h>
#include <vlc_sout.h>
#include <vlc_block.h>

static const char* AV_TRANSPORT_SERVICE_TYPE = "urn:schemas-upnp-org:service:AVTransport:1";
static const char* CONNECTION_MANAGER_SERVICE_TYPE = "urn:schemas-upnp-org:service:ConnectionManager:1";
static const char* RENDERING_CONTROL_SERVICE_TYPE = "urn:schemas-upnp-org:service:RenderingControl:1";

static const char *const ppsz_sout_options[] = {
    "ip", "port", "http-port", "video", "base_url", "url", nullptr
};

namespace DLNA
{

struct sout_stream_id_sys_t
{
    es_format_t           fmt;
    const char            *es_id;
    sout_stream_id_sys_t  *p_sub_id;
    /* the same track in the picture here (the display output) */
    void                  *p_display_id;
    /* flushed (a seek): what comes next is the new part */
    bool                  fresh;
};

/* What is sent goes at the pace of the media, this much ahead (a renderer
 * starting gets that much at once); what is held past FEED_MAX_HOLD goes
 * all the same (the media goes faster); when what comes is later than
 * FEED_SLIP, the input waited: the pace starts again from there. */
#define FEED_LEAD       VLC_TICK_FROM_SEC(1)
#define FEED_MAX_HOLD   VLC_TICK_FROM_SEC(5)
#define FEED_SLIP       VLC_TICK_FROM_SEC(1)
/* The input reads a subtitle once it read the pictures of its time, a little
 * after them, and the conversion blends the subtitles there are into each
 * picture as it goes: with subtitles, what comes waits that much at least,
 * for their subtitle to be there before them. */
#define FEED_SUBTITLE   VLC_TICK_FROM_MS(250)

/* How the renderer does, for the interface ("renderer-state" of libvlc:
 * "connecting", "playing" or "failed:<why>", then "|" and the device's url),
 * and why it failed: it did not answer ("unreachable"),
 * could not read the stream from this computer ("unreadable": a firewall,
 * most of the time), refused it ("refused"), or stopped by itself
 * ("stopped": its remote, or it turned off) */
enum
{
    RENDERER_IDLE,
    RENDERER_CONNECTING,
    RENDERER_PLAYING,
    RENDERER_FAILED,
};
/* the network's errors (libupnp's): the device does not answer */
static bool NetworkError( int error )
{
    return error <= UPNP_E_NETWORK_ERROR || error == UPNP_E_BAD_RESPONSE
        || error == UPNP_E_BAD_HTTPMSG;
}

static vlc_tick_t BlockTs( const block_t *block )
{
    return block->i_dts != VLC_TICK_INVALID ? block->i_dts : block->i_pts;
}

/* A picture a stream can start with: said so (by the demuxer or the
 * packetizer), or an IDR / IRAP picture of H.264 / HEVC, whose NAL units
 * come after start codes or after their size (4 bytes, as in MP4) */
static bool IsKeyframe( const es_format_t *fmt, const block_t *block )
{
    if ( block->i_flags & BLOCK_FLAG_TYPE_I )
        return true;
    const bool h264 = fmt->i_codec == VLC_CODEC_H264;
    if ( !h264 && fmt->i_codec != VLC_CODEC_HEVC )
        return false;
    const auto irap = [h264]( uint8_t header ) {
        const unsigned type = h264 ? header & 0x1F : ( header >> 1 ) & 0x3F;
        return h264 ? type == 5 : type >= 16 && type <= 21;
    };
    const uint8_t *p = block->p_buffer;
    const size_t size = block->i_buffer;
    if ( size >= 4 && p[0] == 0 && p[1] == 0 && ( p[2] == 1 || ( p[2] == 0 && p[3] == 1 ) ) )
    {
        for ( size_t i = 0; i + 3 < size; ++i )
            if ( p[i] == 0 && p[i + 1] == 0 && p[i + 2] == 1 )
            {
                if ( irap( p[i + 3] ) )
                    return true;
                i += 2;
            }
        return false;
    }
    for ( size_t i = 0; i + 5 <= size; )
    {
        const size_t nal = GetDWBE( &p[i] );
        if ( nal == 0 || nal > size - i - 4 )
            return false;
        if ( irap( p[i + 4] ) )
            return true;
        i += 4 + nal;
    }
    return false;
}

static void BlockShift( block_t *block, vlc_tick_t offset )
{
    if ( block->i_dts != VLC_TICK_INVALID )
        block->i_dts += offset;
    if ( block->i_pts != VLC_TICK_INVALID )
        block->i_pts += offset;
}

struct sout_stream_sys_t
{
    sout_stream_sys_t(int http_port, bool supports_video)
        : p_out( nullptr )
        , es_changed( true )
        , b_supports_video( supports_video )
        , perf_warning_shown( false )
        , venc_opt_idx ( -1 )
        , http_port( http_port )
        , announce_at( VLC_TICK_INVALID )
    {
    }

    std::shared_ptr<MediaRenderer> renderer;
    UpnpInstanceWrapper *p_upnp;

    ProtocolPtr canDecodeAudio( vlc_fourcc_t i_codec ) const;
    ProtocolPtr canDecodeVideo( vlc_fourcc_t audio_codec,
                         vlc_fourcc_t video_codec ) const;
    bool startSoutChain( sout_stream_t* p_stream,
                         const std::vector<sout_stream_id_sys_t*> &new_streams,
                         const std::string &sout );
    void stopSoutChain( sout_stream_t* p_stream, bool keep = false );
    int UpdateOutput( sout_stream_t *p_stream );
    void ControlLoop( sout_stream_t *p_stream );
    void Hold( sout_stream_t *p_stream, sout_stream_id_sys_t *id, block_t *block );
    void Retime( sout_stream_t *p_stream, bool force );
    void Feed( sout_stream_t *p_stream, bool all );
    void Release( sout_stream_t *p_stream, sout_stream_id_sys_t *id, block_t *block );
    void Anchor( sout_stream_t *p_stream, vlc_tick_t date, vlc_tick_t ts );
    void UpdateDisplay( sout_stream_t *p_stream );
    void StopDisplay();
    void ResetTimeline();
    vlc_tick_t BaseTs( vlc_tick_t date ) const;
    void Measure( sout_stream_t *p_stream, vlc_tick_t pos, vlc_tick_t resolution,
                  vlc_tick_t sent, vlc_tick_t received );
    void PublishTime( vlc_tick_t now );
    void Report( int status, const char *error );
    void Fail( sout_stream_t *p_stream, const char *error, int code );
    void CheckTransport( sout_stream_t *p_stream, const std::string &state, int error,
                         vlc_tick_t now );

    sout_stream_t                       *p_out;
    bool                                es_changed;
    /* when it did: the first chain waits for the tracks to stay the same a
     * moment (the input adds them one at a time); what comes is held */
    vlc_tick_t                          es_changed_at = VLC_TICK_INVALID;
    bool                                b_supports_video;
    bool                                perf_warning_shown;
    int                                 venc_opt_idx;
    int                                 http_port;
    std::vector<sout_stream_id_sys_t*>  streams;
    std::vector<sout_stream_id_sys_t*>  out_streams;
    std::vector<protocol_info_t>        device_protocols;
    /* The stream the renderer is told to play, once its tracks stopped
     * changing (see Announce) */
    vlc_tick_t                          announce_at;
    std::string                         announce_uri;
    ProtocolPtr                         announce_protocol;
    /* the running chain converts the video, blending the subtitles it gets:
     * another subtitle goes in there, the renderer goes on (Add) */
    bool                                chain_soverlay = false;
    /* it starts with a picture the renderer can show (a keyframe): its
     * position in the stream counts from there */
    bool                                chain_started = false;
    vlc_tick_t                          chain_wait = VLC_TICK_INVALID;

    /* One stream through seeks. The renderer plays what it has received
     * before what comes next: a new stream at each seek would make it start
     * again (a TV loads for seconds, some show an error). So what comes
     * after a seek (Flush) is timed to follow what was sent, and what the
     * input reads ahead is held, to go at the pace of the media (Feed): at a
     * seek, what was held is dropped, and the renderer gets the new part
     * right after the last it has. The times sent are the input's plus
     * ts_offset. */
    struct held_block
    {
        sout_stream_id_sys_t *id;
        block_t              *block;
        bool                  timed;    /* false: after a seek, until Retime */
        vlc_tick_t            arrival;
    };
    std::deque<held_block>              held;
    /* What is held goes at its time even when nothing comes (at the end of
     * a media the input stops sending first): a thread feeds too
     * (FeedThread). feed_lock guards the chains and what is held. */
    vlc::threads::mutex                 feed_lock;
    vlc::threads::condition_variable    feed_wait;
    vlc_thread_t                        feed_thread;
    bool                                feed_running = false;
    bool                                feed_stop = false;
    /* since when what comes is late (the input waited, if it lasts) */
    vlc_tick_t                          late_since = VLC_TICK_INVALID;
    vlc_tick_t                          ts_offset = 0;
    bool                                retime = false;
    vlc_tick_t                          retime_since = VLC_TICK_INVALID;
    vlc_tick_t                          sent_end = VLC_TICK_INVALID;

    /* the picture here too ("renderer-display" of libvlc): a display output
     * gets what is sent, and shows it when the renderer does */
    /* the chain of the media that ended: a TV stops at once when its stream
     * is cut, so it stays (no more in it) until the TV showed what it has,
     * or another plays there (ended_done, ctl_lock) */
    sout_stream_t                       *p_ended = nullptr;
    bool                                ended_done = false;

    sout_stream_t                       *p_display = nullptr;
    bool                                display_started = false;
    bool                                display_keyframe = false;
    vlc_tick_t                          display_wait = VLC_TICK_INVALID;
    /* a picture went already: the next ones may not start a stream */
    bool                                video_sent = false;

    /* The renderer is driven from a thread of its own (ControlLoop): its
     * actions are network round trips, which the media must not wait for.
     * ctl_lock guards the announce and everything below. */
    vlc_thread_t                        ctl_thread;
    vlc::threads::mutex                 ctl_lock;
    vlc::threads::condition_variable    ctl_wait;
    bool                                ctl_running = false;
    bool                                ctl_stop = false;
    /* the end of the media: the renderer is stopped once it has shown what
     * it has (it is that much behind), the next one plays after that */
    vlc_tick_t                          stop_at = VLC_TICK_INVALID;
    /* stopped by the user: now (until another plays) */
    bool                                stop_now = false;
    /* what the interface asks (the "renderer-paused", "renderer-volume" and
     * "renderer-muted" variables of libvlc), and what was done */
    bool                                want_paused = false;
    bool                                sent_paused = false;
    int                                 want_volume = -1;
    int                                 sent_volume = -1;
    int                                 want_muted = -1;
    int                                 sent_muted = -1;
    bool                                playing = false;
    bool                                display_wanted = false;
    /* The pace: the stream's time anchor_ts goes at anchor_date, later by
     * the pauses since (paused_total, and from pause_date while paused).
     * The display here starts its clock on it. */
    vlc_tick_t                          anchor_date = VLC_TICK_INVALID;
    vlc_tick_t                          anchor_ts = VLC_TICK_INVALID;
    vlc_tick_t                          anchor_paused = 0;
    vlc_tick_t                          pause_date = VLC_TICK_INVALID;
    vlc_tick_t                          paused_total = 0;
    /* where the renderer's position is 0: the stream's time of the first
     * picture of the chain; the parts of the stream since the seeks
     * (where each starts, and its offset from the input's times) */
    vlc_tick_t                          tl_first = VLC_TICK_INVALID;
    std::deque<std::pair<vlc_tick_t, vlc_tick_t>> segments;
    /* how much later than the pace the renderer shows the stream (Measure):
     * the origin of its position is between origin_lo and origin_hi */
    vlc_tick_t                          latency = VLC_TICK_INVALID;
    vlc_tick_t                          measure_at = VLC_TICK_INVALID;
    vlc_tick_t                          measure_until = VLC_TICK_INVALID;
    vlc_tick_t                          origin_lo = VLC_TICK_INVALID;
    vlc_tick_t                          origin_hi = VLC_TICK_INVALID;
    unsigned                            polls = 0;
    vlc_tick_t                          time_at = VLC_TICK_INVALID;
    /* Whether it plays what it was told (CheckTransport): what it says it
     * does, asked from when it is told to play (started_at) until it plays,
     * then now and then. A device that stops answering is not asked any more
     * (unreachable): a network round trip waits up to 30 s for it. */
    int                                 status = RENDERER_IDLE;
    vlc_tick_t                          started_at = VLC_TICK_INVALID;
    vlc_tick_t                          check_at = VLC_TICK_INVALID;
    bool                                seen_playing = false;
    unsigned                            stopped_checks = 0;
    unsigned                            failed_checks = 0;
    bool                                unreachable = false;
    /* The stream copied as it was and the device did not read it (a profile,
     * a level or a container it does not take): converted once, to the
     * profile every renderer plays, before saying it failed. The request
     * comes from the control thread (ctl_lock), the chain is made again on
     * the input's (Send). */
    std::atomic<bool>                   fallback_request { false };
    bool                                force_transcode = false;
    /* (ctl_lock) the running stream copies a track as it is: converting it
     * may help */
    bool                                copies_as_is = false;
    vlc_object_t                        *libvlc = nullptr;

private:
    std::string GetAcodecOption( sout_stream_t *, vlc_fourcc_t *, const audio_format_t *, int );

};

static char *getServerIPAddress() {
    const char *ip = UpnpGetServerIpAddress();
    if (ip == nullptr) {
        ip = UpnpGetServerIp6Address();
    }
    return ip != nullptr ? strdup(ip) : nullptr;
}

std::string dlna_write_protocol_info (const protocol_info_t info)
{
    const dlna_org_flags_t flags = DLNA_ORG_FLAG_STREAMING_TRANSFER_MODE |
                                   DLNA_ORG_FLAG_BACKGROUND_TRANSFERT_MODE |
                                   DLNA_ORG_FLAG_CONNECTION_STALL |
                                   DLNA_ORG_FLAG_DLNA_V15;
  /* a live stream: no byte ranges (a TV told otherwise probes the stream at
   * other offsets, and gives up) */
  return dlna_write_protocol_info(info, flags, DLNA_ORG_OPERATION_NONE);
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
        elems.push_back(std::move(item));
    }
    return elems;
}

ProtocolPtr sout_stream_sys_t::canDecodeAudio(vlc_fourcc_t audio_codec) const
{
    for (protocol_info_t protocol : device_protocols) {
        if (protocol.profile.media == DLNA_CLASS_AUDIO
                && protocol.profile.audio_codec == audio_codec)
        {
            return std::unique_ptr<protocol_info_t>(new protocol_info_t(protocol));
        }
    }
    return nullptr;
}

ProtocolPtr sout_stream_sys_t::canDecodeVideo(vlc_fourcc_t audio_codec,
                vlc_fourcc_t video_codec) const
{
    for (protocol_info_t protocol : device_protocols) {
        if (protocol.profile.media == DLNA_CLASS_AV
                && protocol.profile.audio_codec == audio_codec
                && protocol.profile.video_codec == video_codec)
        {
            return std::unique_ptr<protocol_info_t>(new protocol_info_t(protocol));
        }
    }
    return nullptr;
}

bool sout_stream_sys_t::startSoutChain(sout_stream_t *p_stream,
                                       const std::vector<sout_stream_id_sys_t*> &new_streams,
                                       const std::string &sout)
{
    stopSoutChain(p_stream);
    msg_Dbg( p_stream, "Creating chain %s", sout.c_str() );
    out_streams = new_streams;

    p_out = sout_StreamChainNew(VLC_OBJECT(p_stream), sout.c_str(), nullptr);
    if (p_out == nullptr) {
        msg_Err(p_stream, "could not create sout chain:%s", sout.c_str());
        out_streams.clear();
        return false;
    }

    /* check the streams we can actually add */
    for (std::vector<sout_stream_id_sys_t*>::iterator it = out_streams.begin();
            it != out_streams.end(); )
    {
        sout_stream_id_sys_t *p_sys_id = *it;
        p_sys_id->p_sub_id = static_cast<sout_stream_id_sys_t *>(
            sout_StreamIdAdd( p_out, &p_sys_id->fmt, p_sys_id->es_id ) );
        if ( p_sys_id->p_sub_id == nullptr )
        {
            msg_Err( p_stream, "can't handle %4.4s stream",
                    (char *)&p_sys_id->fmt.i_codec );
            es_format_Clean( &p_sys_id->fmt );
            it = out_streams.erase( it );
        }
        else
            ++it;
    }

    if (out_streams.empty())
    {
        stopSoutChain( p_stream );
        return false;
    }

    return true;
}

void sout_stream_sys_t::stopSoutChain(sout_stream_t *p_stream, bool keep)
{
    msg_Dbg( p_stream, "Destroying dlna sout chain");
    {
        vlc::threads::mutex_locker locker( ctl_lock );
        announce_uri.clear();
        tl_first = VLC_TICK_INVALID;
        measure_at = VLC_TICK_INVALID;
        polls = 0;
    }
    chain_soverlay = false;
    chain_started = false;
    chain_wait = VLC_TICK_INVALID;

    for ( size_t i = 0; i < out_streams.size(); i++ )
    {
        sout_StreamIdDel( p_out, out_streams[i]->p_sub_id );
        out_streams[i]->p_sub_id = nullptr;
    }
    out_streams.clear();
    if ( keep )
    {
        if ( p_ended != nullptr )
            sout_StreamChainDelete( p_ended, nullptr );
        p_ended = p_out;
        vlc::threads::mutex_locker locker( ctl_lock );
        ended_done = false;
    }
    else
        sout_StreamChainDelete( p_out, nullptr );
    p_out = nullptr;
}

std::string
sout_stream_sys_t::GetAcodecOption( sout_stream_t *p_stream, vlc_fourcc_t *p_codec_audio,
                                    const audio_format_t *p_aud, int i_quality )
{
    VLC_UNUSED(i_quality);
    std::stringstream ssout;

    msg_Dbg( p_stream, "Converting audio to %.4s", (const char*)p_codec_audio );

    ssout << "acodec=";
    char fourcc[5];
    vlc_fourcc_to_char( *p_codec_audio, fourcc );
    fourcc[4] = '\0';
    ssout << fourcc << ',';

    if ( p_aud && p_aud->i_rate > 48000 )
        ssout << "samplerate=48000,";
    ssout << "aenc=avcodec{codec=aac},";
    return ssout.str();
}

int sout_stream_sys_t::UpdateOutput( sout_stream_t *p_stream )
{
    assert( p_stream->p_sys == this );

    if ( !es_changed )
        return VLC_SUCCESS;

    es_changed = false;

    bool canRemux = true;
    // To keep track of which stream needs transcoding if at all.
    vlc_fourcc_t i_codec_video = 0, i_codec_audio = 0;
    const es_format_t *p_original_audio = nullptr;
    const es_format_t *p_original_video = nullptr;
    const es_format_t *p_original_spu = nullptr;
    sout_stream_id_sys_t *spu_stream = nullptr;
    std::vector<sout_stream_id_sys_t*> new_streams;

    for (sout_stream_id_sys_t *stream : streams)
    {
        const es_format_t *p_es = &stream->fmt;
        if (p_es->i_cat == AUDIO_ES)
        {
            p_original_audio = p_es;
            new_streams.push_back(stream);
        }
        else if (b_supports_video && p_es->i_cat == VIDEO_ES)
        {
            p_original_video = p_es;
            new_streams.push_back(stream);
        }
        else if (b_supports_video && p_es->i_cat == SPU_ES && p_original_spu == nullptr)
        {
            p_original_spu = p_es;
            spu_stream = stream;
        }
    }
    /* the subtitle shown goes burnt into the picture: none without one */
    if (p_original_spu && p_original_video)
        new_streams.push_back(spu_stream);
    else
        p_original_spu = nullptr;

    if (new_streams.empty())
        return VLC_SUCCESS;

    ProtocolPtr stream_protocol;
    // check if we have an audio only stream
    if (!p_original_video && p_original_audio)
    {
        if( !(stream_protocol = canDecodeAudio(p_original_audio->i_codec)) )
        {
            msg_Dbg( p_stream, "can't remux audio track %d codec %4.4s",
                p_original_audio->i_id, (const char*)&p_original_audio->i_codec );
            stream_protocol = make_protocol(default_audio_protocol);
            canRemux = false;
        }
        else
            i_codec_audio = p_original_audio->i_codec;
    }
    // video only stream
    else if (p_original_video && !p_original_audio)
    {
        if( !(stream_protocol = canDecodeVideo(VLC_CODEC_NONE,
                        p_original_video->i_codec)) )
        {
            msg_Dbg(p_stream, "can't remux video track %d codec: %4.4s",
                p_original_video->i_id, (const char*)&p_original_video->i_codec);
            stream_protocol = make_protocol(transcode_video_protocol);
            canRemux = false;
        }
        else
            i_codec_video = p_original_video->i_codec;
    }
    else
    {
        if( !(stream_protocol = canDecodeVideo( p_original_audio->i_codec,
                        p_original_video->i_codec)) )
        {
            msg_Dbg(p_stream, "can't remux video track %d with audio: %4.4s and video: %4.4s",
                p_original_video->i_id, (const char*)&p_original_audio->i_codec,
                    (const char*)&p_original_video->i_codec);
            stream_protocol = make_protocol(transcode_video_protocol);
            canRemux = false;

            // check which codec needs transcoding
            if (stream_protocol->profile.audio_codec == p_original_audio->i_codec)
                i_codec_audio = p_original_audio->i_codec;
            if (stream_protocol->profile.video_codec == p_original_video->i_codec)
                i_codec_video = p_original_video->i_codec;
        }
        else
        {
            i_codec_audio = p_original_audio->i_codec;
            i_codec_video = p_original_video->i_codec;
        }
    }

    /* The subtitle shown in the player goes to the TV burnt into the
     * picture: the video is transcoded, to the profile every renderer plays,
     * and the subtitle blended (soverlay) with the player's style */
    if (p_original_spu)
    {
        msg_Dbg( p_stream, "forcing video transcode because of subtitle '%4.4s'",
                 (const char*)&p_original_spu->i_codec );
        stream_protocol = make_protocol(transcode_video_protocol);
        canRemux = false;
        i_codec_video = 0;
        if (p_original_audio && p_original_audio->i_codec != stream_protocol->profile.audio_codec)
            i_codec_audio = 0;
    }

    /* AAC above 48 kHz (some HE-AAC is signalled at 96 kHz) is not for TVs */
    if (i_codec_audio != 0 && p_original_audio && p_original_audio->audio.i_rate > 48000)
    {
        i_codec_audio = 0;
        canRemux = false;
    }

    /* the device did not read it as it was: converted, as with a subtitle */
    if (force_transcode)
    {
        if (p_original_video)
        {
            stream_protocol = make_protocol(transcode_video_protocol);
            i_codec_video = 0;
            if (p_original_audio && p_original_audio->i_codec != stream_protocol->profile.audio_codec)
                i_codec_audio = 0;
        }
        else
        {
            stream_protocol = make_protocol(default_audio_protocol);
            i_codec_audio = 0;
        }
        canRemux = false;
    }
    {
        const bool copies = p_original_video ? i_codec_video != 0 : i_codec_audio != 0;
        vlc::threads::mutex_locker locker( ctl_lock );
        copies_as_is = copies;
    }

    msg_Dbg( p_stream, "using DLNA profile %s:%s",
                stream_protocol->profile.mime.c_str(),
                stream_protocol->profile.name.c_str() );

    std::ostringstream ssout;
    bool soverlay = false;
    if ( !canRemux )
    {
        if ( !perf_warning_shown && i_codec_video == 0 && p_original_video
          && var_InheritInteger( p_stream, RENDERER_CFG_PREFIX "show-perf-warning" ) )
        {
            int res = vlc_dialog_wait_question( p_stream,
                          VLC_DIALOG_QUESTION_WARNING,
                         _("Cancel"), _("OK"), _("OK, don't warn me again"),
                         _("Performance warning"),
                         _("Casting this video requires conversion. "
                           "This conversion can use all the available power and "
                           "could quickly drain your battery." ) );
            perf_warning_shown = true;
            if ( res <= 0 )
                 return false;
            if ( res == 2 )
                config_PutInt(RENDERER_CFG_PREFIX "show-perf-warning", 0 );
        }

        const int i_quality = var_InheritInteger( p_stream, SOUT_CFG_PREFIX "conversion-quality" );

        /* TODO: provide audio samplerate and channels */
        ssout << "transcode{";
        if ( i_codec_audio == 0 && p_original_audio )
        {
            i_codec_audio = stream_protocol->profile.audio_codec;
            ssout << GetAcodecOption( p_stream, &i_codec_audio,
                                      &p_original_audio->audio, i_quality );
        }
        if ( i_codec_video == 0 && p_original_video )
        {
            i_codec_video = stream_protocol->profile.video_codec;
            try {
                std::string venc = vlc_sout_renderer_GetVcodecOption( p_stream,
                                        { i_codec_video },
                                        &i_codec_video,
                                        &p_original_video->video, i_quality );
                /* a TV plays the profile it is told (Main, AVC_TS_MP_*), and
                 * starts at a keyframe: one every two seconds or so. Each
                 * picture goes as it comes (zerolatency): the renderer has
                 * all that was sent, and goes on through a seek. */
                const size_t x264 = venc.find( "venc=x264{" );
                if ( x264 != std::string::npos )
                    venc.insert( x264 + strlen( "venc=x264{" ),
                                 "profile=main,keyint=60,tune=zerolatency," );
                ssout << venc;
                /* the subtitles burnt in, this one or one chosen later */
                soverlay = true;
            } catch(const std::exception& e) {
                return VLC_EGENERIC ;
            }
        }
        if ( soverlay )
            ssout << "soverlay,";
        ssout << "}:";
    }

    std::ostringstream ss;
    ss << "/dlna"
       << "/" << vlc_tick_now()
       << "/" << static_cast<uint64_t>( vlc_mrand48() )
       << "/stream";
    std::string root_url = ss.str();

    /* the content features of the protocol info (after "http-get:*:mime:"),
     * in the answers of the server too: TVs check them */
    std::string features = dlna_write_protocol_info(*stream_protocol);
    for (int i = 0; i < 3 && features.find(':') != std::string::npos; ++i)
        features.erase(0, features.find(':') + 1);

    ssout << "http{dst=:" << http_port << root_url
          << ",mux=" << stream_protocol->profile.mux
          << ",access=http{mime=" << stream_protocol->profile.mime
          << ",dlna=\"" << features << "\"}}";

    auto ip = vlc::wrap_cptr<char>(getServerIPAddress());
    if (ip == nullptr)
    {
        msg_Err(p_stream, "could not get the local ip address");
        return VLC_EGENERIC;
    }

    char *uri;
    if (asprintf(&uri, "http://%s:%d%s", ip.get(), http_port, root_url.c_str()) < 0) {
        return VLC_ENOMEM;
    }

    if ( !startSoutChain( p_stream, new_streams, ssout.str() ) ) {
        free(uri);
        return VLC_EGENERIC;
    }
    chain_soverlay = soverlay;

    {
        vlc::threads::mutex_locker locker( ctl_lock );
        announce_uri = uri;
        announce_protocol = std::move(stream_protocol);
        announce_at = vlc_tick_now() + VLC_TICK_FROM_MS(500);
        playing = false;
        ctl_wait.signal();
    }

    free(uri);
    return VLC_SUCCESS;
}

/* The stream's time going at date, at the pace (ctl_lock held) */
vlc_tick_t sout_stream_sys_t::BaseTs( vlc_tick_t date ) const
{
    vlc_tick_t paused = paused_total - anchor_paused;
    if ( pause_date != VLC_TICK_INVALID && date > pause_date )
        paused += date - pause_date;
    return anchor_ts + ( date - anchor_date - paused );
}

/* Where the renderer said it is (pos, to the resolution) when it was asked,
 * between sent and received: the origin of its position (when it showed the
 * first picture of the chain) is in between. The polls, at other moments in
 * the second, narrow it down; how much later than the pace it shows the
 * stream follows ("renderer-latency" of libvlc: the picture here waits as
 * long, and both show the same). Again in a while: it can drift. */
void sout_stream_sys_t::Measure( sout_stream_t *p_stream, vlc_tick_t pos,
                                 vlc_tick_t resolution, vlc_tick_t sent,
                                 vlc_tick_t received )
{
    if ( pos == VLC_TICK_INVALID || pos < VLC_TICK_FROM_MS(300) )
    {
        /* it does not say, or has not started yet */
        measure_at = received + VLC_TICK_FROM_MS( pos == VLC_TICK_INVALID ? 1000 : 250 );
        polls = 0;
        return;
    }

    const vlc_tick_t lo = sent - pos - resolution;
    const vlc_tick_t hi = received - pos;
    const vlc_tick_t margin = VLC_TICK_FROM_MS(150);
    if ( polls > 0 && ( lo > origin_hi + margin || hi < origin_lo - margin ) )
        polls = 0;      /* it did not go on as it did (it waited, it jumped) */
    if ( polls == 0 )
    {
        origin_lo = lo;
        origin_hi = hi;
        measure_until = received + VLC_TICK_FROM_SEC(4);
    }
    else
    {
        origin_lo = std::max( origin_lo, lo );
        origin_hi = std::min( origin_hi, hi );
        if ( origin_lo > origin_hi )
            origin_lo = origin_hi = ( origin_lo + origin_hi ) / 2;
    }
    polls++;

    if ( !( polls >= 6 && origin_hi - origin_lo <= VLC_TICK_FROM_MS(40) )
      && received < measure_until )
    {
        measure_at = received + VLC_TICK_FROM_MS(90);
        return;
    }
    polls = 0;
    if ( origin_hi - origin_lo > VLC_TICK_FROM_MS(250) )
    {
        msg_Dbg( p_stream, "the renderer's position is too vague (%" PRId64 " ms)",
                 MS_FROM_VLC_TICK( origin_hi - origin_lo ) );
        measure_at = received + VLC_TICK_FROM_SEC(5);
        return;
    }
    measure_at = received + VLC_TICK_FROM_SEC(30);

    const vlc_tick_t origin = ( origin_lo + origin_hi ) / 2;
    const vlc_tick_t measured = origin - tl_first + anchor_ts - anchor_date
                              - ( paused_total - anchor_paused );
    msg_Dbg( p_stream, "the renderer shows the stream %" PRId64 " ms after its pace "
             "(to %" PRId64 " ms)", MS_FROM_VLC_TICK( measured ),
             MS_FROM_VLC_TICK( origin_hi - origin_lo ) );
    if ( latency != VLC_TICK_INVALID && llabs( measured - latency ) < VLC_TICK_FROM_MS(40) )
        return;
    latency = measured;
    if ( libvlc != nullptr && var_Type( libvlc, "renderer-latency" ) )
        var_SetInteger( libvlc, "renderer-latency", MS_FROM_VLC_TICK( measured ) );
}

/* Where the renderer is in the media, in the input's time ("renderer-time"
 * of libvlc, for the interface): behind the pace by its delay. After a
 * seek, until it gets to the new part, where that part starts (what the
 * user asked for). ctl_lock held. */
void sout_stream_sys_t::PublishTime( vlc_tick_t now )
{
    const vlc_tick_t ts = BaseTs( now ) - latency;
    while ( segments.size() > 1 && segments[1].first <= ts )
        segments.pop_front();
    if ( segments.empty() )
        return;
    const std::pair<vlc_tick_t, vlc_tick_t> &part = segments.back();
    const vlc_tick_t at = segments.size() > 1 ? part.first : ts;
    var_SetInteger( libvlc, "renderer-time", std::max<vlc_tick_t>( at - part.second, 0 ) );
}

/* ctl_lock held */
void sout_stream_sys_t::Report( int new_status, const char *error )
{
    status = new_status;
    if ( libvlc == nullptr || !var_Type( libvlc, "renderer-state" ) )
        return;
    std::string state = new_status == RENDERER_CONNECTING ? "connecting"
                      : new_status == RENDERER_PLAYING ? "playing"
                      : std::string( "failed:" ) + error;
    state += '|' + renderer->device_url;
    var_SetString( libvlc, "renderer-state", state.c_str() );
}

/* It does not play what it was told: the interface says why, and offers to
 * try again or to play here. ctl_lock held. */
void sout_stream_sys_t::Fail( sout_stream_t *p_stream, const char *error, int code )
{
    /* a track copied as it was, not read: converted once first (the
     * device may not take its profile or its container) */
    if ( copies_as_is && !force_transcode && !fallback_request
      && ( !strcmp( error, "unreadable" ) || !strcmp( error, "refused" ) ) )
    {
        msg_Warn( p_stream, "the renderer does not read the stream as it is (%s, %d): "
                  "converting it", error, code );
        playing = false;
        check_at = VLC_TICK_INVALID;
        copies_as_is = false;
        fallback_request = true;
        return;
    }
    msg_Warn( p_stream, "the renderer does not play: %s (%d)", error, code );
    playing = false;
    check_at = VLC_TICK_INVALID;
    if ( !strcmp( error, "unreachable" ) )
        unreachable = true;
    Report( RENDERER_FAILED, error );
}

/* What it said it does (state, "" with the error when it did not say).
 * Right after Play, some say STOPPED a moment, and TRANSITIONING while they
 * read the beginning: it fails when it stays stopped (or never plays), and
 * afterwards when it stops by itself. At the end of the media, it is told to
 * stop once it showed it all: that is not it stopping. ctl_lock held. */
void sout_stream_sys_t::CheckTransport( sout_stream_t *p_stream, const std::string &state,
                                        int error, vlc_tick_t now )
{
    if ( !playing || check_at == VLC_TICK_INVALID )
        return;
    const vlc_tick_t next = seen_playing ? VLC_TICK_FROM_SEC(2) : VLC_TICK_FROM_MS(500);
    check_at = now + next;
    if ( state.empty() )
    {
        if ( NetworkError( error ) )
        {
            if ( ++failed_checks >= 2 )
                Fail( p_stream, "unreachable", error );
        }
        else
        {
            /* it does not tell (GetTransportInfo): it plays, as it said */
            check_at = VLC_TICK_INVALID;
            if ( status != RENDERER_PLAYING )
                Report( RENDERER_PLAYING, "" );
        }
        return;
    }
    failed_checks = 0;
    if ( state == "PLAYING" || state == "PAUSED_PLAYBACK" )
    {
        stopped_checks = 0;
        if ( !seen_playing )
            msg_Dbg( p_stream, "the renderer plays (after %" PRId64 " ms)",
                     MS_FROM_VLC_TICK( now - started_at ) );
        seen_playing = true;
        check_at = now + VLC_TICK_FROM_SEC(2);
        if ( status != RENDERER_PLAYING )
            Report( RENDERER_PLAYING, "" );
    }
    else if ( state == "STOPPED" || state == "NO_MEDIA_PRESENT" )
    {
        if ( stop_at != VLC_TICK_INVALID )
            return;     /* the end: it showed it all */
        if ( ( seen_playing || now - started_at >= VLC_TICK_FROM_SEC(4) )
          && ++stopped_checks >= 2 )
            Fail( p_stream, seen_playing ? "stopped" : "unreadable", 0 );
    }
    else
    {
        /* TRANSITIONING: it reads the stream (a TV takes a few seconds) */
        stopped_checks = 0;
        if ( !seen_playing && now - started_at >= VLC_TICK_FROM_SEC(30) )
            Fail( p_stream, "unreadable", 0 );
    }
}

/* The input adds its tracks one at a time, and the audio one often sends
 * before the video one is added: each change makes a new stream. The
 * renderer is told to play once the tracks stayed the same for a moment,
 * so that a TV does not start an audio stream and then switch. Then it
 * follows what the interface asks: pause, volume, mute. Its delay is
 * measured as it plays, and where it is in the media said. */
void sout_stream_sys_t::ControlLoop( sout_stream_t *p_stream )
{
    vlc::threads::mutex_locker locker( ctl_lock );
    while ( !ctl_stop )
    {
        const vlc_tick_t now = vlc_tick_now();
        vlc_tick_t deadline = VLC_TICK_INVALID;
        const auto wake = [&deadline]( vlc_tick_t at ) {
            if ( deadline == VLC_TICK_INVALID || at < deadline )
                deadline = at;
        };

        if ( stop_at != VLC_TICK_INVALID && announce_uri.empty() )
        {
            if ( now >= stop_at || unreachable )
            {
                stop_at = VLC_TICK_INVALID;
                playing = false;
                if ( !unreachable )
                {
                    ctl_lock.unlock();
                    renderer->Stop();
                    ctl_lock.lock();
                }
                ended_done = true;
                continue;
            }
            wake( stop_at );
        }
        if ( !announce_uri.empty() )
        {
            if ( stop_at != VLC_TICK_INVALID && announce_at < stop_at )
                announce_at = stop_at;
            if ( unreachable )
            {
                announce_uri.clear();
                ended_done = true;
            }
            else if ( now >= announce_at )
            {
                stop_at = VLC_TICK_INVALID;
                stop_now = false;
                const std::string uri = announce_uri;
                const protocol_info_t protocol = *announce_protocol;
                const bool paused = want_paused;
                const bool first = sent_volume < 0;
                announce_uri.clear();
                ctl_lock.unlock();

                /* (some have no PrepareForConnection, and a Stop with
                 * nothing to stop may fail: only the network's errors
                 * count there, and they end it, each would wait as long) */
                int error = 0;
                renderer->PrepareForConnection(dlna_write_protocol_info(protocol).c_str());
                if ( NetworkError( renderer->last_error ) )
                    error = renderer->last_error;
                if ( error == 0 )
                {
                    msg_Dbg(p_stream, "AVTransportURI: %s", uri.c_str());
                    renderer->Stop();
                    if ( NetworkError( renderer->last_error ) )
                        error = renderer->last_error;
                }
                if ( error == 0 && renderer->SetAVTransportURI(uri.c_str(), protocol) != VLC_SUCCESS )
                    error = renderer->last_error ? renderer->last_error : -1;
                if ( error == 0 && renderer->Play("1") != VLC_SUCCESS )
                    error = renderer->last_error ? renderer->last_error : -1;
                if ( error == 0 && paused )
                    renderer->Pause();
                /* its volume, for the interface's control (once) */
                const int volume = first && error == 0 ? renderer->GetVolume() : -1;

                ctl_lock.lock();
                ended_done = true;
                if ( error != 0 )
                {
                    /* 716: it could not get the stream (it tried at once) */
                    Fail( p_stream, NetworkError( error ) ? "unreachable"
                                  : error == 716 ? "unreadable" : "refused", error );
                    continue;
                }
                playing = true;
                sent_paused = paused;
                started_at = vlc_tick_now();
                check_at = started_at + VLC_TICK_FROM_MS(500);
                seen_playing = false;
                stopped_checks = failed_checks = 0;
                if ( volume >= 0 )
                {
                    sent_volume = volume;
                    if ( libvlc != nullptr && var_Type( libvlc, "renderer-volume-device" ) )
                        var_SetInteger( libvlc, "renderer-volume-device", volume );
                }
                continue;
            }
            else
                wake( announce_at );
        }
        if ( unreachable )
        {
            sent_paused = want_paused;
            sent_volume = want_volume;
            sent_muted = want_muted;
        }
        if ( playing && want_paused != sent_paused )
        {
            const bool paused = want_paused;
            sent_paused = paused;
            ctl_lock.unlock();
            if ( paused )
                renderer->Pause();
            else
                renderer->Play("1");
            ctl_lock.lock();
            continue;
        }
        if ( want_volume >= 0 && want_volume != sent_volume )
        {
            const int volume = want_volume;
            sent_volume = volume;
            ctl_lock.unlock();
            renderer->SetVolume( volume );
            ctl_lock.lock();
            continue;
        }
        if ( want_muted >= 0 && want_muted != sent_muted )
        {
            const bool muted = want_muted > 0;
            sent_muted = want_muted;
            ctl_lock.unlock();
            renderer->SetMute( muted );
            ctl_lock.lock();
            continue;
        }
        if ( playing && check_at != VLC_TICK_INVALID && !unreachable )
        {
            if ( now >= check_at )
            {
                ctl_lock.unlock();
                const std::string state = renderer->GetTransportState();
                const int error = state.empty() ? renderer->last_error : 0;
                ctl_lock.lock();
                CheckTransport( p_stream, state, error, vlc_tick_now() );
                continue;
            }
            wake( check_at );
        }
        if ( playing && !sent_paused && pause_date == VLC_TICK_INVALID
          && tl_first != VLC_TICK_INVALID && anchor_date != VLC_TICK_INVALID
          && measure_at != VLC_TICK_INVALID && !unreachable )
        {
            if ( now >= measure_at )
            {
                ctl_lock.unlock();
                vlc_tick_t resolution = VLC_TICK_FROM_SEC(1);
                const vlc_tick_t sent = vlc_tick_now();
                const vlc_tick_t pos = renderer->GetPosition( &resolution );
                const vlc_tick_t received = vlc_tick_now();
                ctl_lock.lock();
                if ( playing && pause_date == VLC_TICK_INVALID && tl_first != VLC_TICK_INVALID )
                    Measure( p_stream, pos, resolution, sent, received );
                continue;
            }
            wake( measure_at );
        }
        if ( latency != VLC_TICK_INVALID && anchor_date != VLC_TICK_INVALID
          && libvlc != nullptr && var_Type( libvlc, "renderer-time" ) )
        {
            if ( time_at == VLC_TICK_INVALID || now >= time_at )
            {
                PublishTime( now );
                time_at = now + VLC_TICK_FROM_MS(200);
            }
            if ( pause_date == VLC_TICK_INVALID )
                wake( time_at );
        }

        if ( deadline == VLC_TICK_INVALID )
            ctl_wait.wait( ctl_lock );
        else
            ctl_wait.timedwait( ctl_lock, deadline );
    }
}

static void *FeedThread( void *data )
{
    vlc_thread_set_name( "vlc-dlna-feed" );
    sout_stream_t *p_stream = static_cast<sout_stream_t *>( data );
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );
    vlc::threads::mutex_locker locker( p_sys->feed_lock );
    while ( !p_sys->feed_stop )
    {
        if ( !p_sys->held.empty() )
            p_sys->Feed( p_stream, false );
        if ( p_sys->held.empty() )
            p_sys->feed_wait.wait( p_sys->feed_lock );
        else
            p_sys->feed_wait.timedwait( p_sys->feed_lock,
                                        vlc_tick_now() + VLC_TICK_FROM_MS(20) );
    }
    return nullptr;
}

static void *ControlThread( void *data )
{
    vlc_thread_set_name( "vlc-dlna-ctl" );
    sout_stream_t *p_stream = static_cast<sout_stream_t *>( data );
    static_cast<sout_stream_sys_t *>( p_stream->p_sys )->ControlLoop( p_stream );
    return nullptr;
}

/* The interface's controls while it casts ("renderer-*" of libvlc). A pause
 * holds what is sent too, and the pace goes on as much later. */
static int RendererVarChanged( vlc_object_t *, char const *psz_var, vlc_value_t,
                               vlc_value_t newval, void *data )
{
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( data );
    vlc::threads::mutex_locker locker( p_sys->ctl_lock );
    if ( !strcmp( psz_var, "renderer-paused" ) )
    {
        const vlc_tick_t now = vlc_tick_now();
        if ( newval.b_bool && p_sys->pause_date == VLC_TICK_INVALID )
            p_sys->pause_date = now;
        else if ( !newval.b_bool && p_sys->pause_date != VLC_TICK_INVALID )
        {
            p_sys->paused_total += now - p_sys->pause_date;
            p_sys->pause_date = VLC_TICK_INVALID;
            /* it goes on when it is told: its delay to see again */
            p_sys->polls = 0;
            if ( p_sys->tl_first != VLC_TICK_INVALID )
                p_sys->measure_at = now + VLC_TICK_FROM_MS(1500);
        }
        p_sys->want_paused = newval.b_bool;
        p_sys->time_at = VLC_TICK_INVALID;
    }
    else if ( !strcmp( psz_var, "renderer-volume" ) )
        p_sys->want_volume = newval.i_int;
    else if ( !strcmp( psz_var, "renderer-muted" ) )
        p_sys->want_muted = newval.i_int;
    else if ( !strcmp( psz_var, "renderer-display" ) )
        p_sys->display_wanted = newval.b_bool;
    else if ( !strcmp( psz_var, "renderer-stop" ) )
    {
        p_sys->stop_now = true;
        if ( p_sys->playing || p_sys->stop_at != VLC_TICK_INVALID )
            p_sys->stop_at = vlc_tick_now();
    }
    p_sys->ctl_wait.signal();
    return VLC_SUCCESS;
}

static const char *const renderer_vars[] = {
    "renderer-paused", "renderer-volume", "renderer-muted", "renderer-display",
    "renderer-stop",
};

/* What comes from the input, held until it goes (Feed) */
void sout_stream_sys_t::Hold( sout_stream_t *p_stream, sout_stream_id_sys_t *id,
                              block_t *block )
{
    const bool timed = !( retime && id->fresh );
    if ( timed && ts_offset != 0 )
        BlockShift( block, ts_offset );
    /* what is sent goes on without a break, seeks or not */
    block->i_flags &= ~BLOCK_FLAG_DISCONTINUITY;
    /* a subtitle goes at once: the conversion blends it into the pictures
     * of its time as they go, so it must be there before them */
    if ( timed && id->fmt.i_cat == SPU_ES )
    {
        Release( p_stream, id, block );
        return;
    }
    held.push_back( { id, block, timed, vlc_tick_now() } );
    feed_wait.signal();
    if ( retime )
        Retime( p_stream, false );
}

/* After a seek, the new part follows what was sent: its first picture shown
 * right after the end of what was sent. The pictures before the place asked
 * (from the keyframe before it) are there to decode the next ones: said so
 * (preroll), but the keyframe often is not. So the first shown is the
 * earliest of the pictures after the last of those said so, once a few came
 * (or a moment passed, or it ends: force); those before it are made preroll
 * (the decoders make nothing of them), and the sound before it goes (it
 * would be over what was sent). */
void sout_stream_sys_t::Retime( sout_stream_t *p_stream, bool force )
{
    const auto picture_ts = []( const block_t *block ) {
        return block->i_pts != VLC_TICK_INVALID ? block->i_pts : block->i_dts;
    };
    vlc_tick_t last_preroll = VLC_TICK_INVALID, first_sound = VLC_TICK_INVALID;
    for ( const held_block &h : held )
    {
        if ( h.timed )
            continue;
        if ( h.id->fmt.i_cat == VIDEO_ES && ( h.block->i_flags & BLOCK_FLAG_PREROLL ) )
        {
            const vlc_tick_t pts = picture_ts( h.block );
            if ( pts != VLC_TICK_INVALID && ( last_preroll == VLC_TICK_INVALID || pts > last_preroll ) )
                last_preroll = pts;
        }
        else if ( h.id->fmt.i_cat == AUDIO_ES && !( h.block->i_flags & BLOCK_FLAG_PREROLL ) )
        {
            const vlc_tick_t ts = BlockTs( h.block );
            if ( ts != VLC_TICK_INVALID && ( first_sound == VLC_TICK_INVALID || ts < first_sound ) )
                first_sound = ts;
        }
    }
    vlc_tick_t first_picture = VLC_TICK_INVALID;
    unsigned pictures = 0;
    for ( const held_block &h : held )
    {
        if ( h.timed || h.id->fmt.i_cat != VIDEO_ES || ( h.block->i_flags & BLOCK_FLAG_PREROLL ) )
            continue;
        const vlc_tick_t pts = picture_ts( h.block );
        if ( pts == VLC_TICK_INVALID
          || ( last_preroll != VLC_TICK_INVALID && pts <= last_preroll ) )
            continue;
        if ( first_picture == VLC_TICK_INVALID || pts < first_picture )
            first_picture = pts;
        pictures++;
    }
    bool video = false;
    for ( const sout_stream_id_sys_t *id : streams )
        if ( id->fmt.i_cat == VIDEO_ES )
            video = true;
    const vlc_tick_t waited = vlc_tick_now() - retime_since;
    if ( !force )
    {
        if ( video ? pictures < 4 && !( pictures > 0 && waited >= VLC_TICK_FROM_MS(500) )
                     && waited < VLC_TICK_FROM_SEC(3)
                   : first_sound == VLC_TICK_INVALID )
            return;
    }
    const vlc_tick_t first = first_picture != VLC_TICK_INVALID ? first_picture : first_sound;

    if ( first != VLC_TICK_INVALID && sent_end != VLC_TICK_INVALID )
        ts_offset = sent_end - first;
    msg_Dbg( p_stream, "after the seek, the stream goes on at %" PRId64 " ms of the media "
             "(offset %" PRId64 " ms)", first != VLC_TICK_INVALID ? MS_FROM_VLC_TICK( first ) : -1,
             MS_FROM_VLC_TICK( ts_offset ) );
    for ( auto it = held.begin(); it != held.end(); )
    {
        if ( it->timed )
        {
            ++it;
            continue;
        }
        block_t *block = it->block;
        if ( first != VLC_TICK_INVALID )
        {
            const vlc_tick_t ts = it->id->fmt.i_cat == VIDEO_ES ? picture_ts( block )
                                                               : BlockTs( block );
            if ( ts != VLC_TICK_INVALID && ts < first )
            {
                /* the sound before the first picture: over what was sent */
                if ( it->id->fmt.i_cat == AUDIO_ES )
                {
                    block_Release( block );
                    it = held.erase( it );
                    continue;
                }
                if ( it->id->fmt.i_cat == VIDEO_ES )
                    block->i_flags |= BLOCK_FLAG_PREROLL;
            }
        }
        if ( ts_offset != 0 )
            BlockShift( block, ts_offset );
        it->timed = true;
        ++it;
    }
    for ( sout_stream_id_sys_t *id : streams )
        id->fresh = false;
    retime = false;
    for ( auto it = held.begin(); it != held.end(); )
        if ( it->id->fmt.i_cat == SPU_ES )
        {
            sout_stream_id_sys_t *id = it->id;
            block_t *block = it->block;
            it = held.erase( it );
            Release( p_stream, id, block );
        }
        else
            ++it;

    if ( first != VLC_TICK_INVALID )
    {
        vlc::threads::mutex_locker locker( ctl_lock );
        segments.emplace_back( first + ts_offset, ts_offset );
        time_at = VLC_TICK_INVALID;
        /* once it shows the new part: its delay again (it may have
         * waited for it) */
        if ( tl_first != VLC_TICK_INVALID && latency != VLC_TICK_INVALID )
        {
            polls = 0;
            measure_at = vlc_tick_now() + latency + FEED_LEAD + VLC_TICK_FROM_MS(1500);
        }
        ctl_wait.signal();
    }
}

/* The pace starts (or again: the input waited): the stream's time ts goes
 * now. The display here starts its clock again. */
void sout_stream_sys_t::Anchor( sout_stream_t *p_stream, vlc_tick_t date, vlc_tick_t ts )
{
    {
        vlc::threads::mutex_locker locker( ctl_lock );
        if ( anchor_date != VLC_TICK_INVALID )
        {
            /* the renderer probably waited too: its delay to see again */
            polls = 0;
            if ( tl_first != VLC_TICK_INVALID )
                measure_at = date + VLC_TICK_FROM_SEC(2);
        }
        anchor_date = date;
        anchor_ts = ts;
        anchor_paused = paused_total;
        if ( segments.empty() )
            segments.emplace_back( ts, ts_offset );
        time_at = VLC_TICK_INVALID;
        ctl_wait.signal();
    }
    if ( display_started )
    {
        for ( sout_stream_id_sys_t *id : streams )
            if ( id->p_display_id != nullptr )
                sout_StreamFlush( p_display, id->p_display_id );
        display_started = false;
        display_keyframe = false;
        display_wait = VLC_TICK_INVALID;
    }
    (void) p_stream;
}

/* What is held goes once its time comes at the pace of the media (a little
 * ahead), nothing while paused; all of it when the media ends (all) */
void sout_stream_sys_t::Feed( sout_stream_t *p_stream, bool all )
{
    while ( !held.empty() )
    {
        const held_block h = held.front();
        if ( !all )
        {
            if ( !h.timed || ( p_out == nullptr && es_changed ) )
                break;
            const vlc_tick_t ts = BlockTs( h.block );
            /* (the pictures before a seek's place go at once: to decode the
             * next ones) */
            if ( h.id->fmt.i_cat != SPU_ES && ts != VLC_TICK_INVALID
              && !( h.block->i_flags & BLOCK_FLAG_PREROLL ) )
            {
                const vlc_tick_t now = vlc_tick_now();
                if ( chain_soverlay && now < h.arrival + FEED_SUBTITLE )
                {
                    bool subtitle = false;
                    for ( const sout_stream_id_sys_t *out : out_streams )
                        if ( out->fmt.i_cat == SPU_ES )
                            subtitle = true;
                    if ( subtitle )
                        break;
                }
                vlc_tick_t base = VLC_TICK_INVALID;
                {
                    vlc::threads::mutex_locker locker( ctl_lock );
                    if ( pause_date != VLC_TICK_INVALID )
                        break;
                    if ( anchor_date != VLC_TICK_INVALID )
                        base = BaseTs( now );
                }
                if ( base == VLC_TICK_INVALID )
                    Anchor( p_stream, now, ts );
                else if ( ts > base + FEED_LEAD )
                {
                    late_since = VLC_TICK_INVALID;
                    const vlc_tick_t last = BlockTs( held.back().block );
                    if ( last == VLC_TICK_INVALID || last - ts < FEED_MAX_HOLD )
                        break;
                }
                else if ( ts < base - FEED_SLIP )
                {
                    /* late: it goes at once; if it stays so, the input
                     * waited, and its pace starts again from here */
                    if ( late_since == VLC_TICK_INVALID )
                        late_since = now;
                    else if ( now - late_since > VLC_TICK_FROM_SEC(3) )
                    {
                        msg_Warn( p_stream, "the stream comes %" PRId64 " ms late: "
                                  "its pace starts again", MS_FROM_VLC_TICK( base - ts ) );
                        Anchor( p_stream, now, ts );
                        late_since = VLC_TICK_INVALID;
                    }
                }
                else
                    late_since = VLC_TICK_INVALID;
            }
        }
        held.pop_front();
        Release( p_stream, h.id, h.block );
    }
}

/* To the renderer and to the picture here */
void sout_stream_sys_t::Release( sout_stream_t *p_stream, sout_stream_id_sys_t *id,
                                 block_t *block )
{
    /* the first picture of the input (where it started or went) is one */
    const bool keyframe = id->fmt.i_cat == VIDEO_ES
                       && ( !video_sent || IsKeyframe( &id->fmt, block ) );
    if ( id->fmt.i_cat == VIDEO_ES )
        video_sent = true;

    if ( id->fmt.i_cat != SPU_ES && block->i_pts != VLC_TICK_INVALID )
    {
        const vlc_tick_t end = block->i_pts + std::max<vlc_tick_t>( block->i_length, 0 );
        if ( sent_end == VLC_TICK_INVALID || end > sent_end )
            sent_end = end;
    }

    /* (its picture from a keyframe, or after a while if none says it is) */
    if ( p_display != nullptr && display_wait == VLC_TICK_INVALID )
        display_wait = vlc_tick_now();
    if ( p_display != nullptr && id->p_display_id != nullptr
      && ( display_keyframe || id->fmt.i_cat != VIDEO_ES || keyframe
        || vlc_tick_now() - display_wait > VLC_TICK_FROM_SEC(3) ) )
    {
        if ( !display_started )
        {
            vlc_tick_t base;
            {
                vlc::threads::mutex_locker locker( ctl_lock );
                base = anchor_date != VLC_TICK_INVALID ? BaseTs( vlc_tick_now() )
                                                       : BlockTs( block );
            }
            sout_StreamSetPCR( p_display, base );
            display_started = true;
        }
        if ( id->fmt.i_cat == VIDEO_ES )
            display_keyframe = true;
        block_t *copy = block_Duplicate( block );
        if ( copy != nullptr )
            sout_StreamIdSend( p_display, id->p_display_id, copy );
    }

    if ( p_out == nullptr || id->p_sub_id == nullptr )
    {
        block_Release( block );
        return;
    }
    if ( !chain_started )
    {
        bool video = false;
        for ( const sout_stream_id_sys_t *out : out_streams )
            if ( out->fmt.i_cat == VIDEO_ES )
                video = true;
        const vlc_tick_t now = vlc_tick_now();
        if ( chain_wait == VLC_TICK_INVALID )
            chain_wait = now;
        const bool start = video ? id->fmt.i_cat == VIDEO_ES
                                   && ( keyframe || now - chain_wait > VLC_TICK_FROM_SEC(3) )
                                 : id->fmt.i_cat == AUDIO_ES;
        if ( !start )
        {
            block_Release( block );
            return;
        }
        chain_started = true;
        vlc::threads::mutex_locker locker( ctl_lock );
        tl_first = block->i_pts != VLC_TICK_INVALID ? block->i_pts : BlockTs( block );
        msg_Dbg( p_stream, "the renderer's stream starts at %" PRId64 " ms (%" PRId64
                 " ms of the media)", MS_FROM_VLC_TICK( tl_first ),
                 MS_FROM_VLC_TICK( tl_first - ts_offset ) );
        polls = 0;
        measure_at = now + VLC_TICK_FROM_SEC(2);
        ctl_wait.signal();
    }
    sout_StreamIdSend( p_out, id->p_sub_id, block );
}

/* The picture here too, as the interface asks: a display output for the
 * picture and the subtitle (the sound plays on the renderer) */
void sout_stream_sys_t::UpdateDisplay( sout_stream_t *p_stream )
{
    bool wanted;
    {
        vlc::threads::mutex_locker locker( ctl_lock );
        wanted = display_wanted && b_supports_video;
    }
    if ( wanted == ( p_display != nullptr ) )
        return;
    if ( !wanted )
    {
        StopDisplay();
        return;
    }

    p_display = sout_StreamChainNew( VLC_OBJECT(p_stream), "display{noaudio}", nullptr );
    if ( p_display == nullptr )
    {
        msg_Err( p_stream, "cannot show the picture here" );
        vlc::threads::mutex_locker locker( ctl_lock );
        display_wanted = false;
        return;
    }
    for ( sout_stream_id_sys_t *id : streams )
        if ( id->fmt.i_cat == VIDEO_ES || id->fmt.i_cat == SPU_ES )
            id->p_display_id = sout_StreamIdAdd( p_display, &id->fmt, id->es_id );
    display_started = false;
    display_keyframe = false;
    display_wait = VLC_TICK_INVALID;
}

/* The media ended: the next one has a timeline of its own (its stream, its
 * pace) */
void sout_stream_sys_t::ResetTimeline()
{
    for ( const held_block &h : held )
        block_Release( h.block );
    held.clear();
    ts_offset = 0;
    retime = false;
    sent_end = VLC_TICK_INVALID;
    late_since = VLC_TICK_INVALID;
    video_sent = false;
    vlc::threads::mutex_locker locker( ctl_lock );
    anchor_date = anchor_ts = VLC_TICK_INVALID;
    segments.clear();
    time_at = VLC_TICK_INVALID;
    if ( libvlc != nullptr && var_Type( libvlc, "renderer-time" ) )
        var_SetInteger( libvlc, "renderer-time", -1 );
}

void sout_stream_sys_t::StopDisplay()
{
    if ( p_display == nullptr )
        return;
    for ( sout_stream_id_sys_t *id : streams )
        if ( id->p_display_id != nullptr )
        {
            sout_StreamIdDel( p_display, id->p_display_id );
            id->p_display_id = nullptr;
        }
    sout_StreamChainDelete( p_display, nullptr );
    p_display = nullptr;
    display_started = false;
    display_keyframe = false;
    display_wait = VLC_TICK_INVALID;
}

char *MediaRenderer::getServiceURL(const char* type, const char *service)
{
    /* each action needs one: the description is downloaded once */
    const std::string key = std::string(type) + '|' + service;
    auto cached = service_urls.find(key);
    if (cached != service_urls.end())
        return strdup(cached->second.c_str());

    IXML_Document *p_description_doc = nullptr;
    const int ret = UpnpDownloadXmlDoc(device_url.c_str(), &p_description_doc);
    if (ret != UPNP_E_SUCCESS)
    {
        last_error = ret < 0 ? ret : UPNP_E_NETWORK_ERROR;
        return nullptr;
    }

    char *psz_found = nullptr;
    IXML_NodeList* p_device_list = ixmlDocument_getElementsByTagName( p_description_doc, "device");
    if ( !p_device_list )
    {
        ixmlDocument_free(p_description_doc);
        return nullptr;
    }

    for (unsigned int i = 0; i < ixmlNodeList_length(p_device_list); ++i)
    {
        IXML_Element* p_device_element = ( IXML_Element* ) ixmlNodeList_item( p_device_list, i );
        if( !p_device_element )
            continue;

        IXML_NodeList* p_service_list = ixmlElement_getElementsByTagName( p_device_element, "service" );
        if ( !p_service_list )
            continue;
        for ( unsigned int j = 0; j < ixmlNodeList_length( p_service_list ); j++ )
        {
            IXML_Element* p_service_element = (IXML_Element*)ixmlNodeList_item( p_service_list, j );

            const char* psz_service_type = xml_getChildElementValue( p_service_element, "serviceType" );
            if ( !psz_service_type || !strstr(psz_service_type, type))
                continue;
            const char* psz_control_url = xml_getChildElementValue( p_service_element,
                                                                    service );
            if ( !psz_control_url )
                continue;

            char* psz_url = NULL;
            if ( UpnpResolveURL2( base_url.c_str(), psz_control_url, &psz_url ) == UPNP_E_SUCCESS )
                psz_found = psz_url;
            break;
        }
        ixmlNodeList_free( p_service_list );
        if ( psz_found )
            break;
    }
    ixmlNodeList_free( p_device_list );
    ixmlDocument_free( p_description_doc );

    if ( psz_found )
        service_urls[key] = psz_found;
    return psz_found;
}

/* The text of the first element of that name in a response */
static std::string responseValue( IXML_Document *response, const char *name )
{
    std::string value;
    IXML_NodeList *list = ixmlDocument_getElementsByTagName( response, name );
    if ( list )
    {
        IXML_Node *node = ixmlNodeList_item( list, 0 );
        IXML_Node *text = node ? ixmlNode_getFirstChild( node ) : nullptr;
        const char *psz = text ? ixmlNode_getNodeValue( text ) : nullptr;
        if ( psz )
            value = psz;
        ixmlNodeList_free( list );
    }
    return value;
}

int MediaRenderer::Pause()
{
    std::list<std::pair<const char*, const char*>> arg_list = {
        {"InstanceID", AVTransportID.c_str()},
    };

    IXML_Document *p_response = SendAction("Pause", AV_TRANSPORT_SERVICE_TYPE, arg_list);
    if (!p_response)
        return VLC_EGENERIC;
    ixmlDocument_free(p_response);
    return VLC_SUCCESS;
}

vlc_tick_t MediaRenderer::GetPosition(vlc_tick_t *resolution)
{
    std::list<std::pair<const char*, const char*>> arg_list = {
        {"InstanceID", AVTransportID.c_str()},
    };

    IXML_Document *p_response = SendAction("GetPositionInfo", AV_TRANSPORT_SERVICE_TYPE, arg_list);
    if (!p_response)
        return VLC_TICK_INVALID;
    const std::string rel_time = responseValue(p_response, "RelTime");
    ixmlDocument_free(p_response);

    /* H:MM:SS, sometimes with a fraction, which says how precise it is
     * ("NOT_IMPLEMENTED" otherwise) */
    unsigned hours, minutes;
    double seconds;
    if (sscanf(rel_time.c_str(), "%u:%u:%lf", &hours, &minutes, &seconds) != 3)
        return VLC_TICK_INVALID;
    *resolution = VLC_TICK_FROM_SEC(1);
    const size_t dot = rel_time.find('.');
    if (dot != std::string::npos)
        for (size_t i = dot + 1; i < rel_time.size() && isdigit((unsigned char)rel_time[i])
                                 && *resolution > VLC_TICK_FROM_MS(1); ++i)
            *resolution /= 10;
    return vlc_tick_from_sec(hours * 3600 + minutes * 60) + vlc_tick_from_sec(seconds);
}

int MediaRenderer::GetVolume()
{
    std::list<std::pair<const char*, const char*>> arg_list = {
        {"InstanceID", RcsID.c_str()},
        {"Channel", "Master"},
    };

    IXML_Document *p_response = SendAction("GetVolume", RENDERING_CONTROL_SERVICE_TYPE, arg_list);
    if (!p_response)
        return -1;
    const std::string volume = responseValue(p_response, "CurrentVolume");
    ixmlDocument_free(p_response);
    return volume.empty() ? -1 : atoi(volume.c_str());
}

int MediaRenderer::SetVolume(int volume)
{
    const std::string desired = std::to_string(volume);
    std::list<std::pair<const char*, const char*>> arg_list = {
        {"InstanceID", RcsID.c_str()},
        {"Channel", "Master"},
        {"DesiredVolume", desired.c_str()},
    };

    IXML_Document *p_response = SendAction("SetVolume", RENDERING_CONTROL_SERVICE_TYPE, arg_list);
    if (!p_response)
        return VLC_EGENERIC;
    ixmlDocument_free(p_response);
    return VLC_SUCCESS;
}

int MediaRenderer::SetMute(bool mute)
{
    std::list<std::pair<const char*, const char*>> arg_list = {
        {"InstanceID", RcsID.c_str()},
        {"Channel", "Master"},
        {"DesiredMute", mute ? "1" : "0"},
    };

    IXML_Document *p_response = SendAction("SetMute", RENDERING_CONTROL_SERVICE_TYPE, arg_list);
    if (!p_response)
        return VLC_EGENERIC;
    ixmlDocument_free(p_response);
    return VLC_SUCCESS;
}

/**
 * Send an action to the control url of the service specified.
 *
 * \return the response as a IXML document or nullptr for failure
 **/
IXML_Document *MediaRenderer::SendAction(const char* action_name,const char *service_type,
                    std::list<std::pair<const char*, const char*>> arguments)
{
    /* Create action */
    IXML_Document *action = UpnpMakeAction(action_name, service_type, 0, nullptr);

    /* Add argument to action */
    for (std::pair<const char*, const char*> arg : arguments) {
      const char *arg_name, *arg_val;
      arg_name = arg.first;
      arg_val  = arg.second;
      UpnpAddToAction(&action, action_name, service_type, arg_name, arg_val);
    }

    /* Get the controlURL of the service */
    last_error = 0;
    char *control_url = getServiceURL(service_type, "controlURL");
    if (control_url == nullptr)
    {
        /* the device didn't answer (its error is kept: getServiceURL), or
         * has no such service; without a URL, libupnp would only say
         * "invalid parameter", which hides that the device is gone */
        ixmlDocument_free(action);
        if (last_error == 0)
            last_error = UPNP_E_INVALID_PARAM;
        msg_Err(parent, "Unable to send action: %s (no control URL, %d: %s)",
                action_name, last_error, UpnpGetErrorMessage(last_error));
        return nullptr;
    }

    /* Send action */
    IXML_Document *response = nullptr;
    int ret = UpnpSendAction(handle, control_url, service_type,
                                    nullptr, action, &response);

    /* Free action */
    ixmlDocument_free(action);
    free(control_url);

    if (ret != UPNP_E_SUCCESS) {
        char *printed = response ? ixmlPrintDocument(response) : nullptr;
        msg_Err(parent, "Unable to send action: %s (%d: %s) response: %s",
                action_name, ret, UpnpGetErrorMessage(ret),
                printed ? printed : "none");
        free(printed);
        if (response) ixmlDocument_free(response);
        last_error = ret;
        return nullptr;
    }

    last_error = 0;
    return response;
}

std::string MediaRenderer::GetTransportState()
{
    std::list<std::pair<const char*, const char*>> arg_list = {
        {"InstanceID", AVTransportID.c_str()},
    };

    IXML_Document *p_response = SendAction("GetTransportInfo", AV_TRANSPORT_SERVICE_TYPE, arg_list);
    if (!p_response)
        return std::string();
    const std::string state = responseValue(p_response, "CurrentTransportState");
    ixmlDocument_free(p_response);
    return state;
}

int MediaRenderer::Play(const char *speed)
{
    std::list<std::pair<const char*, const char*>> arg_list = {
        {"InstanceID", AVTransportID.c_str()},
        {"Speed", speed},
    };

    IXML_Document *p_response = SendAction("Play", AV_TRANSPORT_SERVICE_TYPE, arg_list);
    if(!p_response)
    {
        return VLC_EGENERIC;
    }
    ixmlDocument_free(p_response);
    return VLC_SUCCESS;
}

int MediaRenderer::Stop()
{
    std::list<std::pair<const char*, const char*>> arg_list = {
        {"InstanceID", AVTransportID.c_str()},
    };

    IXML_Document *p_response = SendAction("Stop", AV_TRANSPORT_SERVICE_TYPE, arg_list);
    if(!p_response)
    {
        return VLC_EGENERIC;
    }
    ixmlDocument_free(p_response);
    return VLC_SUCCESS;
}

std::vector<protocol_info_t> MediaRenderer::GetProtocolInfo()
{
    std::string protocol_csv;
    std::vector<protocol_info_t> supported_protocols;
    std::list<std::pair<const char*, const char*>> arg_list;

    IXML_Document *response = SendAction("GetProtocolInfo",
                                CONNECTION_MANAGER_SERVICE_TYPE, arg_list);
    if(!response)
    {
        return supported_protocols;
    }

    // Get the CSV list of protocols/profiles supported by the device
    IXML_NodeList *protocol_list = ixmlDocument_getElementsByTagName( response , "Sink" );
    if( protocol_list )
    {
        IXML_Node* protocol_node = ixmlNodeList_item( protocol_list, 0 );
        if ( protocol_node )
        {
            IXML_Node* p_text_node = ixmlNode_getFirstChild( protocol_node );
            if ( p_text_node )
            {
                protocol_csv.assign(ixmlNode_getNodeValue( p_text_node ));
            }
        }
        ixmlNodeList_free( protocol_list);
    }
    ixmlDocument_free(response);

    msg_Dbg(parent, "Device supports protocols: %s", protocol_csv.c_str());
    // parse the CSV list
    // format: <transportProtocol>:<network>:<mime>:<additionalInfo>
    std::vector<std::string> protocols = split(protocol_csv, ',');
    for (std::string protocol : protocols ) {
        std::vector<std::string> protocol_info = split(protocol, ':');

        // We only support http transport for now.
        if (protocol_info.size() == 4 && protocol_info.at(0) == "http-get")
        {
            protocol_info_t proto;

            // Get the DLNA profile name
            std::string profile_name;
            std::string tag = "DLNA.ORG_PN=";
            std::size_t index = protocol_info.at(3).find(tag);

            if (protocol_info.at(3) == "*")
            {
               profile_name = "*";
            }
            else if (index != std::string::npos)
            {
                std::size_t end = protocol_info.at(3).find(';', index + 1);
                int start = index + tag.length() - 1;
                int length = end - start;
                profile_name = protocol_info.at(3).substr(start, length);
            }

            // Match our supported profiles to device profiles
            for (dlna_profile_t profile : dlna_profile_list) {
                if (protocol_info.at(2) == profile.mime
                        && (profile_name == profile.name || profile_name == "*"))
                {
                    proto.profile = std::move(profile);
                    supported_protocols.push_back(proto);
                    // we do not break here to account for wildcards
                    // as protocolInfo's fourth field aka <additionalInfo>
                }
            }
        }
    }

    msg_Dbg( parent , "Got %zu supported profiles", supported_protocols.size() );
    return supported_protocols;
}

int MediaRenderer::PrepareForConnection(const char* protocol_str)
{
    std::list<std::pair<const char*, const char*>> arg_list = {
        { "PeerConnectionID", "-1" },
        { "PeerConnectionManager", "" },
        { "Direction", "Input" },
        { "RemoteProtocolInfo", protocol_str },
    };

    IXML_Document *response = SendAction("PrepareForConnection",
                                    CONNECTION_MANAGER_SERVICE_TYPE, arg_list);
    if(!response)
    {
        return VLC_EGENERIC;
    }

    msg_Dbg(parent, "PrepareForConnection response: %s",
                                    ixmlPrintDocument(response));

    IXML_NodeList *node_list =
            ixmlDocument_getElementsByTagName(response , "ConnectionID");
    if (node_list)
    {
        IXML_Node* node = ixmlNodeList_item(node_list, 0);
        if (node)
        {
            IXML_Node* p_text_node = ixmlNode_getFirstChild(node);
            if (p_text_node)
            {
                ConnectionID.assign(ixmlNode_getNodeValue(p_text_node));
            }
        }
        ixmlNodeList_free(node_list);
    }

    node_list =
            ixmlDocument_getElementsByTagName(response , "AVTransportID");
    if (node_list)
    {
        IXML_Node* node = ixmlNodeList_item(node_list, 0);
        if (node)
        {
            IXML_Node* p_text_node = ixmlNode_getFirstChild(node);
            if (p_text_node)
            {
                AVTransportID.assign(ixmlNode_getNodeValue(p_text_node));
            }
        }
        ixmlNodeList_free(node_list);
    }

    node_list =
            ixmlDocument_getElementsByTagName(response , "RcsID");
    if (node_list)
    {
        IXML_Node* node = ixmlNodeList_item(node_list, 0);
        if (node)
        {
            IXML_Node* p_text_node = ixmlNode_getFirstChild(node);
            if (p_text_node)
            {
                RcsID.assign(ixmlNode_getNodeValue(p_text_node));
            }
        }
        ixmlNodeList_free(node_list);
    }

    ixmlDocument_free(response);
    return VLC_SUCCESS;
}

int MediaRenderer::ConnectionComplete()
{
    std::list<std::pair<const char*, const char*>> arg_list = {
        {"ConnectionID", ConnectionID.c_str()},
    };

    IXML_Document *p_response = SendAction("ConnectionComplete",
                                    CONNECTION_MANAGER_SERVICE_TYPE, arg_list);
    if(!p_response)
    {
        return VLC_EGENERIC;
    }

    ixmlDocument_free(p_response);
    return VLC_SUCCESS;
}

int MediaRenderer::SetAVTransportURI(const char* uri, const protocol_info_t& proto)
{
    static const char didl[] =
        "<DIDL-Lite "
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
        "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" "
        "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" "
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
        "<item id=\"f-0\" parentID=\"0\" restricted=\"0\">"
        "<dc:title>%s</dc:title>"
        "<upnp:class>%s</upnp:class>"
        "<res protocolInfo=\"%s\">%s</res>"
        "</item>"
        "</DIDL-Lite>";

    bool audio = proto.profile.media == DLNA_CLASS_AUDIO;
    std::string dlna_protocol = dlna_write_protocol_info(proto);

    char *meta_data;
    if (asprintf(&meta_data, didl,
                audio ? "Audio" : "Video",
                audio ? "object.item.audioItem" : "object.item.videoItem",
                dlna_protocol.c_str(),
                uri) < 0) {
        return VLC_ENOMEM;
    }

    msg_Dbg(parent, "didl: %s", meta_data);
    std::list<std::pair<const char*, const char*>> arg_list = {
        {"InstanceID", AVTransportID.c_str()},
        {"CurrentURI", uri},
        {"CurrentURIMetaData", meta_data},
    };

    IXML_Document *p_response = SendAction("SetAVTransportURI",
                                    AV_TRANSPORT_SERVICE_TYPE, arg_list);

    free(meta_data);
    if(!p_response)
    {
        return VLC_EGENERIC;
    }
    ixmlDocument_free(p_response);
    return VLC_SUCCESS;
}

static void *
Add(sout_stream_t *p_stream, const es_format_t *p_fmt, const char *es_id)
{
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );

    if (!p_sys->b_supports_video)
    {
        if (p_fmt->i_cat != AUDIO_ES)
            return nullptr;
    }

    vlc::threads::mutex_locker feed( p_sys->feed_lock );
    sout_stream_id_sys_t *p_sys_id = (sout_stream_id_sys_t *)malloc(sizeof(sout_stream_id_sys_t));
    if(p_sys_id != nullptr)
    {
        es_format_Copy(&p_sys_id->fmt, p_fmt);
        p_sys_id->es_id = es_id;
        p_sys_id->p_sub_id = nullptr;
        p_sys_id->p_display_id = nullptr;
        p_sys_id->fresh = p_sys->retime;
        p_sys->streams.push_back(p_sys_id);

        /* in the picture here too */
        if (p_sys->p_display != nullptr
         && (p_fmt->i_cat == VIDEO_ES || p_fmt->i_cat == SPU_ES))
            p_sys_id->p_display_id = sout_StreamIdAdd(p_sys->p_display, &p_sys_id->fmt,
                                                      p_sys_id->es_id);

        /* Another subtitle while a converted video plays (the user chose
         * one): into the running conversion, which blends it. A new chain
         * would be a new stream, and the TV would have to start again
         * (some stop there). */
        if (p_fmt->i_cat == SPU_ES && p_sys->p_out != nullptr && p_sys->chain_soverlay)
        {
            p_sys_id->p_sub_id = static_cast<sout_stream_id_sys_t *>(
                sout_StreamIdAdd(p_sys->p_out, &p_sys_id->fmt, p_sys_id->es_id));
            if (p_sys_id->p_sub_id != nullptr)
            {
                msg_Dbg(p_stream, "subtitle '%4.4s' blended into the running stream",
                        (const char *)&p_fmt->i_codec);
                p_sys->out_streams.push_back(p_sys_id);
                return p_sys_id;
            }
        }
        /* a subtitle without a picture to draw it on changes nothing that
         * is sent: no new stream (the renderer would start again) */
        bool has_video = false;
        for (const sout_stream_id_sys_t *other : p_sys->streams)
            has_video = has_video || other->fmt.i_cat == VIDEO_ES;
        if (p_fmt->i_cat != SPU_ES || has_video)
        {
            p_sys->es_changed = true;
            p_sys->es_changed_at = vlc_tick_now();
        }
    }
    return p_sys_id;
}

static int Send(sout_stream_t *p_stream, void *id,
                block_t *p_buffer)
{
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );
    sout_stream_id_sys_t *id_sys = static_cast<sout_stream_id_sys_t*>( id );
    vlc::threads::mutex_locker feed( p_sys->feed_lock );

    if ( p_sys->p_ended != nullptr )
    {
        bool done;
        {
            vlc::threads::mutex_locker locker( p_sys->ctl_lock );
            done = p_sys->ended_done;
        }
        if ( done )
        {
            sout_StreamChainDelete( p_sys->p_ended, nullptr );
            p_sys->p_ended = nullptr;
        }
    }

    /* converted, the device not reading it as it was (Fail) */
    if ( p_sys->fallback_request.exchange( false ) )
    {
        p_sys->force_transcode = true;
        p_sys->es_changed = true;
    }

    /* the chain for the tracks there are now (the first one once they stay
     * the same a moment), the picture here as asked */
    if ( ( p_sys->p_out != nullptr || p_sys->es_changed_at == VLC_TICK_INVALID
        || vlc_tick_now() - p_sys->es_changed_at >= VLC_TICK_FROM_MS(300) )
      && p_sys->UpdateOutput( p_stream ) != VLC_SUCCESS )
    {
        block_Release( p_buffer );
        return VLC_EGENERIC;
    }
    p_sys->UpdateDisplay( p_stream );

    p_sys->Hold( p_stream, id_sys, p_buffer );
    p_sys->Feed( p_stream, false );
    return VLC_SUCCESS;
}

/* A seek: what is held of the track is dropped (the media goes on
 * elsewhere), and what comes next follows what was sent (Retime). The
 * renderer goes on with what it has, then the new part: no new stream. */
static void Flush( sout_stream_t *p_stream, void *id )
{
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );
    sout_stream_id_sys_t *id_sys = static_cast<sout_stream_id_sys_t*>( id );
    vlc::threads::mutex_locker feed( p_sys->feed_lock );

    for ( auto it = p_sys->held.begin(); it != p_sys->held.end(); )
        if ( it->id == id_sys )
        {
            block_Release( it->block );
            it = p_sys->held.erase( it );
        }
        else
            ++it;
    id_sys->fresh = true;
    if ( !p_sys->retime )
    {
        p_sys->retime = true;
        p_sys->retime_since = vlc_tick_now();
    }

    if ( p_sys->p_out != nullptr && id_sys->p_sub_id != nullptr )
        sout_StreamFlush( p_sys->p_out, id_sys->p_sub_id );
}

static void Del(sout_stream_t *p_stream, void *_id)
{
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );
    sout_stream_id_sys_t *id = static_cast<sout_stream_id_sys_t *>( _id );
    vlc::threads::mutex_locker feed( p_sys->feed_lock );

    /* the end of the sound or the picture (the media ends, or another
     * track): what is held goes */
    if ( id->fmt.i_cat != SPU_ES )
    {
        if ( p_sys->retime )
            p_sys->Retime( p_stream, true );
        p_sys->Feed( p_stream, true );
    }
    for ( auto it = p_sys->held.begin(); it != p_sys->held.end(); )
        if ( it->id == id )
        {
            block_t *block = it->block;
            it = p_sys->held.erase( it );
            p_sys->Release( p_stream, id, block );
        }
        else
            ++it;

    for (auto it = p_sys->streams.begin(); it != p_sys->streams.end(); ++it)
    {
        sout_stream_id_sys_t *p_sys_id = *it;
        if ( p_sys_id == id )
        {
            if ( p_sys_id->p_display_id != nullptr )
                sout_StreamIdDel( p_sys->p_display, p_sys_id->p_display_id );
            if ( p_sys_id->p_sub_id != nullptr )
            {
                sout_StreamIdDel( p_sys->p_out, p_sys_id->p_sub_id );
                for (auto out_it = p_sys->out_streams.begin();
                     out_it != p_sys->out_streams.end(); ++out_it)
                {
                    if (*out_it == id)
                    {
                        p_sys->out_streams.erase(out_it);
                        break;
                    }
                }
            }

            es_format_Clean( &p_sys_id->fmt );
            free( p_sys_id );
            p_sys->streams.erase( it );
            break;
        }
    }

    /* (a track that wasn't in the chain, e.g. a subtitle of a sound: the
     * chain kept for the renderer to finish stays as it is) */
    if (p_sys->out_streams.empty() && p_sys->p_out != nullptr)
    {
        p_sys->stopSoutChain(p_stream, true);
        vlc::threads::mutex_locker locker( p_sys->ctl_lock );
        /* until it shows the end of what was sent */
        const vlc_tick_t now = vlc_tick_now();
        vlc_tick_t left = VLC_TICK_FROM_SEC(4);
        if ( p_sys->latency != VLC_TICK_INVALID && p_sys->anchor_date != VLC_TICK_INVALID
          && p_sys->sent_end != VLC_TICK_INVALID )
            left = p_sys->sent_end - ( p_sys->BaseTs( now ) - p_sys->latency );
        p_sys->stop_at = p_sys->stop_now ? now
                       : now + std::clamp<vlc_tick_t>( left + VLC_TICK_FROM_MS(500),
                                                       VLC_TICK_FROM_SEC(1),
                                                       VLC_TICK_FROM_SEC(15) );
        msg_Dbg( p_stream, "the end: the renderer stops in %" PRId64 " ms, once it showed it",
                 MS_FROM_VLC_TICK( p_sys->stop_at - now ) );
        p_sys->ctl_wait.signal();
    }
    if (p_sys->streams.empty())
    {
        p_sys->StopDisplay();
        p_sys->ResetTimeline();
    }
}

static void CloseSout( sout_stream_t *p_stream )
{
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );

    if ( p_sys->feed_running )
    {
        {
            vlc::threads::mutex_locker locker( p_sys->feed_lock );
            p_sys->feed_stop = true;
            p_sys->feed_wait.signal();
        }
        vlc_join( p_sys->feed_thread, nullptr );
    }
    if ( p_sys->libvlc != nullptr )
        for ( const char *psz_var : renderer_vars )
            var_DelCallback( p_sys->libvlc, psz_var, RendererVarChanged, p_sys );
    if ( p_sys->ctl_running )
    {
        {
            vlc::threads::mutex_locker locker( p_sys->ctl_lock );
            p_sys->ctl_stop = true;
            p_sys->ctl_wait.signal();
        }
        vlc_join( p_sys->ctl_thread, nullptr );
    }
    /* casting stops (another device, or here): now (not when it does not
     * answer: each round trip would wait for it) */
    if ( !p_sys->unreachable && ( p_sys->playing || p_sys->stop_at != VLC_TICK_INVALID ) )
        p_sys->renderer->Stop();
    if ( p_sys->p_ended != nullptr )
        sout_StreamChainDelete( p_sys->p_ended, nullptr );
    p_sys->p_ended = nullptr;
    if ( p_sys->libvlc != nullptr && var_Type( p_sys->libvlc, "renderer-latency" ) )
        var_SetInteger( p_sys->libvlc, "renderer-latency", 0 );
    if ( p_sys->libvlc != nullptr && var_Type( p_sys->libvlc, "renderer-time" ) )
        var_SetInteger( p_sys->libvlc, "renderer-time", -1 );
    for ( const sout_stream_sys_t::held_block &h : p_sys->held )
        block_Release( h.block );
    p_sys->held.clear();
    p_sys->StopDisplay();

    if ( !p_sys->unreachable )
        p_sys->renderer->ConnectionComplete();
    p_sys->p_upnp->release();
    delete p_sys;
}

/* The renderer reads the stream as it plays: the input must go at the
 * speed of the media (otherwise it converts the whole media as fast as it
 * can, the TV misses most of it, and the media ends at once) */
static int Control(sout_stream_t *, int query, va_list args)
{
    switch (query)
    {
        case SOUT_STREAM_IS_SYNCHRONOUS:
            *va_arg(args, bool *) = true;
            return VLC_SUCCESS;
        default:
            return VLC_EGENERIC;
    }
}

static const struct sout_stream_operations ops = [] {
    struct sout_stream_operations ops {};
    ops.add = Add;
    ops.del = Del;
    ops.send = Send;
    ops.flush = Flush;
    ops.control = Control;
    ops.close = CloseSout;
    return ops;
}();

int OpenSout( vlc_object_t *p_this )
{
    sout_stream_t *p_stream = reinterpret_cast<sout_stream_t*>(p_this);
    sout_stream_sys_t *p_sys = nullptr;

    config_ChainParse(p_stream, SOUT_CFG_PREFIX, ppsz_sout_options, p_stream->p_cfg);

    int http_port = var_InheritInteger(p_stream, SOUT_CFG_PREFIX "http-port");
    bool b_supports_video = var_GetBool(p_stream, SOUT_CFG_PREFIX "video");
    auto base_url = vlc::wrap_cptr(
        var_GetNonEmptyString(p_stream, SOUT_CFG_PREFIX "base_url"));
    auto device_url = vlc::wrap_cptr(
        var_GetNonEmptyString(p_stream, SOUT_CFG_PREFIX "url"));
    if ( device_url == nullptr)
    {
        msg_Err( p_stream, "missing Url" );
        goto error;
    }

    try {
        p_sys = new sout_stream_sys_t(http_port, b_supports_video);
    }
    catch ( const std::exception& ex ) {
        msg_Err( p_stream, "Failed to instantiate sout_stream_sys_t: %s", ex.what() );
        return VLC_EGENERIC;
    }

    p_sys->p_upnp = UpnpInstanceWrapper::get( p_this );
    if ( !p_sys->p_upnp )
        goto error;
    try {
        p_sys->renderer = std::make_shared<MediaRenderer>(p_stream,
                            p_sys->p_upnp, base_url.get(), device_url.get());
    }
    catch ( const std::bad_alloc& ) {
        msg_Err( p_stream, "Failed to create a MediaRenderer");
        p_sys->p_upnp->release();
        goto error;
    }

    p_sys->device_protocols = p_sys->renderer->GetProtocolInfo();

    p_stream->p_sys = p_sys;
    p_stream->ops = &ops;

    /* the interface's controls (pause, volume, mute), when it has them */
    {
        vlc_object_t *libvlc = VLC_OBJECT( vlc_object_instance( p_stream ) );
        if ( var_Type( libvlc, "renderer-paused" ) && var_Type( libvlc, "renderer-display" ) )
        {
            p_sys->libvlc = libvlc;
            p_sys->want_paused = var_GetBool( libvlc, "renderer-paused" );
            if ( p_sys->want_paused )
                p_sys->pause_date = vlc_tick_now();
            p_sys->display_wanted = var_GetBool( libvlc, "renderer-display" );
            for ( const char *psz_var : renderer_vars )
                var_AddCallback( libvlc, psz_var, RendererVarChanged, p_sys );
        }
    }
    {
        vlc::threads::mutex_locker locker( p_sys->ctl_lock );
        if ( NetworkError( p_sys->renderer->last_error ) )
            p_sys->Fail( p_stream, "unreachable", p_sys->renderer->last_error );
        else
            p_sys->Report( RENDERER_CONNECTING, "" );
    }
    p_sys->ctl_running = !vlc_clone( &p_sys->ctl_thread, ControlThread, p_stream );
    if ( !p_sys->ctl_running )
        msg_Err( p_stream, "cannot start the renderer control thread" );
    p_sys->feed_running = !vlc_clone( &p_sys->feed_thread, FeedThread, p_stream );
    if ( !p_sys->feed_running )
        msg_Err( p_stream, "cannot start the feeding thread" );

    return VLC_SUCCESS;

error:
    delete p_sys;
    return VLC_EGENERIC;
}

}
