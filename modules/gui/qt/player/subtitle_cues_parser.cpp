/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/*****************************************************************************
 * subtitle_cues_parser.cpp: The times of the lines of a subtitle file
 *****************************************************************************/
#include "subtitle_cues_parser.hpp"

#include <algorithm>
#include <string_view>

namespace subtitlecues {
namespace {

using View = std::string_view;

// the end of a line known only by the next one (MicroDVD without its end,
// SAMI, VobSub): at most this long
constexpr int64_t openEndMs = 4000;

bool isDigit(char c) { return c >= '0' && c <= '9'; }
bool isSpace(char c) { return c == ' ' || c == '\t'; }
char lower(char c) { return c >= 'A' && c <= 'Z' ? char(c - 'A' + 'a') : c; }

View trim(View s)
{
    while (!s.empty() && (isSpace(s.front()) || s.front() == '\r' || s.front() == '\n'))
        s.remove_prefix(1);
    while (!s.empty() && (isSpace(s.back()) || s.back() == '\r' || s.back() == '\n'))
        s.remove_suffix(1);
    return s;
}

bool startsWithNoCase(View s, View prefix)
{
    if (s.size() < prefix.size())
        return false;
    for (size_t i = 0; i < prefix.size(); ++i)
        if (lower(s[i]) != lower(prefix[i]))
            return false;
    return true;
}

size_t findNoCase(View s, View needle, size_t from = 0)
{
    if (needle.empty() || s.size() < needle.size())
        return View::npos;
    for (size_t i = from; i + needle.size() <= s.size(); ++i)
        if (startsWithNoCase(s.substr(i), needle))
            return i;
    return View::npos;
}

void appendUtf8(std::string &out, uint32_t c)
{
    if (c < 0x80) {
        out += char(c);
    } else if (c < 0x800) {
        out += char(0xC0 | (c >> 6));
        out += char(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        out += char(0xE0 | (c >> 12));
        out += char(0x80 | ((c >> 6) & 0x3F));
        out += char(0x80 | (c & 0x3F));
    } else {
        out += char(0xF0 | (c >> 18));
        out += char(0x80 | ((c >> 12) & 0x3F));
        out += char(0x80 | ((c >> 6) & 0x3F));
        out += char(0x80 | (c & 0x3F));
    }
}

std::string fromUtf16(const std::string &bytes, size_t start, bool littleEndian)
{
    const auto unit = [&](size_t i) -> uint32_t {
        const uint32_t a = uint8_t(bytes[i]), b = uint8_t(bytes[i + 1]);
        return littleEndian ? (a | (b << 8)) : ((a << 8) | b);
    };
    std::string out;
    out.reserve(bytes.size() / 2);
    for (size_t i = start; i + 1 < bytes.size(); i += 2) {
        uint32_t c = unit(i);
        if (c >= 0xD800 && c < 0xDC00 && i + 3 < bytes.size()) {
            const uint32_t low = unit(i + 2);
            if (low >= 0xDC00 && low < 0xE000) {
                c = 0x10000 + ((c - 0xD800) << 10) + (low - 0xDC00);
                i += 2;
            } else {
                c = 0xFFFD;
            }
        } else if (c >= 0xD800 && c < 0xE000) {
            c = 0xFFFD;
        }
        appendUtf8(out, c);
    }
    return out;
}

// UTF-16 (with its mark, or plain text with every other byte zero) to
// UTF-8; the mark of UTF-8 goes
std::string normalize(const std::string &bytes)
{
    const size_t n = bytes.size();
    const auto at = [&](size_t i) { return uint8_t(bytes[i]); };
    if (n >= 3 && at(0) == 0xEF && at(1) == 0xBB && at(2) == 0xBF)
        return bytes.substr(3);
    if (n >= 2 && at(0) == 0xFF && at(1) == 0xFE)
        return fromUtf16(bytes, 2, true);
    if (n >= 2 && at(0) == 0xFE && at(1) == 0xFF)
        return fromUtf16(bytes, 2, false);
    if (n >= 4 && at(0) != 0 && at(1) == 0 && at(2) != 0 && at(3) == 0)
        return fromUtf16(bytes, 0, true);
    if (n >= 4 && at(0) == 0 && at(1) != 0 && at(2) == 0 && at(3) != 0)
        return fromUtf16(bytes, 0, false);
    return bytes;
}

std::vector<View> splitLines(const std::string &text)
{
    std::vector<View> lines;
    const View all(text);
    size_t start = 0;
    for (size_t i = 0; i < all.size(); ++i) {
        if (all[i] != '\n' && all[i] != '\r')
            continue;
        lines.push_back(all.substr(start, i - start));
        if (all[i] == '\r' && i + 1 < all.size() && all[i + 1] == '\n')
            ++i;
        start = i + 1;
    }
    if (start < all.size())
        lines.push_back(all.substr(start));
    return lines;
}

// digits at pos (the first 9 count; the others are skipped)
bool readNumber(View s, size_t &pos, int64_t &value, int &digits)
{
    value = 0;
    digits = 0;
    size_t p = pos;
    while (p < s.size() && isDigit(s[p])) {
        if (digits < 9) {
            value = value * 10 + (s[p] - '0');
            ++digits;
        }
        ++p;
    }
    if (p == pos)
        return false;
    pos = p;
    return true;
}

int64_t fractionMs(int64_t value, int digits)
{
    switch (digits) {
    case 1: return value * 100;
    case 2: return value * 10;
    case 3: return value;
    default: {
        for (int i = 3; i < digits; ++i)
            value /= 10;
        return value;
    }
    }
}

// "[h:]m:s[.,fraction]" (SRT, WebVTT, ASS, SubViewer), and the
// "h:m:s:ms" of VobSub; a '-' before it for the delays of VobSub
bool parseTime(View s, size_t &pos, int64_t &ms)
{
    size_t p = pos;
    while (p < s.size() && isSpace(s[p]))
        ++p;
    bool negative = false;
    if (p < s.size() && s[p] == '-') {
        negative = true;
        ++p;
    }
    int64_t groups[4] = {};
    int count = 0;
    int digits = 0;
    for (;;) {
        if (!readNumber(s, p, groups[count], digits))
            return false;
        ++count;
        if (count < 4 && p + 1 < s.size() && s[p] == ':' && isDigit(s[p + 1])) {
            ++p;
            continue;
        }
        break;
    }
    int64_t fraction = 0;
    if (count == 4) {
        fraction = fractionMs(groups[3], digits);
        count = 3;
    } else if (p + 1 < s.size() && (s[p] == '.' || s[p] == ',') && isDigit(s[p + 1])) {
        ++p;
        int64_t value;
        readNumber(s, p, value, digits);
        fraction = fractionMs(value, digits);
    }
    if (count < 2)
        return false;
    const int64_t hours = count == 3 ? groups[0] : 0;
    const int64_t minutes = groups[count - 2];
    const int64_t seconds = groups[count - 1];
    if (seconds >= 60 || (count == 3 && minutes >= 60))
        return false;
    ms = ((hours * 60 + minutes) * 60 + seconds) * 1000 + fraction;
    if (negative)
        ms = -ms;
    pos = p;
    return true;
}

// the words of a line: no HTML-like tags, no {styling} of ASS and
// MicroDVD, its line breaks kept
std::string cleanText(View raw)
{
    std::string out;
    out.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        const char c = raw[i];
        if (c == '<') {
            const size_t close = raw.find('>', i);
            if (close != View::npos && close - i < 256) {
                const View tag = raw.substr(i + 1, close - i - 1);
                if (startsWithNoCase(tag, "br"))
                    out += '\n';
                i = close;
                continue;
            }
        } else if (c == '{') {
            const size_t close = raw.find('}', i);
            if (close != View::npos && close - i < 512) {
                i = close;
                continue;
            }
        } else if (c == '\\' && i + 1 < raw.size()) {
            const char next = raw[i + 1];
            if (next == 'N' || next == 'n') {
                out += '\n';
                ++i;
                continue;
            }
            if (next == 'h') {
                out += ' ';
                ++i;
                continue;
            }
        } else if (c == '&') {
            static const struct { View name; View value; } entities[] = {
                { "&amp;", "&" }, { "&lt;", "<" }, { "&gt;", ">" }, { "&quot;", "\"" },
                { "&apos;", "'" }, { "&#39;", "'" }, { "&nbsp;", " " },
            };
            bool replaced = false;
            for (const auto &entity : entities)
                if (startsWithNoCase(raw.substr(i), entity.name)) {
                    out += entity.value;
                    i += entity.name.size() - 1;
                    replaced = true;
                    break;
                }
            if (replaced)
                continue;
        } else if (c == '\r') {
            continue;
        } else if (c == '\t') {
            out += ' ';
            continue;
        }
        out += c;
    }

    // its lines trimmed, the empty ones gone
    std::string result;
    size_t start = 0;
    while (start <= out.size()) {
        size_t end = out.find('\n', start);
        if (end == std::string::npos)
            end = out.size();
        const View line = trim(View(out).substr(start, end - start));
        if (!line.empty()) {
            if (!result.empty())
                result += '\n';
            result.append(line.data(), line.size());
        }
        start = end + 1;
    }
    if (result.size() > maxTextBytes) {
        size_t cut = maxTextBytes;
        while (cut > 0 && (uint8_t(result[cut]) & 0xC0) == 0x80)
            --cut;
        result.resize(cut);
        result += "\xE2\x80\xA6"; // …
    }
    return result;
}

std::string replaceAll(View s, View from, View to)
{
    std::string out;
    size_t start = 0;
    for (size_t at; (at = findNoCase(s, from, start)) != View::npos; start = at + from.size()) {
        out.append(s.data() + start, at - start);
        out.append(to.data(), to.size());
    }
    out.append(s.data() + start, s.size() - start);
    return out;
}

void add(std::vector<Cue> &cues, int64_t start, int64_t end, std::string text)
{
    if (cues.size() >= maxCues || end < start || end < 0)
        return;
    cues.push_back({ std::max<int64_t>(0, start), end, std::move(text) });
}

// the lines known by their start only end at the next one
void closeOpenEnds(std::vector<Cue> &cues)
{
    std::stable_sort(cues.begin(), cues.end(),
                     [](const Cue &a, const Cue &b) { return a.startMs < b.startMs; });
    for (size_t i = 0; i < cues.size(); ++i) {
        if (cues[i].endMs >= cues[i].startMs)
            continue;
        int64_t end = cues[i].startMs + openEndMs;
        if (i + 1 < cues.size())
            end = std::min(end, std::max(cues[i].startMs, cues[i + 1].startMs));
        cues[i].endMs = end;
    }
}

// SubRip and WebVTT: "start --> end", then the text up to an empty line
std::vector<Cue> parseArrows(const std::vector<View> &lines)
{
    std::vector<Cue> cues;
    for (size_t i = 0; i < lines.size(); ++i) {
        const View line = lines[i];
        const size_t arrow = line.find("-->");
        if (arrow == View::npos)
            continue;
        size_t p = 0;
        int64_t start, end;
        if (!parseTime(line, p, start))
            continue;
        size_t q = arrow + 3;
        if (!parseTime(line, q, end))
            continue;
        std::string raw;
        while (i + 1 < lines.size() && !trim(lines[i + 1]).empty()
               && lines[i + 1].find("-->") == View::npos) {
            ++i;
            if (!raw.empty())
                raw += '\n';
            raw.append(lines[i].data(), lines[i].size());
        }
        add(cues, start, end, cleanText(raw));
    }
    return cues;
}

// ASS and SSA: the Dialogue lines of [Events], by the fields of its Format
std::vector<Cue> parseAss(const std::vector<View> &lines)
{
    std::vector<Cue> cues;
    size_t startField = 1, endField = 2, textField = 9;
    bool events = false;
    for (const View raw : lines) {
        const View line = trim(raw);
        if (!line.empty() && line.front() == '[') {
            events = startsWithNoCase(line, "[events]");
            continue;
        }
        if (events && startsWithNoCase(line, "format:")) {
            View names = line.substr(7);
            size_t index = 0, s = 9, e = 9, t = 9;
            bool hasStart = false, hasEnd = false, hasText = false;
            while (true) {
                const size_t comma = names.find(',');
                const View name = trim(names.substr(0, comma));
                if (startsWithNoCase(name, "start") && name.size() == 5) { s = index; hasStart = true; }
                else if (startsWithNoCase(name, "end") && name.size() == 3) { e = index; hasEnd = true; }
                else if (startsWithNoCase(name, "text") && name.size() == 4) { t = index; hasText = true; }
                if (comma == View::npos)
                    break;
                names.remove_prefix(comma + 1);
                ++index;
            }
            if (hasStart && hasEnd && hasText) {
                startField = s;
                endField = e;
                textField = t;
            }
            continue;
        }
        if (!startsWithNoCase(line, "dialogue:"))
            continue;
        View rest = line.substr(9);
        std::vector<View> fields;
        // the text is the last field: its commas are its own
        while (fields.size() < textField) {
            const size_t comma = rest.find(',');
            if (comma == View::npos)
                break;
            fields.push_back(rest.substr(0, comma));
            rest.remove_prefix(comma + 1);
        }
        if (fields.size() < textField || startField >= fields.size() || endField >= fields.size())
            continue;
        size_t p = 0, q = 0;
        int64_t start, end;
        if (!parseTime(fields[startField], p, start) || !parseTime(fields[endField], q, end))
            continue;
        add(cues, start, end, cleanText(rest));
    }
    return cues;
}

// MicroDVD: "{start frame}{end frame}text|text"; "{1}{1}23.976" first
// gives the frame rate
std::vector<Cue> parseMicroDvd(const std::vector<View> &lines, double fps)
{
    std::vector<Cue> cues;
    if (!(fps > 0))
        fps = 25.0;
    bool first = true;
    for (const View raw : lines) {
        const View line = trim(raw);
        if (line.size() < 4 || line[0] != '{')
            continue;
        size_t p = 1;
        int64_t startFrame, endFrame = -1;
        int digits;
        if (!readNumber(line, p, startFrame, digits) || p + 1 >= line.size() || line[p] != '}' || line[p + 1] != '{')
            continue;
        p += 2;
        if (p < line.size() && isDigit(line[p]) && !readNumber(line, p, endFrame, digits))
            continue;
        if (p >= line.size() || line[p] != '}')
            continue;
        const View text = line.substr(p + 1);
        if (first && startFrame == 1 && endFrame == 1) {
            first = false;
            const std::string number(trim(text));
            char *end = nullptr;
            const double value = strtod(number.c_str(), &end);
            if (end && *end == '\0' && value > 1 && value < 200) {
                fps = value;
                continue;
            }
        }
        first = false;
        const int64_t start = int64_t(startFrame * 1000.0 / fps);
        const int64_t end = endFrame >= 0 ? int64_t(endFrame * 1000.0 / fps) : -1;
        if (endFrame >= 0)
            add(cues, start, end, cleanText(replaceAll(text, "|", "\n")));
        else if (cues.size() < maxCues)
            cues.push_back({ start, -1, cleanText(replaceAll(text, "|", "\n")) });
    }
    closeOpenEnds(cues);
    return cues;
}

// SubViewer 2 and SBV: "start,end" alone on its line, then the text
std::vector<Cue> parseSubViewer(const std::vector<View> &lines)
{
    std::vector<Cue> cues;
    for (size_t i = 0; i < lines.size(); ++i) {
        const View line = trim(lines[i]);
        size_t p = 0;
        int64_t start, end;
        if (!parseTime(line, p, start) || p >= line.size() || line[p] != ',')
            continue;
        ++p;
        if (!parseTime(line, p, end) || !trim(line.substr(p)).empty())
            continue;
        std::string raw;
        while (i + 1 < lines.size() && !trim(lines[i + 1]).empty()) {
            ++i;
            if (!raw.empty())
                raw += '\n';
            raw.append(lines[i].data(), lines[i].size());
        }
        add(cues, start, end, cleanText(replaceAll(raw, "[br]", "\n")));
    }
    return cues;
}

// SAMI: each <SYNC Start=ms> shows its text until the next one (an empty
// one clears it)
std::vector<Cue> parseSami(View text)
{
    std::vector<Cue> cues;
    struct Sync { int64_t start; std::string text; };
    std::vector<Sync> syncs;
    size_t at = findNoCase(text, "<sync");
    while (at != View::npos && syncs.size() < maxCues) {
        const size_t close = text.find('>', at);
        if (close == View::npos)
            break;
        const View tag = text.substr(at, close - at);
        size_t next = findNoCase(text, "<sync", close);
        const size_t body = findNoCase(text, "</body", close);
        const size_t stop = std::min(next, body);
        const size_t key = findNoCase(tag, "start");
        if (key != View::npos) {
            size_t p = key + 5;
            while (p < tag.size() && (isSpace(tag[p]) || tag[p] == '=' || tag[p] == '"' || tag[p] == '\''))
                ++p;
            int64_t start;
            int digits;
            if (readNumber(tag, p, start, digits)) {
                const View content = text.substr(close + 1, stop == View::npos ? View::npos : stop - close - 1);
                syncs.push_back({ start, cleanText(content) });
            }
        }
        at = next;
    }
    std::stable_sort(syncs.begin(), syncs.end(),
                     [](const Sync &a, const Sync &b) { return a.start < b.start; });
    for (size_t i = 0; i < syncs.size(); ++i) {
        if (syncs[i].text.empty())
            continue;
        int64_t end = syncs[i].start + openEndMs;
        if (i + 1 < syncs.size())
            end = std::max(syncs[i].start, syncs[i + 1].start);
        add(cues, syncs[i].start, end, std::move(syncs[i].text));
    }
    return cues;
}

// VobSub index: "id: en, index: 0", then its "timestamp: 00:00:01:101,
// filepos: ..." (and "delay:" lines moving the next ones)
std::vector<Cue> parseVobSubIndex(const std::vector<View> &lines, int index)
{
    std::vector<Cue> cues;
    bool inTrack = false;
    bool chosen = false;
    int64_t delay = 0;
    for (const View raw : lines) {
        const View line = trim(raw);
        if (startsWithNoCase(line, "id:")) {
            const size_t key = findNoCase(line, "index:");
            int64_t value = -1;
            if (key != View::npos) {
                size_t p = key + 6;
                while (p < line.size() && isSpace(line[p]))
                    ++p;
                int digits;
                if (!readNumber(line, p, value, digits))
                    value = -1;
            }
            inTrack = index < 0 ? !chosen : value == index;
            chosen = chosen || inTrack;
            delay = 0;
        } else if (inTrack && startsWithNoCase(line, "delay:")) {
            size_t p = 6;
            int64_t value;
            if (parseTime(line, p, value))
                delay += value;
        } else if (inTrack && startsWithNoCase(line, "timestamp:")) {
            size_t p = 10;
            int64_t start;
            if (parseTime(line, p, start) && start + delay >= 0 && cues.size() < maxCues)
                cues.push_back({ start + delay, -1, {} });
        }
    }
    closeOpenEnds(cues);
    return cues;
}

}

std::vector<Cue> parse(const std::string &bytes, double microDvdFps, int vobSubIndex)
{
    if (bytes.empty() || bytes.size() > maxFileBytes)
        return {};
    const std::string text = normalize(bytes);
    const std::vector<View> lines = splitLines(text);
    const View head = View(text).substr(0, 8192);

    std::vector<Cue> cues;
    bool vobSub = findNoCase(head, "VobSub index file") != View::npos;
    for (size_t i = 0; i < lines.size() && i < 100 && !vobSub; ++i)
        vobSub = startsWithNoCase(trim(lines[i]), "timestamp:");
    if (vobSub)
        cues = parseVobSubIndex(lines, vobSubIndex);
    else if (findNoCase(head, "[script info]") != View::npos || findNoCase(head, "[events]") != View::npos
             || findNoCase(text, "\ndialogue:") != View::npos)
        cues = parseAss(lines);
    else if (findNoCase(head, "<sami") != View::npos || findNoCase(head, "<sync") != View::npos)
        cues = parseSami(text);
    if (cues.empty())
        cues = parseArrows(lines);
    if (cues.empty())
        cues = parseMicroDvd(lines, microDvdFps);
    if (cues.empty())
        cues = parseSubViewer(lines);

    std::stable_sort(cues.begin(), cues.end(),
                     [](const Cue &a, const Cue &b) { return a.startMs < b.startMs; });
    return cues;
}

}
