/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/*****************************************************************************
 * subtitle_cues_parser.hpp: The times of the lines of a subtitle file
 *****************************************************************************/
#ifndef VLC_QT_SUBTITLE_CUES_PARSER_HPP_
#define VLC_QT_SUBTITLE_CUES_PARSER_HPP_

// Plain C++ (no Qt, no VLC): built alone by its test too
// (extras/juxtopposed/subtitle-cues-parser.cpp)
#include <cstdint>
#include <string>
#include <vector>

namespace subtitlecues {

struct Cue
{
    int64_t startMs = 0;
    int64_t endMs = 0;
    // the words of the line, without their styling (UTF-8 when the file
    // is UTF-8 or UTF-16; the bytes of the file otherwise), lines by '\n'
    std::string text;
};

// No more is read: a subtitle file of a film is well under a megabyte
constexpr size_t maxFileBytes = 32u * 1024u * 1024u;
constexpr size_t maxCues = 200000;
constexpr size_t maxTextBytes = 240;

// The lines of a text subtitle file: SubRip (SRT), WebVTT, ASS/SSA,
// MicroDVD (at the frame rate of its first line, else microDvdFps: VLC's
// 25 by default), SubViewer and SBV, SAMI, and the times of VobSub (.idx:
// the stream of that index, the first one for -1; their ends are the next
// start, at most 4 s later). Sorted by start, an empty list for the others.
std::vector<Cue> parse(const std::string &bytes, double microDvdFps = 25.0, int vobSubIndex = -1);

}

#endif
