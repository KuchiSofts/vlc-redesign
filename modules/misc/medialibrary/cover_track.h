/*****************************************************************************
 * cover_track.h: a song's picture kept as a video track
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VLC_REDESIGN_ML_COVER_TRACK_H
#define VLC_REDESIGN_ML_COVER_TRACK_H

/*
 * VLC Redesign: some tools write a song's picture into the file as a video
 * track (an .m4a with its thumbnail as a Motion JPEG or PNG stream of one or
 * a few frames) instead of as cover art. The media library takes any file
 * with a video track for a video, so those songs were listed under Videos
 * and Continue Watching, and never under Music.
 *
 * A track is a cover, not a video, when the three hold together:
 *   - the file is named as audio (.m4a, .mp3, .flac, .mka ...);
 *   - the track's codec is one of single pictures (JPEG, PNG, BMP, GIF ...);
 *   - the file has an audio track.
 * A camera's Motion JPEG clip (.avi, .mov) stays a video: its name is one of
 * videos. A film in an .ogg file stays a video: Theora is no picture codec.
 *
 * Plain C++, no VLC type: the test (extras/juxtopposed/cover-track.cpp)
 * includes this file alone. The codecs are the four characters VLC names
 * them with (vlc_fourcc_to_char, what the media library stores).
 */

#include <cstring>
#include <string>

namespace redesign_cover
{

/** The extension of the file an MRL or a path names, in lower case; empty when it has none */
inline std::string extensionOf( const char* mrl )
{
    if ( mrl == nullptr )
        return {};
    std::string name( mrl );
    // (what follows the file's name in an address)
    const auto end = name.find_first_of( "?#" );
    if ( end != std::string::npos )
        name.erase( end );
    const auto slash = name.find_last_of( "/\\" );
    if ( slash != std::string::npos )
        name.erase( 0, slash + 1 );
    const auto dot = name.rfind( '.' );
    if ( dot == std::string::npos || dot == 0 || dot + 1 == name.size() )
        return {};
    std::string ext = name.substr( dot + 1 );
    for ( auto& c : ext )
        if ( c >= 'A' && c <= 'Z' )
            c = static_cast<char>( c - 'A' + 'a' );
    return ext;
}

/** A file named as audio: the extensions of EXTENSIONS_AUDIO_CSV (include/vlc_interface.h) */
inline bool isAudioName( const char* mrl )
{
    static const char* const extensions[] = {
        "3ga", "669", "a52", "aac", "ac3", "adt", "adts", "aif", "aifc", "aiff", "alac",
        "amb", "amr", "aob", "ape", "au", "awb", "caf", "dts", "dsf", "dff", "flac", "it", "kar",
        "m4a", "m4b", "m4p", "m5p", "mka", "mlp", "mod", "mpa", "mp1", "mp2", "mp3", "mpc", "mpga", "mus",
        "oga", "ogg", "oma", "opus", "qcp", "ra", "rmi", "s3m", "sid", "spx", "tak", "thd", "tta",
        "voc", "vqf", "w64", "wav", "wma", "wv", "xa", "xm"
    };
    const std::string ext = extensionOf( mrl );
    if ( ext.empty() )
        return false;
    for ( const char* e : extensions )
        if ( ext == e )
            return true;
    return false;
}

/** A codec of single pictures, by the four characters VLC names it with */
inline bool isPictureCodec( const char* fourcc )
{
    static const char* const codecs[] = {
        "MJPG", "mjpb", "jpeg", "MJLS", "JP2K", "png ", "bmp ", "gif ", "tiff", "tga ", "WEBP", "svg "
    };
    if ( fourcc == nullptr )
        return false;
    for ( const char* c : codecs )
        if ( std::memcmp( fourcc, c, 4 ) == 0 )
            return true;
    return false;
}

/** Whether a video track of this codec, in this file, is the song's cover and not a video */
inline bool isCoverTrack( const char* mrl, const char* fourcc, bool hasAudioTrack )
{
    return hasAudioTrack && isPictureCodec( fourcc ) && isAudioName( mrl );
}

}

#endif
