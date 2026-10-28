/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * The subtitle lines drawn on the seek bar: the parser of their times.
 *   g++ -std=c++17 -O1 -Wall -Wextra -I. extras/juxtopposed/subtitle-cues-parser.cpp \
 *       modules/gui/qt/player/subtitle_cues_parser.cpp -o subtitle-cues-parser && ./subtitle-cues-parser
 */
#include "modules/gui/qt/player/subtitle_cues_parser.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>

using subtitlecues::parse;

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

int main()
{
    // SubRip: the mark of UTF-8, CRLF, tags, two lines of text
    {
        const auto cues = parse("\xEF\xBB\xBF" "1\r\n00:00:01,500 --> 00:00:03,250\r\n<i>Hello</i>\r\nthere\r\n\r\n"
                                "2\r\n00:01:02,000 --> 00:01:04,000 X1:10\r\n{\\an8}Top &amp; more\r\n");
        CHECK(cues.size() == 2);
        CHECK(cues[0].startMs == 1500 && cues[0].endMs == 3250);
        CHECK(cues[0].text == "Hello\nthere");
        CHECK(cues[1].startMs == 62000 && cues[1].endMs == 64000);
        CHECK(cues[1].text == "Top & more");
    }
    // WebVTT: no hours, a NOTE, cue settings, a karaoke time inside
    {
        const auto cues = parse("WEBVTT\n\nNOTE a comment\n\nid-1\n00:05.000 --> 00:07.5 align:start\n"
                                "<v Bob>One <00:06.000>two</v>\n\n01:00:00.000 --> 01:00:01.000\nLast\n");
        CHECK(cues.size() == 2);
        CHECK(cues[0].startMs == 5000 && cues[0].endMs == 7500);
        CHECK(cues[0].text == "One two");
        CHECK(cues[1].startMs == 3600000 && cues[1].endMs == 3601000);
    }
    // ASS: its Format, commas in the text, overrides, \N, a Comment
    {
        const auto cues = parse("[Script Info]\nTitle: x\n\n[V4+ Styles]\nFormat: Name, Fontname\n\n[Events]\n"
                                "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n"
                                "Comment: 0,0:00:00.00,0:00:09.00,Default,,0,0,0,,hidden\n"
                                "Dialogue: 0,0:00:10.20,0:00:12.00,Default,,0,0,0,,{\\b1}Well, yes\\Nno\n"
                                "Dialogue: 0,0:00:02.00,0:00:04.00,Default,,0,0,0,,First\n");
        CHECK(cues.size() == 2);
        CHECK(cues[0].startMs == 2000 && cues[0].endMs == 4000 && cues[0].text == "First");
        CHECK(cues[1].startMs == 10200 && cues[1].endMs == 12000);
        CHECK(cues[1].text == "Well, yes\nno");
    }
    // SSA with fewer fields in another order
    {
        const auto cues = parse("[Events]\nFormat: Start, End, Text\nDialogue: 0:00:01.00,0:00:02.00,A, b\n");
        CHECK(cues.size() == 1 && cues[0].startMs == 1000 && cues[0].endMs == 2000 && cues[0].text == "A, b");
    }
    // MicroDVD: the frame rate of its first line, then VLC's 25
    {
        const auto cues = parse("{1}{1}23.976\n{24}{48}Hello|world\n{100}{}{y:i}open\n");
        CHECK(cues.size() == 2);
        CHECK(cues[0].startMs == 1001 && cues[0].endMs == 2002);
        CHECK(cues[0].text == "Hello\nworld");
        const auto plain = parse("{25}{50}One\n{100}{}Two\n{110}{120}Three\n");
        CHECK(plain.size() == 3);
        CHECK(plain[0].startMs == 1000 && plain[0].endMs == 2000);
        CHECK(plain[1].startMs == 4000 && plain[1].endMs == 4400); // up to the next one
    }
    // SubViewer 2 and SBV
    {
        const auto cues = parse("[INFORMATION]\n[TITLE]x\n[END INFORMATION]\n\n00:00:01.00,00:00:02.50\nA[br]B\n\n"
                                "0:00:03.000,0:00:04.000\nC\n");
        CHECK(cues.size() == 2);
        CHECK(cues[0].startMs == 1000 && cues[0].endMs == 2500 && cues[0].text == "A\nB");
        CHECK(cues[1].startMs == 3000 && cues[1].endMs == 4000);
    }
    // SAMI: an empty SYNC ends the line before it
    {
        const auto cues = parse("<SAMI><BODY><SYNC Start=1000><P Class=ENCC>Hi<br>you\n"
                                "<SYNC Start=2500><P Class=ENCC>&nbsp;\n<SYNC Start=\"4000\"><P>Bye</BODY></SAMI>");
        CHECK(cues.size() == 2);
        CHECK(cues[0].startMs == 1000 && cues[0].endMs == 2500 && cues[0].text == "Hi\nyou");
        CHECK(cues[1].startMs == 4000 && cues[1].endMs == 8000);
    }
    // VobSub index: the stream asked for, its delay
    {
        const std::string idx = "# VobSub index file, v7 (do not modify this line!)\nsize: 720x480\n"
                                "id: en, index: 0\ntimestamp: 00:00:01:000, filepos: 000000000\n"
                                "timestamp: 00:00:03:500, filepos: 000000800\n"
                                "id: fr, index: 1\ndelay: 00:00:01:000\n"
                                "timestamp: 00:00:10:000, filepos: 000001000\n";
        const auto first = parse(idx);
        CHECK(first.size() == 2);
        CHECK(first[0].startMs == 1000 && first[0].endMs == 3500);
        CHECK(first[1].startMs == 3500 && first[1].endMs == 7500);
        const auto second = parse(idx, 25.0, 1);
        CHECK(second.size() == 1 && second[0].startMs == 11000);
        CHECK(parse(idx, 25.0, 7).empty());
    }
    // UTF-16 LE with its mark, and without
    {
        const std::string text = "1\n00:00:01,000 --> 00:00:02,000\nCaf\xC3\xA9\n";
        std::string utf16 = "\xFF\xFE";
        std::string bare;
        for (size_t i = 0; i < text.size(); ++i) {
            const unsigned char c = (unsigned char)text[i];
            if (c == 0xC3) { // é
                utf16 += '\xE9'; utf16 += '\0';
                bare += '\xE9'; bare += '\0';
                ++i;
                continue;
            }
            utf16 += char(c); utf16 += '\0';
            bare += char(c); bare += '\0';
        }
        const auto cues = parse(utf16);
        CHECK(cues.size() == 1 && cues[0].startMs == 1000 && cues[0].text == "Caf\xC3\xA9");
        const auto plain = parse(bare);
        CHECK(plain.size() == 1 && plain[0].endMs == 2000);
    }
    // sorted by start; nothing from what is not a subtitle file
    {
        const auto cues = parse("2\n00:00:05,000 --> 00:00:06,000\nB\n\n1\n00:00:01,000 --> 00:00:02,000\nA\n");
        CHECK(cues.size() == 2 && cues[0].text == "A" && cues[1].text == "B");
        CHECK(parse("").empty());
        CHECK(parse("just some words\n12:345 nothing\n").empty());
        CHECK(parse(std::string("\x1A\x45\xDF\xA3\x00\x00\x01\xBA", 8)).empty());
        // an end before its start is dropped
        CHECK(parse("1\n00:00:05,000 --> 00:00:04,000\nX\n").empty());
    }
    // a long line is cut at a character, with an ellipsis
    {
        const std::string longText(400, 'a');
        const auto cues = parse("1\n00:00:01,000 --> 00:00:02,000\n" + longText + "\n");
        CHECK(cues.size() == 1 && cues[0].text.size() == subtitlecues::maxTextBytes + 3);
    }
    // damaged files (bytes changed, cut short): no crash, sorted ends after
    // starts (run it with -fsanitize=address,undefined)
    {
        const std::string samples[] = {
            "1\n00:00:01,500 --> 00:00:03,250\n<i>Hi</i>\n\n2\n00:01:02,000 --> 00:01:04,000\n{\\an8}x &amp;\n",
            "[Events]\nFormat: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n"
            "Dialogue: 0,0:00:10.20,0:00:12.00,Default,,0,0,0,,{\\b1}Well, yes\\Nno\n",
            "{1}{1}23.976\n{24}{48}Hello|world\n{100}{}open\n",
            "<SAMI><BODY><SYNC Start=1000><P>Hi<br>you\n<SYNC Start=2500>&nbsp;</BODY>",
            "id: en, index: 0\ndelay: -00:00:01:000\ntimestamp: 00:00:01:000, filepos: 0\n",
            "00:00:01.00,00:00:02.50\nA[br]B\n\n",
        };
        unsigned seed = 12345;
        const auto next = [&seed] { seed = seed * 1103515245u + 12345u; return (seed >> 16) & 0x7FFF; };
        for (int round = 0; round < 20000; ++round) {
            std::string data = samples[round % 6];
            const int changes = 1 + int(next() % 8);
            for (int i = 0; i < changes && !data.empty(); ++i) {
                const size_t at = next() % data.size();
                switch (next() % 4) {
                case 0: data[at] = char(next() & 0xFF); break;
                case 1: data.erase(at, 1 + next() % 4); break;
                case 2: data.insert(at, 1, "<>{}:,.-\\|\n0"[next() % 13]); break;
                default: data.resize(at); break;
                }
            }
            const auto cues = parse(data, round % 3 ? 25.0 : 0.0, int(next() % 3) - 1);
            for (size_t i = 0; i < cues.size(); ++i) {
                CHECK(cues[i].endMs >= cues[i].startMs && cues[i].startMs >= 0);
                CHECK(i == 0 || cues[i - 1].startMs <= cues[i].startMs);
            }
            if (failures)
                break;
        }
    }

    if (failures) {
        std::fprintf(stderr, "%d failure(s)\n", failures);
        return EXIT_FAILURE;
    }
    std::puts("PASS: SRT, WebVTT, ASS/SSA, MicroDVD, SubViewer/SBV, SAMI, VobSub idx, UTF-16, sorting, rejects");
    return EXIT_SUCCESS;
}
