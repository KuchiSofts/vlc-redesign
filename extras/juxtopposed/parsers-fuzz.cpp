/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * A robustness (fuzz) test of the parsers of what comes from the network:
 * Live TV's playlists (M3U), guides (XMLTV, gzip) and Xtream Codes answers,
 * the user's servers and tuners (Jellyfin, Emby, HDHomeRun, NextPVR,
 * Schedules Direct, mDNS packets), the automatic guide (iptv-org's channels,
 * the catalog), the subtitle services' answers and the subtitle files read
 * for the seek bar.
 *
 * Each parser is fed an empty input, every prefix of each of its fixtures
 * (random cut points for the big ones), a few thousand random mutations of
 * them (a fixed seed: every run is the same), structured replacements (huge
 * and negative numbers, odd strings, other JSON types in every field) and
 * hostile shapes: deep JSON and XML nesting, 10 MB lines, NUL bytes, invalid
 * UTF-8, gzip bombs against the limits, mDNS pointer loops and lengths past
 * the end, XMLTV times with odd offsets, M3U attributes without their
 * closing quote... Every call is timed (more than 2 s is reported) and its
 * peak allocation measured (with AddressSanitizer); inputs that grow by
 * steps ("ladders") show a cost that grows faster than the input.
 *
 * Each parser runs in a child process: a crash, a sanitizer's report or a
 * hang (killed after 6 s) is recorded with the input that caused it (saved
 * in the work folder, /tmp/parsers-fuzz) and the run goes on with the next
 * input; each crash is run once more alone at the end, for UBSan's stack. A
 * time over 2 s that only grows as the input does (a 10 MB input read in a
 * linear time) is a note; the problems are the crashes, hangs, sanitizer
 * reports, times that grow faster than the input, and allocations far past
 * the input's size. A summary is printed; the exit code is 0 when there is
 * no problem.
 *
 * On Linux, from the root of the tree, with g++, Qt 6 Core and zlib:
 *   g++ -std=c++17 -O1 -g -fsanitize=address,undefined,float-cast-overflow \
 *       -fno-sanitize-recover=undefined,float-cast-overflow -fno-omit-frame-pointer -fPIC -DHAVE_ZLIB \
 *       -I. -Imodules/gui/qt $(pkg-config --cflags Qt6Core) extras/juxtopposed/parsers-fuzz.cpp \
 *       modules/misc/iptv/playlist.cpp modules/misc/iptv/guide.cpp modules/misc/iptv/xtream.cpp \
 *       modules/misc/iptv/servers.cpp modules/misc/iptv/autoguide.cpp \
 *       modules/gui/qt/player/subtitle_services.cpp modules/gui/qt/player/subtitle_cues_parser.cpp \
 *       $(pkg-config --libs Qt6Core) -lz -o parsers-fuzz
 *   ./parsers-fuzz extras/juxtopposed/fixtures
 * (float-cast-overflow is named: g++'s "undefined" leaves it out.)
 * Options: --list; --only <parser>[,<parser>...]; --jobs N (4); --mutations N
 * (2000 a parser); --work <folder>; --repro <parser> <file> (one input, in
 * this process); --cases <parser> (its inputs, numbered); --dump <parser>
 * <index> <file> (writes that input).
 */
#include "modules/misc/iptv/autoguide.hpp"
#include "modules/misc/iptv/guide.hpp"
#include "modules/misc/iptv/playlist.hpp"
#include "modules/misc/iptv/servers.hpp"
#include "modules/misc/iptv/xtream.hpp"
#include "modules/gui/qt/player/subtitle_cues_parser.hpp"
#include "modules/gui/qt/player/subtitle_services.hpp"

#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

#include <zlib.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <new>
#include <random>
#include <set>
#include <string>
#include <vector>

// The sanitizers' own interface (weak: the test also builds without them)
extern "C" {
int __sanitizer_install_malloc_and_free_hooks(void (*)(const volatile void *, size_t),
                                              void (*)(const volatile void *)) __attribute__((weak));
size_t __sanitizer_get_allocated_size(const volatile void *) __attribute__((weak));
int __lsan_do_recoverable_leak_check(void) __attribute__((weak));
// UBSan's reports name their place without a stack: the sweep stays fast,
// and each crash found is run again at the end with its stack
const char *__ubsan_default_options() { return "print_stacktrace=0"; }
}

namespace {

namespace servers = iptv::servers;
namespace xtream = iptv::xtream;
namespace autoguide = iptv::autoguide;
namespace subs = SubtitleProviders;

// ---------------------------------------------------------------------------
// The run's settings

struct Settings
{
    QString fixtures = QStringLiteral("extras/juxtopposed/fixtures");
    std::string work = "/tmp/parsers-fuzz";
    int jobs = 4;
    int mutations = 2000;
    double slowSecs = 2.0;
    double hangSecs = 6.0;
    std::set<std::string> only;
} S;

long long nowNs()
{
    timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000000LL + t.tv_nsec;
}

std::string sizeLabel(long long n)
{
    char buf[32];
    if (n >= (1 << 20) && n % (1 << 20) == 0)
        std::snprintf(buf, sizeof buf, "%lldM", n >> 20);
    else if (n >= 1024 && n % 1024 == 0)
        std::snprintf(buf, sizeof buf, "%lldK", n >> 10);
    else
        std::snprintf(buf, sizeof buf, "%lld", n);
    return buf;
}

std::string mb(long long n)
{
    char buf[32];
    if (n >= (1 << 20))
        std::snprintf(buf, sizeof buf, "%.1f MB", double(n) / (1 << 20));
    else if (n >= 1024)
        std::snprintf(buf, sizeof buf, "%.1f KB", double(n) / 1024);
    else
        std::snprintf(buf, sizeof buf, "%lld B", n);
    return buf;
}

void copyTo(char *dst, size_t size, const std::string &s)
{
    const size_t n = std::min(size - 1, s.size());
    std::memcpy(dst, s.data(), n);
    dst[n] = 0;
}

// ---------------------------------------------------------------------------
// The allocations, counted by AddressSanitizer's hooks: the live bytes and
// their peak during a call

bool g_measure = false;
long long g_live = 0;
long long g_peak = 0;

void onMalloc(const volatile void *, size_t size)
{
    g_live += (long long)size;
    if (g_live > g_peak)
        g_peak = g_live;
}

void onFree(const volatile void *p)
{
    if (p)
        g_live -= (long long)__sanitizer_get_allocated_size(p);
}

// ---------------------------------------------------------------------------
// What is found, kept in memory shared with the parent

enum Kind { Crash, Hang, Slow, Superlinear, Alloc, Leak, Note };
const char *const kindNames[] = { "CRASH", "HANG", "SLOW", "SUPERLINEAR", "ALLOC", "LEAK", "NOTE" };

struct Finding
{
    int kind = 0;
    double value = 0;
    char what[256] = {};
    char detail[2048] = {};
};

constexpr int maxSlotFindings = 48;
constexpr int maxSkip = 128;

struct Slot
{
    std::atomic<long> caseIndex { -1 };
    std::atomic<long long> callStart { 0 };
    std::atomic<long> tried { 0 };
    std::atomic<long> skipped { 0 };
    std::atomic<int> group { -1 };
    char current[256] = {};
    double maxSecs = 0;
    char maxSecsWhat[256] = {};
    long long maxPeak = 0;
    char maxPeakWhat[256] = {};
    int nSkip = 0;
    int skipGroups[maxSkip] = {};
    std::atomic<int> nFindings { 0 };
    Finding findings[maxSlotFindings];
};

Slot *g_slot = nullptr;   // the child's (none in --repro)
std::string g_currentWhat;
std::set<std::string> g_notes;

void addFinding(int kind, double value, const std::string &what, const std::string &detail)
{
    if (!g_slot)
    {
        std::printf("[%s] %s: %s\n", kindNames[kind], what.c_str(), detail.c_str());
        return;
    }
    const int i = g_slot->nFindings.load();
    if (i >= maxSlotFindings)
        return;
    Finding &f = g_slot->findings[i];
    f.kind = kind;
    f.value = value;
    copyTo(f.what, sizeof f.what, what);
    copyTo(f.detail, sizeof f.detail, detail);
    g_slot->nFindings.store(i + 1);
}

// Something wrong that is no crash (a parser's answer that makes no sense)
void note(const std::string &text)
{
    if (g_notes.insert(text).second)
        addFinding(Note, 0, g_currentWhat, text);
}

// ---------------------------------------------------------------------------
// The inputs

struct Case
{
    QByteArray input;
    std::function<QByteArray()> make;   // built when it runs (the big ones)
    std::string what;
    std::string ladder;                 // the ladder's name
    int group = -1;                     // its ladder (sizes growing)
    long long scale = 0;
    long long allow = 0;                // more bytes it may allocate
    bool judged = true;                 // its ladder's growth judged
};

enum InputKind { Json = 1, Xml = 2, Text = 4, Binary = 8 };

struct Builder;

struct Target
{
    std::string name;
    int kind = Json;
    std::vector<std::string> fixtures;   // relative to the fixtures folder ("dir/*": its files)
    std::function<void(const QByteArray &)> run;
    std::function<void(Builder &, const std::vector<QByteArray> &)> extra;
    std::function<std::vector<QByteArray>(const std::vector<QByteArray> &)> moreSeeds;
    int mutations = -1;
    int weight = 1;   // the heavy ones start first
};

struct Seeds
{
    std::vector<QByteArray> data;
    std::vector<std::string> names;
};

struct Builder
{
    std::vector<Case> &cases;
    int nextGroup = 0;

    void add(QByteArray input, std::string what, long long allow = 0)
    {
        Case c;
        c.input = std::move(input);
        c.what = std::move(what);
        c.allow = allow;
        cases.push_back(std::move(c));
    }
    void lazy(std::function<QByteArray()> make, std::string what, long long allow = 0)
    {
        Case c;
        c.make = std::move(make);
        c.what = std::move(what);
        c.allow = allow;
        cases.push_back(std::move(c));
    }
    // the same shape, bigger and bigger (sizes in bytes, about)
    // (judged false: inputs so big that their time is the memory's, the
    // runner checks what it is there for)
    void ladder(const std::string &name, std::vector<long long> sizes, std::function<QByteArray(long long)> make,
                long long allow = 0, bool judged = true)
    {
        const int group = nextGroup++;
        for (const long long n : sizes)
        {
            Case c;
            c.make = [make, n]() { return make(n); };
            c.what = name + " @" + sizeLabel(n);
            c.ladder = name;
            c.group = group;
            c.scale = n;
            c.allow = allow;
            c.judged = judged;
            cases.push_back(std::move(c));
        }
    }
};

// long lines and strings, up to 10 MB; many items, up to 1 MB (enough to see
// how the time grows)
const std::vector<long long> kLadder = { 4 << 10, 16 << 10, 64 << 10, 256 << 10, 1 << 20, 10 << 20 };
const std::vector<long long> kSmallLadder = { 4 << 10, 16 << 10, 64 << 10, 256 << 10, 1 << 20 };

QByteArray fill(const QByteArray &unit, long long bytes)
{
    QByteArray out;
    const long long count = std::max<long long>(1, bytes / std::max<long long>(1, unit.size()));
    out.reserve(qsizetype(count * unit.size()));
    for (long long i = 0; i < count; ++i)
        out += unit;
    return out;
}

// numbered units ("c1", "c2"...) up to about bytes: f(i) gives the unit
QByteArray numbered(long long bytes, const std::function<QByteArray(long long)> &f, const char *separator = "")
{
    QByteArray out;
    out.reserve(qsizetype(bytes + 256));
    for (long long i = 0; out.size() < bytes; ++i)
    {
        if (i > 0)
            out += separator;
        out += f(i);
    }
    return out;
}

QByteArray num(long long v)
{
    return QByteArray::number(v);
}

QByteArray bytes(const char *s, size_t n)
{
    return QByteArray(s, qsizetype(n));
}

// --- JSON and text, read for the structured mutations

struct Span
{
    int begin;
    int end;
};

bool isSpaceChar(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
bool isDigitChar(char c) { return c >= '0' && c <= '9'; }

struct JsonSpans
{
    std::vector<Span> strings;   // the contents of the string values (not the keys)
    std::vector<Span> numbers;
    std::vector<Span> scalars;   // every value but objects and arrays
};

JsonSpans scanJson(const QByteArray &d)
{
    JsonSpans out;
    const int n = int(d.size());
    const char *p = d.constData();
    int i = 0;
    while (i < n)
    {
        const char c = p[i];
        if (c == '"')
        {
            int j = i + 1;
            while (j < n && p[j] != '"')
                j += p[j] == '\\' ? 2 : 1;
            if (j >= n)
                break;
            int k = j + 1;
            while (k < n && isSpaceChar(p[k]))
                ++k;
            if (k >= n || p[k] != ':')
            {
                out.strings.push_back({ i + 1, j });
                out.scalars.push_back({ i, j + 1 });
            }
            i = j + 1;
        }
        else if (c == '-' || isDigitChar(c))
        {
            int j = i + 1;
            while (j < n && (isDigitChar(p[j]) || p[j] == '.' || p[j] == 'e' || p[j] == 'E' || p[j] == '+' || p[j] == '-'))
                ++j;
            out.numbers.push_back({ i, j });
            out.scalars.push_back({ i, j });
            i = j;
        }
        else if ((c == 't' && n - i >= 4 && !std::memcmp(p + i, "true", 4))
                 || (c == 'n' && n - i >= 4 && !std::memcmp(p + i, "null", 4)))
        {
            out.scalars.push_back({ i, i + 4 });
            i += 4;
        }
        else if (c == 'f' && n - i >= 5 && !std::memcmp(p + i, "false", 5))
        {
            out.scalars.push_back({ i, i + 5 });
            i += 5;
        }
        else
            ++i;
    }
    return out;
}

struct TextSpans
{
    std::vector<Span> quoted;   // "..." (attributes)
    std::vector<Span> digits;
    std::vector<Span> texts;    // >...< (XML text)
};

TextSpans scanText(const QByteArray &d)
{
    TextSpans out;
    const int n = int(d.size());
    const char *p = d.constData();
    for (int i = 0; i < n;)
    {
        if (p[i] == '"')
        {
            int j = i + 1;
            while (j < n && p[j] != '"' && p[j] != '\n')
                ++j;
            if (j < n && p[j] == '"')
                out.quoted.push_back({ i + 1, j });
            i = j + 1;
        }
        else if (isDigitChar(p[i]))
        {
            int j = i;
            while (j < n && isDigitChar(p[j]))
                ++j;
            out.digits.push_back({ i, j });
            i = j;
        }
        else
            ++i;
    }
    for (int i = 0; i < n; ++i)
        if (p[i] == '>')
        {
            int j = i + 1;
            while (j < n && p[j] != '<')
                ++j;
            if (j < n && j > i + 1)
            {
                bool blank = true;
                for (int k = i + 1; k < j && blank; ++k)
                    blank = isSpaceChar(p[k]);
                if (!blank)
                    out.texts.push_back({ i + 1, j });
            }
            i = j - 1;
        }
    return out;
}

QByteArray replaced(const QByteArray &d, const std::vector<Span> &spans, const QByteArray &with)
{
    QByteArray out;
    int at = 0;
    for (const Span &s : spans)
    {
        if (s.begin < at)
            continue;
        out.append(d.constData() + at, s.begin - at);
        out.append(with);
        at = s.end;
    }
    out.append(d.constData() + at, d.size() - at);
    return out;
}

const char *const hostileNumbers[] = {
    "1e308", "-1e308", "1e19", "-1e19", "9223372036854775807", "9223372036854775808", "-9223372036854775809",
    "2147483647", "2147483648", "-2147483649", "4294967296", "-1", "0", "1e-320", "3.5", "-0",
};
// in a JSON string, escaped
const char *const hostileJsonStrings[] = {
    "", " ", "nan", "inf", "-inf", "1e400", "-1", "2147483648", "\\u0000", "\\ud800", "\\udfff\\ud800", "%",
    "%00%ff", "../../../etc/passwd", "http://[::1", "http://a:99999999/x", "//evil.example/x",
    "S01E01 S99E99 1x1 2x03", "tt", "tt99999999999999999999", "9999-99-99T99:99:99Z",
    "0001-01-01T00:00:00.0000000Z", "2026-11-08T10:00:00.0000000Z", "\xC3\x28\xFF\xFE", "\xED\xA0\x80",
    "((((((((((", "[[[[", "|||", "* - * - *", "\\\"", "\\\\",
};
// in XML, M3U and text
const char *const hostileTextStrings[] = {
    "", " ", "nan", "inf", "-1", "2147483648", "%", "../../x", "http://[::1", "&lt;&amp;", "&#0;", "&#x110000;",
    "&undefined;", "\xC3\x28\xFF\xFE", "\xED\xA0\x80", "{duration:", "{offset:-1}", "{duration:0}",
    "${start}{utc}{Y}", "((((((((((", "+1 +12 +99", "\"", "'",
};
const char *const jsonTypes[] = { "null", "true", "{}", "[]", "\"x\"", "0", "[[[]]]", "{\"a\":[{}]}", "-0.0", "\"\"" };
const unsigned char interestingBytes[] = { 0x00, 0xFF, 0x7F, 0x80, 0xC0, 0xED, 0xFE, '"', '\\', '{', '}', '[', ']',
                                           ',', ':', '<', '>', '/', '&', ';', '\n', '\r', '#', '(', ')', '-', '+',
                                           '9', '0', '.', '=', '\'', '|', ' ', 'e', '%' };
const char invalidUtf8[] = "\xC3\x28\xA0\xA1\xE2\x28\xA1\xF0\x28\x8C\xBC\xF8\xA1\xA1\xA1\xA1\xFC\xA1\xA1\xA1\xA1\xA1"
                           "\xFF\xFE\xED\xA0\x80\xC0\x80\xF4\x90\x80\x80";

template <typename T, size_t N>
constexpr size_t countOf(const T (&)[N])
{
    return N;
}

using Rng = std::mt19937_64;

// A mutation of one of the seeds: 1 to 4 changes; which seed in *from
QByteArray mutate(Rng &rng, const std::vector<QByteArray> &seeds, int kind, size_t *from)
{
    auto pick = [&](size_t n) { return size_t(rng() % std::max<size_t>(n, 1)); };
    *from = pick(seeds.size());
    QByteArray d = seeds[*from];
    const int rounds = 1 + int(rng() % 4);
    for (int r = 0; r < rounds; ++r)
    {
        const int op = int(rng() % 11);
        const int n = int(d.size());
        switch (op)
        {
        case 0:
            if (n)
                d[int(pick(n))] ^= char(1 << (rng() % 8));
            break;
        case 1:
            if (n)
                d[int(pick(n))] = char(interestingBytes[pick(sizeof interestingBytes)]);
            break;
        case 2:
            d.insert(int(pick(n + 1)), char(interestingBytes[pick(sizeof interestingBytes)]));
            break;
        case 3:
            if (n)
                d.remove(int(pick(n)), int(1 + pick(16)));
            break;
        case 4:
            if (n)
            {
                const int at = int(pick(n));
                const int len = int(1 + pick(std::min(64, n - at)));
                d.insert(int(pick(n + 1)), d.mid(at, len));
            }
            break;
        case 5:
        case 6:
        case 7:
            if (kind & Json)
            {
                const JsonSpans s = scanJson(d);
                if (op == 5 && !s.numbers.empty())
                    d = replaced(d, { s.numbers[pick(s.numbers.size())] }, hostileNumbers[pick(countOf(hostileNumbers))]);
                else if (op == 6 && !s.strings.empty())
                    d = replaced(d, { s.strings[pick(s.strings.size())] },
                                 hostileJsonStrings[pick(countOf(hostileJsonStrings))]);
                else if (op == 7 && !s.scalars.empty())
                    d = replaced(d, { s.scalars[pick(s.scalars.size())] }, jsonTypes[pick(countOf(jsonTypes))]);
            }
            else
            {
                const TextSpans s = scanText(d);
                if (op == 5 && !s.digits.empty())
                    d = replaced(d, { s.digits[pick(s.digits.size())] }, hostileNumbers[pick(countOf(hostileNumbers))]);
                else if (op == 6 && !s.quoted.empty())
                    d = replaced(d, { s.quoted[pick(s.quoted.size())] },
                                 hostileTextStrings[pick(countOf(hostileTextStrings))]);
                else if (op == 7 && !s.texts.empty())
                    d = replaced(d, { s.texts[pick(s.texts.size())] },
                                 hostileTextStrings[pick(countOf(hostileTextStrings))]);
            }
            break;
        case 8:
        {
            const QByteArray &o = seeds[pick(seeds.size())];
            d = d.left(int(pick(n + 1))) + o.mid(int(pick(o.size() + 1)));
            break;
        }
        case 9:
        {
            static const QByteArray bad[] = { bytes("\xC3", 1), bytes("\xFF", 1), bytes("\xED\xA0\x80", 3),
                                              bytes("\0", 1), bytes("\xF4\x90\x80\x80", 4), bytes("\xC0\x80", 2) };
            d.insert(int(pick(n + 1)), bad[pick(countOf(bad))]);
            break;
        }
        default:
            if (n)
            {
                const int at = int(pick(n));
                const int len = int(1 + pick(std::min(16, n - at)));
                for (int k = 0; k < len; ++k)
                    d[at + k] = char(rng());
            }
            break;
        }
    }
    return d;
}

QByteArray gzipOf(const QByteArray &d, int level = 9)
{
    z_stream zs {};
    if (deflateInit2(&zs, level, Z_DEFLATED, 16 + MAX_WBITS, 9, Z_DEFAULT_STRATEGY) != Z_OK)
        return {};
    QByteArray out;
    out.resize(qsizetype(deflateBound(&zs, uLong(d.size())) + 64));
    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(d.constData()));
    zs.avail_in = uInt(d.size());
    zs.next_out = reinterpret_cast<Bytef *>(out.data());
    zs.avail_out = uInt(out.size());
    deflate(&zs, Z_FINISH);
    out.resize(qsizetype(zs.total_out));
    deflateEnd(&zs);
    return out;
}

QByteArray rawDeflate(const QByteArray &d)
{
    z_stream zs {};
    if (deflateInit2(&zs, 9, Z_DEFLATED, -MAX_WBITS, 9, Z_DEFAULT_STRATEGY) != Z_OK)
        return {};
    QByteArray out;
    out.resize(qsizetype(deflateBound(&zs, uLong(d.size())) + 64));
    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(d.constData()));
    zs.avail_in = uInt(d.size());
    zs.next_out = reinterpret_cast<Bytef *>(out.data());
    zs.avail_out = uInt(out.size());
    deflate(&zs, Z_FINISH);
    out.resize(qsizetype(zs.total_out));
    deflateEnd(&zs);
    return out;
}

QByteArray le16(quint32 v)
{
    return QByteArray(1, char(v & 0xFF)) + char((v >> 8) & 0xFF);
}

QByteArray le32(quint32 v)
{
    return le16(v & 0xFFFF) + le16(v >> 16);
}

quint32 crcOf(const QByteArray &d)
{
    return quint32(crc32(0, reinterpret_cast<const Bytef *>(d.constData()), uInt(d.size())));
}

// A zip archive: its files (stored, method 0, or deflated, 8), then its
// central directory and its end
struct ZipFile
{
    QByteArray name;
    QByteArray data;
    int method = 8;
    bool badCrc = false;
};

QByteArray zipHeaders(quint32 signature, const ZipFile &f, const QByteArray &packed, bool central, quint32 offset)
{
    const quint32 crc = f.badCrc ? 0xDEADBEEFu : crcOf(f.data);
    QByteArray h = le32(signature);
    if (central)
        h += le16(20);
    h += le16(20) + le16(0) + le16(quint32(f.method)) + le16(0) + le16(0) + le32(crc) + le32(quint32(packed.size()))
         + le32(quint32(f.data.size())) + le16(quint32(f.name.size())) + le16(0);
    if (central)
        h += le16(0) + le16(0) + le16(0) + le32(0) + le32(offset);
    return h + f.name;
}

QByteArray zipOf(const std::vector<ZipFile> &files, const QByteArray &comment = QByteArray())
{
    QByteArray out, directory;
    for (const ZipFile &f : files)
    {
        const QByteArray packed = f.method == 8 ? rawDeflate(f.data) : f.data;
        const quint32 offset = quint32(out.size());
        out += zipHeaders(0x04034b50, f, packed, false, 0) + packed;
        directory += zipHeaders(0x02014b50, f, packed, true, offset);
    }
    const quint32 at = quint32(out.size());
    return out + directory + le32(0x06054b50) + le16(0) + le16(0) + le16(quint32(files.size()))
           + le16(quint32(files.size())) + le32(quint32(directory.size())) + le32(at) + le16(quint32(comment.size()))
           + comment;
}

// gzip members of `megabytes` MB of zeros, one after the other (1 KB each)
QByteArray gzipBomb(int members, int megabytesEach)
{
    const QByteArray member = gzipOf(QByteArray(qsizetype(megabytesEach) << 20, '\0'));
    QByteArray out;
    for (int i = 0; i < members; ++i)
        out += member;
    return out;
}

QByteArray readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return f.readAll();
}

Seeds seedsOf(const Target &t)
{
    Seeds s;
    for (const std::string &f : t.fixtures)
    {
        if (f.size() > 2 && f.compare(f.size() - 2, 2, "/*") == 0)
        {
            const QString dir = S.fixtures + QLatin1Char('/') + QString::fromStdString(f.substr(0, f.size() - 2));
            const QStringList names = QDir(dir).entryList(QDir::Files, QDir::Name);
            for (const QString &name : names)
                if (!name.endsWith(QLatin1String(".expected")) && !name.endsWith(QLatin1String(".py"))
                    && !name.startsWith(QLatin1Char('.')))
                {
                    s.data.push_back(readFile(dir + QLatin1Char('/') + name));
                    s.names.push_back(f.substr(0, f.size() - 1) + name.toStdString());
                }
            continue;
        }
        const QByteArray d = readFile(S.fixtures + QLatin1Char('/') + QString::fromStdString(f));
        if (d.isEmpty())
            std::fprintf(stderr, "warning: no fixture %s\n", f.c_str());
        s.data.push_back(d);
        s.names.push_back(f);
    }
    if (t.moreSeeds)
    {
        const std::vector<QByteArray> more = t.moreSeeds(s.data);
        for (size_t i = 0; i < more.size(); ++i)
        {
            s.data.push_back(more[i]);
            s.names.push_back("derived seed " + std::to_string(i));
        }
    }
    if (s.data.empty())
    {
        s.data.push_back(QByteArray());
        s.names.push_back("(none)");
    }
    return s;
}

QByteArray utf16le(const QByteArray &utf8)
{
    const QString s = QString::fromUtf8(utf8);
    QByteArray out("\xFF\xFE", 2);
    out.append(reinterpret_cast<const char *>(s.utf16()), s.size() * 2);
    return out;
}

std::vector<Case> buildCases(const Target &t, const Seeds &seeds)
{
    std::vector<Case> cases;
    Builder b { cases };
    Rng rng(std::hash<std::string>()(t.name) ^ 0x5eedf00dULL);
    const std::vector<QByteArray> &data = seeds.data;

    // 1. nothing, and the plain odd inputs
    b.add({}, "empty");
    b.add(QByteArray(1, '\0'), "one NUL byte");
    b.add(QByteArray(4096, '\0'), "4096 NUL bytes");
    b.add(bytes(invalidUtf8, sizeof invalidUtf8 - 1), "invalid UTF-8");
    b.add("\xEF\xBB\xBF", "a UTF-8 mark alone");
    b.add(bytes("\xFF\xFE", 2), "a UTF-16 mark alone");
    if (t.kind & Json)
        for (const char *top : { "null", "true", "0", "\"s\"", "[]", "{}", "[null,1,\"x\",[],{},true]", "[{}]", "{\"\":{}}" })
            b.add(top, std::string("JSON ") + top);
    for (int k = 0; k < 24; ++k)
    {
        QByteArray r;
        const int len = int(rng() % 2048);
        for (int i = 0; i < len; ++i)
            r += char(rng());
        b.add(r, "random bytes #" + std::to_string(k));
    }
    for (size_t si = 0; si < data.size(); ++si)
    {
        const QByteArray &d = data[si];
        const std::string &name = seeds.names[si];
        b.add(d, "fixture " + name);
        b.add("\xEF\xBB\xBF" + d, "UTF-8 mark + " + name);
        b.add(QByteArray(d).replace('\n', '\r'), name + " with CR line ends");
        b.add(QByteArray(d).replace("\n", "\r\n"), name + " with CRLF line ends");
        b.add(d + d, name + " twice");
        QByteArray nuls;
        for (int i = 0; i < d.size(); ++i)
        {
            nuls += d[i];
            if (i % 7 == 6)
                nuls += '\0';
        }
        b.add(nuls, name + " with NUL bytes inside");
        b.add(utf16le(d), name + " as UTF-16");
        const std::vector<Span> quoted = (t.kind & Json) ? scanJson(d).strings : scanText(d).quoted;
        std::vector<Span> starts;
        for (const Span &s : quoted)
            starts.push_back({ s.begin, s.begin });
        b.add(replaced(d, starts, bytes("\xFF\xC3", 2)), name + " with invalid UTF-8 in every string");
        b.add(replaced(d, starts, bytes("\0", 1)), name + " with a NUL in every string");
    }

    // 2. every prefix (random cut points past 4 KB)
    for (size_t si = 0; si < data.size(); ++si)
    {
        const QByteArray &d = data[si];
        if (d.size() <= 4096)
            for (int len = 1; len < d.size(); ++len)
                b.add(d.left(len), "prefix " + std::to_string(len) + " of " + seeds.names[si]);
        else
            for (int k = 0; k < 300; ++k)
            {
                const int len = int(rng() % uint64_t(d.size()));
                b.add(d.left(len), "prefix " + std::to_string(len) + " of " + seeds.names[si]);
            }
    }

    // 3. random mutations
    const int mutations = t.mutations >= 0 ? t.mutations : S.mutations;
    for (int k = 0; k < mutations; ++k)
    {
        size_t from = 0;
        QByteArray m = mutate(rng, data, t.kind, &from);
        b.add(std::move(m), "mutation #" + std::to_string(k) + " of " + seeds.names[from]);
    }

    // 4. structured: every number, every string, every value of another type
    for (size_t si = 0; si < data.size(); ++si)
    {
        const QByteArray &d = data[si];
        const std::string &name = seeds.names[si];
        if (t.kind & Json)
        {
            const JsonSpans s = scanJson(d);
            if (!s.numbers.empty())
                for (const char *v : hostileNumbers)
                    b.add(replaced(d, s.numbers, v), name + ": every number " + v);
            if (!s.strings.empty())
                for (const char *v : hostileJsonStrings)
                    b.add(replaced(d, s.strings, v), name + ": every string \"" + v + "\"");
            if (!s.scalars.empty())
                for (const char *v : jsonTypes)
                    b.add(replaced(d, s.scalars, v), name + ": every value " + v);
            // digits inside strings (dates, ids, numbers given as text)
            const TextSpans ts = scanText(d);
            if (!ts.digits.empty())
                for (const char *v : { "99999999999999999999", "-1", "2147483648", "0" })
                    b.add(replaced(d, ts.digits, v), name + ": every digit run " + v);
        }
        else if (t.kind & (Xml | Text))
        {
            const TextSpans s = scanText(d);
            if (!s.digits.empty())
                for (const char *v : hostileNumbers)
                    b.add(replaced(d, s.digits, v), name + ": every digit run " + v);
            if (!s.quoted.empty())
                for (const char *v : hostileTextStrings)
                    b.add(replaced(d, s.quoted, v), name + ": every quoted value \"" + v + "\"");
            if (!s.texts.empty())
                for (const char *v : hostileTextStrings)
                    b.add(replaced(d, s.texts, v), name + ": every text \"" + v + "\"");
        }
    }

    // 5. the hostile shapes of the kind
    if (t.kind & Json)
    {
        b.lazy([] { return fill("[", 100000); }, "JSON: 100000 [ never closed");
        b.lazy([] { return fill("{\"a\":", 100000 * 5); }, "JSON: 100000 nested objects never closed");
        b.add(fill("[", 1000) + fill("]", 1000), "JSON: 1000 nested arrays");
        b.add(fill("[", 1025) + fill("]", 1025), "JSON: 1025 nested arrays");
        const JsonSpans s = scanJson(data[0]);
        if (!s.scalars.empty())
            b.add(replaced(data[0], { s.scalars[0] }, fill("[", 1000) + fill("]", 1000)),
                  seeds.names[0] + ": its first value 1000 arrays deep");
        b.lazy([] { return "{" + fill("\"k\":1,", 200000 * 6) + "\"k\":1}"; }, "JSON: 200000 times the same key");
        b.lazy([] { return "[" + fill("{},", 300000 * 3) + "{}]"; }, "JSON: an array of 300000 empty objects");
        b.lazy([] { return "[" + fill("null,", 300000 * 5) + "null]"; }, "JSON: an array of 300000 nulls");
    }
    if (t.kind & Xml)
    {
        b.lazy([] { return "<tv>" + fill("<a>", 200000 * 3); }, "XML: 200000 elements never closed");
        b.lazy([] { return "<tv>" + fill("<a>", 100000 * 3) + fill("</a>", 100000 * 4) + "</tv>"; },
               "XML: 100000 nested elements");
        QByteArray laughs = "<?xml version=\"1.0\"?>\n<!DOCTYPE tv [\n<!ENTITY l0 \"lollollollollollollollollollol\">\n";
        for (int i = 1; i <= 9; ++i)
        {
            laughs += "<!ENTITY l" + num(i) + " \"";
            for (int k = 0; k < 10; ++k)
                laughs += "&l" + num(i - 1) + ";";
            laughs += "\">\n";
        }
        laughs += "]>\n<tv><channel id=\"c\"><display-name>&l9;</display-name></channel>"
                  "<programme start=\"20261025180000 +0000\" channel=\"c\"><title>&l9;</title><desc>&l5;</desc>"
                  "</programme></tv>\n";
        b.add(laughs, "XML: a billion laughs");
        b.add("<?xml version=\"1.0\"?><!DOCTYPE tv [<!ENTITY x SYSTEM \"file:///etc/passwd\">]><tv>"
              "<programme start=\"20261025180000 +0000\" channel=\"c\"><title>&x;</title></programme></tv>",
              "XML: an external entity");
        b.add("<tv><programme start=\"20261025180000 +0000\" channel=\"c\"><title>&nope;</title></programme></tv>",
              "XML: an undefined entity");
        b.add("<?xml version=\"1.0\" encoding=\"UTF-16\"?><tv><programme start=\"20261025180000 +0000\" channel=\"c\">"
              "<title>x</title></programme></tv>",
              "XML: UTF-16 declared, UTF-8 given");
        b.add("<?xml version=\"1.0\" encoding=\"x-nonsense\"?><tv/>", "XML: an unknown encoding");
    }
    if (t.kind & Binary)
    {
        for (int k = 0; k < 200; ++k)
        {
            QByteArray r;
            const int len = int(rng() % 600);
            for (int i = 0; i < len; ++i)
                r += char(interestingBytes[rng() % sizeof interestingBytes]);
            b.add(r, "random packet #" + std::to_string(k));
        }
        b.add(QByteArray(512, char(0xC0)), "512 bytes 0xC0");
        b.add(QByteArray(512, char(0x3F)), "512 bytes 0x3F");
        b.add(QByteArray(512, char(0xFF)), "512 bytes 0xFF");
        b.add(QByteArray(65535, char(0xFF)), "65535 bytes 0xFF");
    }

    // 6. the parser's own
    if (t.extra)
        t.extra(b, data);

    // 7. ladders: 10 MB lines and strings
    if (t.kind & (Text | Xml))
        b.ladder("a single line of A", kLadder, [](long long n) { return QByteArray(qsizetype(n), 'A'); });
    if (t.kind & Xml)
        b.ladder("XML: a programme with a long title", kLadder, [](long long n) {
            return "<tv><programme start=\"20261025180000 +0000\" channel=\"c\"><title>" + QByteArray(qsizetype(n), 'A')
                   + "</title></programme></tv>";
        });
    if (t.kind & Json)
    {
        const QByteArray first = data[0];
        const JsonSpans s = scanJson(first);
        if (!s.strings.empty())
        {
            const Span span = s.strings[0];
            b.ladder(seeds.names[0] + ": its first string long", kLadder,
                     [first, span](long long n) { return replaced(first, { span }, QByteArray(qsizetype(n), 'A')); });
            const int count = int(s.strings.size());
            std::vector<long long> sizes;
            for (const long long n : { 1 << 10, 4 << 10, 16 << 10, 64 << 10, 256 << 10 })
                if (n * count <= (16 << 20))
                    sizes.push_back(n);
            const std::vector<Span> all = s.strings;
            if (sizes.size() > 1)
                b.ladder(seeds.names[0] + ": every string x---x long", sizes, [first, all](long long n) {
                    return replaced(first, all, "x" + QByteArray(qsizetype(n), '-') + "x");
                });
        }
    }
    return cases;
}

// ---------------------------------------------------------------------------
// The parsers, and what each is fed

const QDateTime &now()
{
    static const QDateTime t(QDate(2026, 10, 25), QTime(22, 0), QTimeZone::utc());
    return t;
}

template <typename F>
void forLines(const QByteArray &d, int cap, F f)
{
    qsizetype at = 0;
    int n = 0;
    while (at <= d.size() && n < cap)
    {
        qsizetype nl = d.indexOf('\n', at);
        if (nl < 0)
            nl = d.size();
        f(QString::fromUtf8(d.constData() + at, nl - at));
        ++n;
        at = nl + 1;
    }
}

iptv::GuideFilter filterAll()
{
    return {};
}

iptv::GuideFilter filterSome()
{
    iptv::GuideFilter f;
    f.keys = { iptv::guideKey(QStringLiteral("news.example")), iptv::guideKey(QStringLiteral("ch42")),
               iptv::guideKey(QStringLiteral("Example News HD")), iptv::guideKey(QStringLiteral("shop")),
               iptv::guideKey(QStringLiteral("c")) };
    f.from = now().addDays(-1).toSecsSinceEpoch();
    f.until = now().addDays(2).toSecsSinceEpoch();
    f.languages = { QStringLiteral("en") };
    return f;
}

void useGuide(const iptv::Guide &g)
{
    for (auto it = g.programmes.cbegin(); it != g.programmes.cend(); ++it)
    {
        const auto &list = it.value();
        for (qsizetype i = 1; i < list.size(); ++i)
            if (list[i - 1].start > list[i].start)
                note("a channel's programmes are not sorted by start");
        if (!list.isEmpty())
            iptv::programmeAt(list, list.first().start);
    }
}

// The duration a catch-up input asks for ("#FUZZ-DURATION:<seconds>" first)
int fuzzDuration(const QByteArray &d, int fallback)
{
    if (!d.startsWith("#FUZZ-DURATION:"))
        return fallback;
    const qsizetype nl = d.indexOf('\n');
    return d.mid(15, nl < 0 ? -1 : nl - 15).toInt();
}

const xtream::Account &account()
{
    static const xtream::Account a { QStringLiteral("http://panel.example:8080"), QStringLiteral("me"),
                                     QStringLiteral("p@ss") };
    return a;
}

const QVector<xtream::Category> &fixedCategories()
{
    static const QVector<xtream::Category> c = xtream::parseCategories(
        readFile(S.fixtures + QStringLiteral("/fuzz/xtream-categories.json")));
    return c;
}

const xtream::AccountInfo &fixedInfo()
{
    static const xtream::AccountInfo i = xtream::parseAccountInfo(
        readFile(S.fixtures + QStringLiteral("/fuzz/xtream-account.json")));
    return i;
}

void catchups(const iptv::Channel &c, int duration)
{
    for (const qint64 back : { 600, 3600, 2 * 86400, 30 * 86400 })
        iptv::catchupUrl(c, now().addSecs(-back), duration, now());
    iptv::catchupUrl(c, QDateTime(), duration, now());
}

// The playlist written for a server's channels reads back as many channels
void roundTripM3u(const QVector<servers::Channel> &channels)
{
    const QByteArray m3u = servers::m3uOf(channels, QStringLiteral("https://guide.example/x.xml?a=\"b\""));
    const iptv::Playlist back = iptv::parseM3u(m3u);
    int expected = 0;
    for (const servers::Channel &c : channels)
        expected += c.url.isEmpty() ? 0 : 1;
    if (back.channels.size() != expected)
        note("m3uOf's playlist reads back as " + std::to_string(back.channels.size()) + " channels, not "
             + std::to_string(expected));
}

void roundTripXmltv(const QVector<servers::Channel> &channels, const QVector<servers::Programme> &programmes)
{
    const QByteArray xml = servers::xmltvOf(channels, programmes);
    const iptv::Guide g = iptv::parseXmltv(xml, filterAll());
    if (!g.error.isEmpty() && !programmes.isEmpty())
        note("xmltvOf's guide does not read back: " + g.error.left(80).toStdString());
}

// --- subtitle services

subs::Query filmQuery()
{
    subs::Query q;
    q.imdb = QStringLiteral("tt0133093");
    q.title = QStringLiteral("The Matrix");
    q.year = 1999;
    q.fileName = QStringLiteral("The.Matrix.1999.1080p.BluRay.x264.mkv");
    q.hash = QStringLiteral("8e245d9679d31e12");
    q.languages = { QStringLiteral("ar"), QStringLiteral("pt-BR"), QStringLiteral("zh-TW"), QStringLiteral("en"),
                    QStringLiteral("zh") };
    q.credentials.key = QStringLiteral("my-key");
    return q;
}

subs::Query episodeQuery()
{
    subs::Query q;
    q.imdb = QStringLiteral("tt0903747");
    q.title = QStringLiteral("Breaking Bad");
    q.series = true;
    q.season = 2;
    q.episode = 3;
    q.fileName = QStringLiteral("Breaking.Bad.S02E03.720p.BluRay.x264-DEMAND.mkv");
    q.languages = { QStringLiteral("ar"), QStringLiteral("en"), QStringLiteral("hr"), QStringLiteral("ro") };
    q.credentials.key = QStringLiteral("my-key");
    return q;
}

QString languageOfName(const QString &name)
{
    const QString n = name.trimmed().toLower();
    if (n.contains(QLatin1String("brazil")))
        return QStringLiteral("pt-BR");
    static const QHash<QString, QString> names = {
        { "arabic", "ar" }, { "english", "en" }, { "french", "fr" }, { "german", "de" }, { "hebrew", "he" },
    };
    if (names.contains(n))
        return names.value(n);
    return n.size() == 2 ? n : QString();
}

void checkResults(const QList<subs::Result> &results)
{
    for (const subs::Result &r : results)
    {
        if (!std::isfinite(r.rank))
            note("a result's rank is " + std::string(std::isnan(r.rank) ? "NaN" : "infinite") + " ("
                 + r.provider.toStdString() + ")");
        if (!std::isfinite(r.fps))
            note("a result's fps is not finite (" + r.provider.toStdString() + ")");
    }
}

// --- packets

QByteArray be16(int v)
{
    QByteArray b(2, '\0');
    b[0] = char((v >> 8) & 0xFF);
    b[1] = char(v & 0xFF);
    return b;
}

QByteArray dnsHeader(int questions, int answers, int authority = 0, int additional = 0)
{
    return QByteArray(2, '\0') + bytes("\x84\0", 2) + be16(questions) + be16(answers) + be16(authority)
           + be16(additional);
}

QByteArray dnsName(const char *dotted)
{
    QByteArray out;
    for (const QByteArray &label : QByteArray(dotted).split('.'))
        if (!label.isEmpty())
            out += char(label.size()) + label;
    return out + '\0';
}

QByteArray dnsRecord(const QByteArray &name, int type, const QByteArray &data)
{
    return name + be16(type) + be16(0x8001) + bytes("\0\0\0\x78", 4) + be16(int(data.size())) + data;
}

QByteArray hdhrPacket(int type, const QByteArray &payload)
{
    QByteArray p = be16(type) + be16(int(payload.size())) + payload;
    const uLong crc = crc32(0, reinterpret_cast<const Bytef *>(p.constData()), uInt(p.size()));
    for (int i = 0; i < 4; ++i)
        p += char((crc >> (8 * i)) & 0xFF);
    return p;
}

const QString channelsService = QStringLiteral("_channels_dvr._tcp.local");

// --- the list

Target target(const char *name, int kind, std::vector<std::string> fixtures, std::function<void(const QByteArray &)> run)
{
    Target t;
    t.name = name;
    t.kind = kind;
    t.fixtures = std::move(fixtures);
    t.run = std::move(run);
    return t;
}

std::vector<QByteArray> gzipped(const std::vector<QByteArray> &seeds)
{
    std::vector<QByteArray> out;
    for (const QByteArray &d : seeds)
        out.push_back(gzipOf(d));
    return out;
}

std::vector<Target> makeTargets()
{
    std::vector<Target> list;
    const std::vector<std::string> m3uFixtures = { "iptv-playlist.m3u", "iptv-servers/tvheadend-playlist.m3u",
                                                   "fuzz/m3u-extra.m3u" };
    const std::vector<std::string> guideFixtures = { "iptv-guide.xml", "fuzz/guide-extra.xml" };

    // ----- Live TV: playlists
    {
        Target t = target("parseM3u", Text, m3uFixtures, [](const QByteArray &d) {
            const iptv::Playlist a = iptv::parseM3u(d);
            const iptv::Playlist b = iptv::parseM3u(d, QStringLiteral("http://base.example/dir/list.m3u?x=1"));
            for (const iptv::Channel &c : a.channels)
            {
                iptv::displayGroup(c.group);
                iptv::looksAdult(c.group, c.name);
            }
            iptv::isChannelList(d);
        });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.add("#EXTM3U\n#EXTINF:-1 tvg-name=\"never closed,Title\nhttp://h.example/1\n", "M3U: a quote never closed");
            b.add("#EXTM3U\n#EXTINF:-1 tvg-name='never closed,Title\nhttp://h.example/1\n",
                  "M3U: a single quote never closed");
            b.add("#EXTM3U url-tvg=\"https://g.example/a.xml,\n#EXTINF:-1 group-title=\",T\nhttp://h.example/1\n",
                  "M3U: header and entry quotes never closed");
            b.add("#EXTINF:-1 a=\"b\" c=\"d\nhttp://h.example/1\n", "M3U: no header, an attribute never closed");
            b.add("#EXTM3U\n#EXTINF:-1 =\"x\" == \"\" ,T\nhttp://h.example/1\n", "M3U: empty keys");
            b.add("#EXTM3U\n#EXTINF\nhttp://h.example/1\n#EXTINF:\nhttp://h.example/2\n#EXTINF:-1\n",
                  "M3U: EXTINF without colon or comma");
            for (const char *v : { "1e300", "-1e300", "nan", "inf", "-inf", "1e10", "596524", "-596524", "596523" })
            {
                b.add(QByteArray("#EXTM3U\n#EXTINF:-1 tvg-shift=\"") + v + "\",T\nhttp://h.example/1\n",
                      std::string("M3U: tvg-shift=") + v);
                b.add(QByteArray("#EXTM3U tvg-shift=\"") + v + "\"\n#EXTINF:-1,T\nhttp://h.example/1\n",
                      std::string("M3U: header tvg-shift=") + v);
            }
            for (const char *v : { "99999999999", "-5", "2147483647", "-2147483648", "1e3" })
                b.add(QByteArray("#EXTM3U\n#EXTINF:-1 catchup=\"default\" catchup-days=\"") + v + "\" tvg-chno=\"" + v
                          + "\",T\nhttp://h.example/1\n",
                      std::string("M3U: catchup-days and tvg-chno ") + v);
            b.add("#EXTM3U\n#EXTINF:-1,T\n#EXTINF:-1,U\n#EXTINF:-1,V\nhttp://h.example/1\n", "M3U: EXTINF after EXTINF");
            b.add("#EXTM3U\n#EXTGRP:\n#EXTVLCOPT:\n#EXTVLCOPT:x=1\nhttp://h.example/1\n", "M3U: EXTGRP, EXTVLCOPT empty");
            b.add("#EXTM3U\n#EXTINF:-1,T\nhttp://[::1\n#EXTINF:-1,U\nhttp://a:99999999/\n#EXTINF:-1,V\n:\n//\n\xFF\xFE\n",
                  "M3U: odd addresses");
            b.ladder("M3U: url-tvg with many guide addresses", kSmallLadder, [](long long n) {
                return "#EXTM3U url-tvg=\""
                       + numbered(n, [](long long i) { return "http://g" + num(i) + ".example/"; }, ",")
                       + "\"\n#EXTINF:-1,T\nhttp://h.example/1\n";
            });
            b.ladder("M3U: an EXTINF with many attributes", kSmallLadder, [](long long n) {
                return "#EXTM3U\n#EXTINF:-1 "
                       + numbered(n, [](long long i) { return "a" + num(i) + "=\"v\""; }, " ")
                       + ",T\nhttp://h.example/1\n";
            });
            b.ladder("M3U: many channels", kSmallLadder, [](long long n) {
                return "#EXTM3U\n" + numbered(n, [](long long i) {
                           return "#EXTINF:-1 tvg-id=\"c" + num(i) + "\" group-title=\"G\",Channel " + num(i)
                                  + "\nhttp://h.example/" + num(i) + "\n";
                       });
            });
            b.ladder("M3U: an attribute never closed, long", kLadder, [](long long n) {
                return "#EXTM3U\n#EXTINF:-1 tvg-name=\"" + QByteArray(qsizetype(n), 'A') + "\nhttp://h.example/1\n";
            });
            b.ladder("M3U: a long title of words", kLadder, [](long long n) {
                return "#EXTM3U\n#EXTINF:-1," + fill("word ", n) + "\nhttp://h.example/1\n";
            });
            b.ladder("M3U: a title of 18+ marks", kLadder, [](long long n) {
                return "#EXTM3U\n#EXTINF:-1," + fill("118+1 ", n) + "\nhttp://h.example/1\n";
            });
            b.ladder("M3U: many EXTVLCOPT lines", kSmallLadder, [](long long n) {
                return "#EXTM3U\n#EXTINF:-1,T\n" + fill("#EXTVLCOPT:http-user-agent=x\n", n) + "http://h.example/1\n";
            });
        };
        list.push_back(t);
    }
    {
        Target t = target("catchupUrl", Text, m3uFixtures, [](const QByteArray &d) {
            const int duration = fuzzDuration(d, 3600);
            const iptv::Playlist p = iptv::parseM3u(d);
            int n = 0;
            for (const iptv::Channel &c : p.channels)
            {
                if (++n > 2000)
                    break;
                catchups(c, duration);
                if (c.catchup == iptv::CatchupMode::Xtream)
                {
                    iptv::Channel shifted = c;
                    shifted.timeOffsetSecs = 50400;
                    catchups(shifted, duration);
                }
            }
        });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            const QByteArray channels =
                "#EXTM3U\n#EXTINF:-1 catchup=\"xc\" catchup-days=\"7\",X\nhttp://p.example/live/u/p/1.ts\n"
                "#EXTINF:-1 catchup=\"fs\" catchup-days=\"7\",F\nhttps://f.example/ch/index.m3u8\n"
                "#EXTINF:-1 catchup-source=\"https://a.example/{utc}-{duration:60}-{offset:1}.ts\",T\nhttps://t.example/\n"
                "#EXTINF:-1 catchup=\"append\" catchup-days=\"7\",A\nhttps://a.example/x.m3u8\n";
            for (const char *v : { "2147483647", "2147483600", "-2147483648", "-1", "0", "1" })
                b.add(QByteArray("#FUZZ-DURATION:") + v + "\n" + channels, std::string("catch-up of duration ") + v);
            for (const char *source : { "{duration:0}", "{duration:-1}", "{duration:9223372036854775807}",
                                        "{duration:-9223372036854775808}", "{duration:", "{offset:", "{duration:abc}",
                                        "{duration:1}{duration:", "{offset:0}{offset:-7}", "{Y}{m}{d}{H}{M}{S}",
                                        "${start}${end}${duration}${offset}${timestamp}" })
                b.add(QByteArray("#EXTM3U\n#EXTINF:-1 catchup-source=\"") + source + "\",T\nhttps://t.example/\n",
                      std::string("catch-up template ") + source);
            b.add("#EXTM3U\n#EXTINF:-1 catchup=\"xc\",X\nhttp://p.example/1.ts\n#EXTINF:-1 catchup=\"xc\",Y\n/\n",
                  "catch-up: Xtream addresses too short");
            b.add("#EXTM3U\n#EXTINF:-1 catchup=\"fs\",F\nhttps://f.example/\n#EXTINF:-1 catchup=\"fs\",G\n.m3u8\n",
                  "catch-up: Flussonic addresses without a name");
            b.ladder("catch-up template: many {duration:1}", kSmallLadder, [](long long n) {
                return "#EXTM3U\n#EXTINF:-1 catchup-source=\"" + fill("{duration:1}", n) + "\",T\nhttps://t.example/\n";
            });
            b.ladder("catch-up template: many {utc}", kSmallLadder, [](long long n) {
                return "#EXTM3U\n#EXTINF:-1 catchup-source=\"" + fill("{utc}", n) + "\",T\nhttps://t.example/\n";
            });
        };
        list.push_back(t);
    }
    {
        Target t = target("isChannelList", Text, m3uFixtures, [](const QByteArray &d) { iptv::isChannelList(d); });
        t.mutations = 500;
        list.push_back(t);
    }
    {
        Target t = target("displayGroup", Text, { "fuzz/groups.txt" }, [](const QByteArray &d) {
            forLines(d, 20000, [](const QString &line) { iptv::displayGroup(line); });
            iptv::displayGroup(QString::fromUtf8(d));
        });
        t.mutations = 500;
        list.push_back(t);
    }

    // ----- Live TV: guides
    {
        Target t = target("parseXmltv(QByteArray)", Xml, guideFixtures, [](const QByteArray &d) {
            useGuide(iptv::parseXmltv(d, filterAll()));
            useGuide(iptv::parseXmltv(d, filterSome()));
        });
        t.moreSeeds = gzipped;
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            auto programme = [](const QByteArray &start, const QByteArray &stop, const QByteArray &inside) {
                return "<tv><channel id=\"c\"><display-name>C</display-name></channel><programme start=\"" + start
                       + "\"" + (stop.isNull() ? QByteArray() : " stop=\"" + stop + "\"") + " channel=\"c\">"
                       + inside + "</programme></tv>";
            };
            for (const char *e : { "2147483647.2147483647.", "2147483647.0.", "0.2147483647.", "-2147483648.-2147483648.",
                                   "99999999999.1.", "1.99999999999.", "./.", "..", ".", "1/0.2/0.3/0", "\xFF.\xFF." })
                b.add(programme("20261025180000 +0000", "20261025190000 +0000",
                                QByteArray("<title>T</title><episode-num system=\"xmltv_ns\">") + e + "</episode-num>"),
                      std::string("XMLTV: xmltv_ns episode ") + e);
            static const char *const times[] = {
                "20261025180000 +2400", "20261025180000 +9999", "20261025180000 -9999", "20261025180000 +",
                "20261025180000 +00", "20261025180000 +0", "20261025180000+0130", "20261025180000 +01:30",
                "20261025180000 GMT", "20261025180000 -0000", "202610251800", "20261025180",
                "99991231235959 +0000", "99991231235959 -9999", "00010101000000 -2359", "00000000000000 +0000",
                "20261025180000 +-100", "20261025180000 ++++", "2026102518000a +0000", "+2026102518000 +0000",
                "20261025180000 -2147483648", "\xD9\xA2\xD9\xA0\xD9\xA2\xD9\xA6\x31\x30\x32\x35\x31\x38\x30\x30\x30\x30",
            };
            for (const char *tm : times)
            {
                b.add(programme(tm, "20261025190000 +0000", "<title>T</title>"), std::string("XMLTV: start ") + tm);
                b.add(programme("20261025180000 +0000", tm, "<title>T</title>"), std::string("XMLTV: stop ") + tm);
            }
            b.add(programme("20261025180000 +0000", QByteArray(), "<title>T</title>")
                      + programme("20261025180000 +0000", QByteArray(), "<title>U</title>"),
                  "XMLTV: two roots");
            b.lazy([] { return gzipBomb(4, 64); }, "XMLTV: a gzip bomb (256 KB to 256 MB) read in memory");
            b.ladder("XMLTV: a programme with many nested elements", kSmallLadder, [programme](long long n) {
                return programme("20261025180000 +0000", "20261025190000 +0000",
                                 "<title>T</title>" + fill("<x>", n / 2) + fill("</x>", n / 2));
            });
            b.ladder("XMLTV: many programmes of one channel", kSmallLadder, [](long long n) {
                return "<tv>" + numbered(n, [](long long i) {
                           return "<programme start=\"2026" + QByteArray::number(1010 + (i * 7919) % 20) + "180000 +0000\""
                                  " channel=\"c\"><title>P</title></programme>";
                       }) + "</tv>";
            });
            b.ladder("XMLTV: many channels", kSmallLadder, [](long long n) {
                return "<tv>" + numbered(n, [](long long i) {
                           return "<channel id=\"c" + num(i) + "\"><display-name>C " + num(i)
                                  + "</display-name></channel><programme start=\"20261025180000 +0000\" channel=\"c"
                                  + num(i) + "\"><title>P</title></programme>";
                       }) + "</tv>";
            });
            b.ladder("XMLTV: a long description", kLadder, [programme](long long n) {
                return programme("20261025180000 +0000", "20261025190000 +0000",
                                 "<title>T</title><desc>" + fill("words ", n) + "</desc>");
            });
            b.ladder("XMLTV: a long channel id", kLadder, [](long long n) {
                const QByteArray id = fill("ab-", n);
                return "<tv><channel id=\"" + id + "\"><display-name>" + id + "</display-name></channel>"
                       "<programme start=\"20261025180000 +0000\" channel=\"" + id + "\"><title>T</title></programme></tv>";
            });
        };
        list.push_back(t);
    }
    {
        Target t = target("parseXmltv(QIODevice)", Xml, guideFixtures, [](const QByteArray &d) {
            QByteArray copy = d;
            QBuffer buffer(&copy);
            buffer.open(QIODevice::ReadOnly);
            if (iptv::isGzip(d))
            {
                iptv::GunzipDevice inflated(&buffer, 64 << 20);
                inflated.open(QIODevice::ReadOnly);
                useGuide(iptv::parseXmltv(inflated, filterSome()));
            }
            else
                useGuide(iptv::parseXmltv(buffer, filterSome()));
        });
        t.moreSeeds = gzipped;
        t.mutations = 1500;
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.lazy([] { return gzipBomb(2, 64); }, "XMLTV stream: a gzip bomb (128 KB to 128 MB) of zeros");
            b.lazy([] { return gzipOf("<tv>" + QByteArray(128 << 20, ' ')); },
                   "XMLTV stream: gzip of 128 MB of spaces (limit 64 MB)");
        };
        list.push_back(t);
    }
    {
        Target t = target("parseXmltvFile", Xml, guideFixtures, [](const QByteArray &d) {
            const QString path = QString::fromStdString(S.work) + QStringLiteral("/guide-%1.xml").arg(getpid());
            {
                QFile f(path);
                if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    return;
                f.write(d);
            }
            useGuide(iptv::parseXmltvFile(path, filterAll()));
            QFile::remove(path);
        });
        t.moreSeeds = gzipped;
        t.mutations = 800;
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            // (the file is streamed through GunzipDevice, its limit 4 GB)
            b.lazy([] { return gzipOf("<tv>" + QByteArray(96 << 20, ' ')); },
                   "XMLTV file: gzip of 96 MB of spaces in one element (96 KB)");
            b.lazy([] { return gzipOf("<tv><!--" + QByteArray(96 << 20, 'x') + "--></tv>"); },
                   "XMLTV file: gzip of a 96 MB comment (96 KB)");
        };
        list.push_back(t);
    }
    {
        // a reading cancelled (VLC quitting) stops soon
        Target t = target("parseXmltv(cancelled)", Xml, guideFixtures, [](const QByteArray &d) {
            QByteArray copy = d;
            QBuffer buffer(&copy);
            buffer.open(QIODevice::ReadOnly);
            static const std::atomic<bool> stop { true };
            iptv::GuideFilter f;
            f.cancel = &stop;
            const long long t0 = nowNs();
            const iptv::Guide g = iptv::parseXmltv(buffer, f);
            const double secs = double(nowNs() - t0) / 1e9;
            if (secs > 0.5 && g.error != QLatin1String("cancelled"))
            {
                char buf[200];
                std::snprintf(buf, sizeof buf, "a cancelled reading went on for %.1f s over %s and was not cancelled",
                              secs, mb(d.size()).c_str());
                note(buf);
            }
        });
        t.mutations = 100;
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.ladder("XMLTV cancelled: one long comment", { 4 << 20, 16 << 20, 64 << 20 },
                     [](long long n) { return "<tv><!--" + QByteArray(qsizetype(n), 'x') + "--></tv>"; }, 0, false);
            b.ladder("XMLTV cancelled: one long text", { 4 << 20, 16 << 20, 64 << 20 },
                     [](long long n) { return "<tv>" + QByteArray(qsizetype(n), 'x') + "</tv>"; }, 0, false);
        };
        list.push_back(t);
    }
    {
        Target t = target("gunzip", Binary, {}, [](const QByteArray &d) {
            const qint64 limit = 16 << 20;
            iptv::isGzip(d);
            const QByteArray out = iptv::gunzip(d, limit);
            if (out.size() > limit)
                note("gunzip gave more than its limit");
            // (whole gzip data, one member: its trailer's CRC and size are those of what came out)
            auto whole = [](const QByteArray &gz, const QByteArray &inflated) {
                if (gz.size() < 18)
                    return false;
                const uchar *t = reinterpret_cast<const uchar *>(gz.constData() + gz.size() - 8);
                const quint32 crc = quint32(t[0]) | quint32(t[1]) << 8 | quint32(t[2]) << 16 | quint32(t[3]) << 24;
                const quint32 size = quint32(t[4]) | quint32(t[5]) << 8 | quint32(t[6]) << 16 | quint32(t[7]) << 24;
                return size == quint32(inflated.size())
                       && crc == quint32(crc32(0, reinterpret_cast<const Bytef *>(inflated.constData()), uInt(inflated.size())));
            };
            if (!out.isEmpty() && d.size() < 65536 && whole(d, out)
                && iptv::gunzip(d + QByteArray(8, '\0'), limit).isEmpty())
                note("gunzip gives nothing at all when zero bytes follow the gzip data (padding)");
        });
        t.moreSeeds = [](const std::vector<QByteArray> &) {
            return std::vector<QByteArray> {
                gzipOf(readFile(S.fixtures + QStringLiteral("/iptv-guide.xml"))),
                gzipOf(readFile(S.fixtures + QStringLiteral("/iptv-playlist.m3u")), 1),
                gzipOf(readFile(S.fixtures + QStringLiteral("/iptv-autoguide/iptv-org-channels.json"))),
                gzipOf(QByteArray()) + gzipOf("two members"),
            };
        };
        t.extra = [](Builder &b, const std::vector<QByteArray> &seeds) {
            const long long allow = 48 << 20;
            b.lazy([] { return gzipBomb(24, 1); }, "gzip bomb: 24 members of 1 MB (limit 16 MB)", allow);
            b.lazy([] { return gzipBomb(1, 64); }, "gzip bomb: one member of 64 MB (limit 16 MB)", allow);
            b.lazy([] { return gzipBomb(1, 16) + gzipBomb(1, 1); }, "gzip bomb: 17 MB in two members", allow);
            b.add(seeds[0] + "garbage", "gzip: garbage after the data");
            b.add(seeds[0].left(10), "gzip: the header alone");
            QByteArray name = seeds[0];
            name[3] = char(name[3] | 0x08);   // FNAME, never ended
            b.add(name, "gzip: a file name never ended");
            QByteArray extra = seeds[0];
            extra[3] = char(extra[3] | 0x04);   // FEXTRA, its length past the end
            extra.insert(10, bytes("\xFF\xFF", 2));
            b.add(extra, "gzip: an extra field past the end");
        };
        list.push_back(t);
    }
    {
        Target t = target("GunzipDevice", Binary, {}, [](const QByteArray &d) {
            const qint64 limit = 16 << 20;
            QByteArray copy = d;
            QBuffer source(&copy);
            source.open(QIODevice::ReadOnly);
            iptv::GunzipDevice inflated(&source, limit);
            inflated.open(QIODevice::ReadOnly);
            static char chunk[65536];
            qint64 total = 0;
            int reads = 0;
            for (; reads < 100000; ++reads)
            {
                const qint64 got = inflated.read(chunk, sizeof chunk);
                if (got <= 0)
                    break;
                total += got;
            }
            if (reads >= 100000)
                note("GunzipDevice never ends");
            if (total > limit + qint64(sizeof chunk))
                note("GunzipDevice read past its limit");
        });
        t.moreSeeds = [](const std::vector<QByteArray> &) {
            return std::vector<QByteArray> { gzipOf(readFile(S.fixtures + QStringLiteral("/iptv-guide.xml"))),
                                             gzipOf(QByteArray()) + gzipOf("two members") };
        };
        t.mutations = 1500;
        t.extra = [](Builder &b, const std::vector<QByteArray> &seeds) {
            const long long allow = 48 << 20;
            b.lazy([] { return gzipBomb(24, 1); }, "gzip bomb: 24 members of 1 MB (limit 16 MB)", allow);
            b.lazy([] { return gzipBomb(1, 64); }, "gzip bomb: one member of 64 MB (limit 16 MB)", allow);
            b.add(seeds[0] + QByteArray(8, '\0'), "gzip: zero bytes after the data");
        };
        list.push_back(t);
    }
    {
        Target t = target("parseXmltvTime", Text, { "fuzz/xmltv-times.txt" }, [](const QByteArray &d) {
            forLines(d, 20000, [](const QString &line) {
                iptv::parseXmltvTime(line);
                iptv::parseXmltvTime(QStringView(line).mid(1));
            });
        });
        list.push_back(t);
    }
    {
        Target t = target("guideKey", Text, { "fuzz/names.txt" }, [](const QByteArray &d) {
            forLines(d, 20000, [](const QString &line) { iptv::guideKey(line); });
        });
        t.mutations = 1000;
        list.push_back(t);
    }

    // ----- Live TV: Xtream Codes
    {
        Target t = target("xtream::parseAccountInfo", Json, { "fuzz/xtream-account.json" },
                          [](const QByteArray &d) { xtream::parseAccountInfo(d); });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.add(R"({"user_info":{"auth":1},"server_info":{"timestamp_now":1,"time_now":"2100-01-01 00:00:00"}})",
                  "Xtream: the server's clock 70 years from its timestamp");
            b.add(R"({"user_info":{"auth":1},"server_info":{"timestamp_now":4102444800,"time_now":"1970-01-01 00:00:00"}})",
                  "Xtream: the server's clock 70 years before its timestamp");
            b.add(R"({"user_info":{"auth":1},"server_info":{"timestamp_now":9223372036854775807,"time_now":"2026-10-25 21:00:00"}})",
                  "Xtream: timestamp_now 2^63");
            b.add(R"({"user_info":{"auth":1e300,"exp_date":-1e300,"max_connections":1e19}})", "Xtream: numbers past 64 bits");
            b.add(R"({"user_info":{"auth":"1","exp_date":"99999999999999999999","max_connections":"4294967297"}})",
                  "Xtream: number strings past 64 and 32 bits");
        };
        list.push_back(t);
    }
    {
        Target t = target("xtream::parseCategories", Json, { "fuzz/xtream-categories.json" },
                          [](const QByteArray &d) { xtream::parseCategories(d); });
        list.push_back(t);
    }
    auto xtreamItems = [](const char *idKey) {
        return [idKey](long long n) {
            return "[" + numbered(n, [idKey](long long i) {
                       return QByteArray("{\"") + idKey + "\":\"" + num(i) + "\",\"name\":\"N\",\"category_id\":\""
                              + num(i) + "\",\"category_name\":\"G" + num(i) + "\"}";
                   }, ",") + "]";
        };
    };
    {
        Target t = target("xtream::parseLiveStreams", Json, { "fuzz/xtream-live.json" }, [](const QByteArray &d) {
            QVector<xtream::Category> categories = fixedCategories();
            categories += xtream::parseCategories(d);
            const QVector<iptv::Channel> channels = xtream::parseLiveStreams(d, account(), categories, fixedInfo());
            xtream::AccountInfo tsOnly = fixedInfo();
            tsOnly.outputFormats = { QStringLiteral("ts") };
            xtream::parseLiveStreams(d, account(), {}, tsOnly);
            int n = 0;
            for (const iptv::Channel &c : channels)
                if (c.catchup != iptv::CatchupMode::None && ++n < 200)
                    iptv::catchupUrl(c, now().addSecs(-3600), 1800, now());
        });
        t.extra = [xtreamItems](Builder &b, const std::vector<QByteArray> &) {
            b.ladder("Xtream: many channels, each of its own category", kSmallLadder, xtreamItems("stream_id"));
            b.add(R"([{"stream_id":1e300,"name":"x","tv_archive":1,"tv_archive_duration":1e300}])", "Xtream: ids 1e300");
            b.add(R"({"0":{"stream_id":1},"1":{"stream_id":2},"x":[]})", "Xtream: an object keyed by index");
        };
        list.push_back(t);
    }
    {
        Target t = target("xtream::parseVodStreams", Json, { "fuzz/xtream-vod.json" }, [](const QByteArray &d) {
            QVector<xtream::Category> categories = fixedCategories();
            categories += xtream::parseCategories(d);
            xtream::parseVodStreams(d, account(), categories);
        });
        t.extra = [xtreamItems](Builder &b, const std::vector<QByteArray> &) {
            b.ladder("Xtream: many films, each of its own category", kSmallLadder, xtreamItems("stream_id"));
        };
        list.push_back(t);
    }
    {
        Target t = target("xtream::parseSeries", Json, { "fuzz/xtream-series.json" }, [](const QByteArray &d) {
            QVector<xtream::Category> categories = fixedCategories();
            categories += xtream::parseCategories(d);
            xtream::parseSeries(d, categories);
        });
        list.push_back(t);
    }
    {
        Target t = target("xtream::parseSeriesInfo", Json,
                          { "fuzz/xtream-series-info.json", "fuzz/xtream-series-info-array.json" },
                          [](const QByteArray &d) { xtream::parseSeriesInfo(d, account()); });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.ladder("Xtream: a series of many episodes", kSmallLadder, [](long long n) {
                return "{\"episodes\":{\"1\":[" + numbered(n, [](long long i) {
                           return "{\"id\":\"" + num(i) + "\",\"season\":" + num(i % 7) + ",\"episode_num\":"
                                  + num((i * 7919) % 1000) + "}";
                       }, ",") + "]}}";
            });
        };
        list.push_back(t);
    }
    {
        Target t = target("xtream::accountFromM3uUrl", Text, { "fuzz/addresses.txt" }, [](const QByteArray &d) {
            forLines(d, 2000, [](const QString &line) {
                const xtream::Account a = xtream::accountFromM3uUrl(line);
                xtream::normalizeServer(line);
                xtream::apiUrl(a, QStringLiteral("get_live_streams"), { { QStringLiteral("x"), line } });
            });
        });
        t.mutations = 1000;
        list.push_back(t);
    }

    // ----- Live TV: the user's servers and tuners
    const std::string srv = "iptv-servers/";
    {
        Target t = target("servers::parseHdhrDiscover", Json, { srv + "hdhr-discover.json" },
                          [](const QByteArray &d) { servers::parseHdhrDiscover(d); });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseHdhrCloudDiscover", Json, { srv + "hdhr-cloud-discover.json" },
                          [](const QByteArray &d) { servers::parseHdhrCloudDiscover(d); });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseHdhrLineup", Json, { srv + "hdhr-lineup.json" },
                          [](const QByteArray &d) { roundTripM3u(servers::parseHdhrLineup(d)); });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseHdhrGuide", Json, { srv + "hdhr-guide.json" }, [](const QByteArray &d) {
            static const QVector<servers::Channel> lineup =
                servers::parseHdhrLineup(readFile(S.fixtures + QStringLiteral("/iptv-servers/hdhr-lineup.json")));
            roundTripXmltv(lineup, servers::parseHdhrGuide(d));
        });
        list.push_back(t);
    }
    {
        Target t = target("servers::isHdhrDiscoverReply", Binary, {},
                          [](const QByteArray &d) { servers::isHdhrDiscoverReply(d); });
        t.moreSeeds = [](const std::vector<QByteArray> &) {
            const QByteArray tags = bytes("\x01\x04\x00\x00\x00\x01", 6) + bytes("\x02\x04\x12\x34\x56\x78", 6)
                                    + "\x2A\x16" + "http://192.168.1.20:80";
            return std::vector<QByteArray> { hdhrPacket(3, tags), servers::hdhrDiscoverPacket() };
        };
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.add(be16(3) + be16(0xFFFF) + QByteArray(8, '\0'), "HDHomeRun: a length past the end");
            b.add(be16(3) + be16(0), "HDHomeRun: 4 bytes");
            b.add(hdhrPacket(3, QByteArray(65531, 'x')), "HDHomeRun: the largest reply");
            b.add(hdhrPacket(3, QByteArray(70000, 'x')), "HDHomeRun: a length that wraps");
        };
        list.push_back(t);
    }
    {
        Target t = target("servers::parseMediaBrowserLogin", Json,
                          { srv + "jellyfin-login.json", srv + "jellyfin-login-no-token.json" },
                          [](const QByteArray &d) { servers::parseMediaBrowserLogin(d); });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseMediaBrowserChannels", Json, { srv + "jellyfin-channels.json" },
                          [](const QByteArray &d) {
                              for (const bool emby : { false, true })
                                  roundTripM3u(servers::parseMediaBrowserChannels(
                                      d, QStringLiteral("http://media.example:8096"), emby));
                          });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseMediaBrowserProgrammes", Json, { srv + "jellyfin-programs.json" },
                          [](const QByteArray &d) {
                              static const QVector<servers::Channel> channels = servers::parseMediaBrowserChannels(
                                  readFile(S.fixtures + QStringLiteral("/iptv-servers/jellyfin-channels.json")),
                                  QStringLiteral("http://media.example:8096"), false);
                              roundTripXmltv(channels, servers::parseMediaBrowserProgrammes(d));
                          });
        list.push_back(t);
    }
    {
        Target t = target("servers::mediaBrowserStreamUrl", Json,
                          { srv + "jellyfin-playbackinfo-direct.json", srv + "jellyfin-playbackinfo-hls.json",
                            srv + "jellyfin-playbackinfo-transcode.json" },
                          [](const QByteArray &d) {
                              for (const bool emby : { false, true })
                              {
                                  const QString url = servers::mediaBrowserStreamUrl(
                                      d, QStringLiteral("http://media.example:8096"), QStringLiteral("abc123"),
                                      QStringLiteral("secret-token"), emby);
                                  const QUrl u(url);
                                  if (u.host().isEmpty() || u.host() == QLatin1String("media.example"))
                                      continue;
                                  const QString transcoding = QJsonDocument::fromJson(d).object()
                                                                  .value(QLatin1String("MediaSources")).toArray().at(0)
                                                                  .toObject().value(QLatin1String("TranscodingUrl"))
                                                                  .toVariant().toString().trimmed();
                                  if (transcoding.startsWith(QLatin1String("//")) && url.contains(QLatin1String("secret-token")))
                                      note("a TranscodingUrl starting with // sends the stream, with the token, to "
                                           "another host" + std::string(emby ? " (Emby)" : " (Jellyfin)"));
                              }
                          });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.add(R"({"MediaSources":[{"SupportsDirectStream":false,"TranscodingUrl":"//evil.example/videos/x/master.m3u8?a=1","LiveStreamId":"l"}],"PlaySessionId":"s"})",
                  "Jellyfin: a TranscodingUrl of another host (//host/...)");
            b.add(R"({"MediaSources":[{"SupportsDirectStream":false,"TranscodingUrl":"/\\evil.example/x"}]})",
                  "Jellyfin: a TranscodingUrl /\\host");
        };
        list.push_back(t);
    }
    {
        Target t = target("servers::parseMediaBrowserDiscoverReply", Json,
                          { srv + "jellyfin-discover-reply.json", srv + "emby-discover-reply.json" },
                          [](const QByteArray &d) {
                              servers::parseMediaBrowserDiscoverReply(d, false);
                              servers::parseMediaBrowserDiscoverReply(d, true);
                          });
        list.push_back(t);
    }
    {
        Target t = target("servers::tvheadendWithUser", Text, { srv + "tvheadend-playlist.m3u" }, [](const QByteArray &d) {
            const QByteArray out = servers::tvheadendWithUser(d, QStringLiteral("user"), QStringLiteral("p@ss:w/rd#?"));
            iptv::parseM3u(out);
        });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseNextPvrInitiate", Json, { srv + "nextpvr-initiate.json" }, [](const QByteArray &d) {
            const servers::Salted s = servers::parseNextPvrInitiate(d);
            servers::nextPvrLoginKey(QStringLiteral("0000"), s.salt);
        });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseNextPvrLogin", Json,
                          { srv + "nextpvr-login-ok.json", srv + "nextpvr-login-fail.json" },
                          [](const QByteArray &d) { servers::parseNextPvrLogin(d); });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseNextPvrChannels", Json, { srv + "nextpvr-channels.json" }, [](const QByteArray &d) {
            roundTripM3u(servers::parseNextPvrChannels(d, QStringLiteral("http://pvr.example:8866"), QStringLiteral("sid")));
        });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseNextPvrListings", Json, { srv + "nextpvr-listings.json" }, [](const QByteArray &d) {
            servers::Channel c;
            c.id = QStringLiteral("7");
            c.name = QStringLiteral("Seven");
            roundTripXmltv({ c }, servers::parseNextPvrListings(d, QStringLiteral("7")));
        });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseMdnsAnswer", Binary,
                          { srv + "mdns-channels-reply.bin", srv + "mdns-other-reply.bin" },
                          [](const QByteArray &d) { servers::parseMdnsAnswer(d, channelsService); });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            const QByteArray question = dnsName("_channels_dvr._tcp.local") + be16(12) + be16(1);
            b.add(dnsHeader(0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF), "mDNS: every count 65535, nothing after");
            b.add(dnsHeader(1, 0) + bytes("\xC0\x0C", 2) + be16(12) + be16(1), "mDNS: a question pointing at itself");
            b.add(dnsHeader(0, 1) + dnsRecord(bytes("\xC0\x0C", 2), 12, bytes("\xC0\x0C", 2)),
                  "mDNS: a record pointing at itself");
            b.add(dnsHeader(0, 1) + dnsRecord(bytes("\xC0\xFF", 2), 12, bytes("\xC0\x0C", 2)), "mDNS: a pointer past the end");
            b.add(dnsHeader(0, 1) + dnsRecord(bytes("\xC0\x00", 2), 1, bytes("\x7F\0\0\x01", 4)),
                  "mDNS: a pointer into the header");
            b.add(dnsHeader(0, 1) + bytes("\x3F", 1) + "abc", "mDNS: a label past the end");
            b.add(dnsHeader(0, 1) + bytes("\x40", 1) + QByteArray(64, 'a') + bytes("\0", 1) + be16(1),
                  "mDNS: a label of the reserved kind 0x40");
            b.add(dnsHeader(0, 1) + bytes("\x80", 1) + QByteArray(128, 'a') + bytes("\0", 1) + be16(1) + be16(1)
                      + bytes("\0\0\0\0", 4) + be16(4) + bytes("\x7F\0\0\x01", 4),
                  "mDNS: a label of the reserved kind 0x80");
            b.add(dnsHeader(1, 1) + question
                      + dnsRecord(bytes("\xC0\x0C", 2), 12, bytes("\x04" "Demo", 5) + bytes("\xC0\x0C", 2))
                      + dnsRecord(bytes("\x04" "Demo\xC0\x0C", 7), 33, bytes("\0\0\0\0\x1F\x99", 6) + bytes("\xC0\xFF", 2)),
                  "mDNS: a service whose host points past the end");
            b.add(dnsHeader(1, 1) + question + dnsRecord(bytes("\xC0\x0C", 2), 33, QByteArray(7, '\xC0')),
                  "mDNS: an SRV of 7 bytes of pointers");
            b.add(dnsHeader(1, 1) + question + bytes("\xC0\x0C", 2) + be16(33) + be16(1) + bytes("\0\0\0\0", 4)
                      + be16(0xFFFF) + QByteArray(10, '\0'),
                  "mDNS: a record's length past the end");
            b.add(dnsHeader(1, 1) + question + bytes("\xC0\x0C", 2) + be16(1) + be16(1) + bytes("\0\0\0\0", 4) + be16(4)
                      + bytes("\x0A\0\0", 3),
                  "mDNS: an address cut short");
            {
                // 31 pointers one after the other, then a name: the most it follows
                QByteArray p = dnsHeader(0, 1);
                const int first = int(p.size());
                for (int i = 0; i < 31; ++i)
                    p += bytes("\xC0", 1) + char(first + 2 * (i + 1));
                p += dnsName("x.local");
                QByteArray chain = dnsHeader(0, 1) + p.mid(12);
                chain += dnsRecord(bytes("\xC0\x0C", 2), 1, bytes("\x7F\0\0\x01", 4));
                b.add(chain, "mDNS: 31 pointers in a row");
            }
            // one long name, read again by every record of the packet
            b.ladder("mDNS: a long name read by every record", { 4 << 10, 16 << 10, 64 << 10 }, [](long long n) {
                QByteArray name;
                while (name.size() + 64 < n / 2)
                    name += char(63) + QByteArray(63, 'a');
                name += '\0';
                QByteArray records = dnsRecord(name, 16, QByteArray(1, '\0'));
                int count = 1;
                while (12 + records.size() + 14 <= n && count < 65535)
                {
                    records += dnsRecord(bytes("\xC0\x0C", 2), 12, bytes("\xC0\x0C", 2));
                    ++count;
                }
                return dnsHeader(0, count) + records;
            });
            b.ladder("mDNS: many instances of the service", { 4 << 10, 16 << 10, 64 << 10 }, [question](long long n) {
                QByteArray records;
                int count = 0;
                while (12 + question.size() + records.size() + 24 <= n && count < 65535)
                {
                    const QByteArray label = "i" + num(count);
                    records += dnsRecord(bytes("\xC0\x0C", 2), 12, char(label.size()) + label + bytes("\xC0\x0C", 2));
                    ++count;
                }
                return dnsHeader(1, count) + question + records;
            });
        };
        list.push_back(t);
    }
    {
        Target t = target("servers::parseSdToken", Json, { srv + "sd-token-ok.json", srv + "sd-token-refused.json" },
                          [](const QByteArray &d) { servers::parseSdToken(d); });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseSdLineups", Json, { srv + "sd-lineups.json", srv + "sd-lineups-none.json" },
                          [](const QByteArray &d) {
                              int code = 0;
                              servers::parseSdLineups(d, &code);
                              servers::parseSdLineups(d, nullptr);
                          });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseSdLineup", Json, { srv + "sd-lineup.json" },
                          [](const QByteArray &d) { roundTripM3u(servers::parseSdLineup(d)); });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseSdSchedules", Json, { srv + "sd-schedules.json" },
                          [](const QByteArray &d) { servers::parseSdSchedules(d); });
        list.push_back(t);
    }
    {
        Target t = target("servers::parseSdPrograms", Json, { srv + "sd-programs.json" }, [](const QByteArray &d) {
            const QHash<QString, servers::Programme> programmes = servers::parseSdPrograms(d);
            QVector<servers::Programme> list;
            qint64 start = now().toSecsSinceEpoch();
            for (auto it = programmes.cbegin(); it != programmes.cend(); ++it)
            {
                servers::Programme p = it.value();
                p.channel = QStringLiteral("10001");
                p.start = start;
                p.stop = start += 1800;
                list.append(p);
            }
            servers::Channel c;
            c.id = QStringLiteral("10001");
            c.name = QStringLiteral("Station");
            roundTripXmltv({ c }, list);
        });
        list.push_back(t);
    }
    {
        Target t = target("servers::baseUrl", Text, { "fuzz/addresses.txt" }, [](const QByteArray &d) {
            forLines(d, 2000, [](const QString &line) {
                servers::baseUrl(line, 8096);
                servers::baseUrl(line, 0);
            });
        });
        t.mutations = 1000;
        list.push_back(t);
    }

    // ----- Live TV: the automatic guide
    const std::string ag = "iptv-autoguide/";
    {
        Target t = target("autoguide::parseIptvOrgChannels", Json, { ag + "iptv-org-channels.json" },
                          [](const QByteArray &d) {
                              const QVector<autoguide::Known> known = autoguide::parseIptvOrgChannels(d);
                              const QVector<autoguide::Known> back =
                                  autoguide::readCompactChannels(autoguide::compactChannels(known));
                              if (back.size() != known.size())
                                  note("the compact copy of the channels reads back as " + std::to_string(back.size())
                                       + " channels, not " + std::to_string(known.size()));
                          });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.ladder("iptv-org: many channels named (((...", kSmallLadder, [](long long n) {
                return "[" + numbered(n, [](long long i) {
                           return "{\"id\":\"C" + num(i) + ".uk\",\"name\":\"" + fill("(", 200) + "\",\"country\":\"UK\"}";
                       }, ",") + "]";
            });
        };
        list.push_back(t);
    }
    {
        Target t = target("autoguide::readCompactChannels", Json, {}, [](const QByteArray &d) {
            autoguide::readCompactChannels(d);
        });
        t.moreSeeds = [](const std::vector<QByteArray> &) {
            return std::vector<QByteArray> { autoguide::compactChannels(autoguide::parseIptvOrgChannels(
                readFile(S.fixtures + QStringLiteral("/iptv-autoguide/iptv-org-channels.json")))) };
        };
        list.push_back(t);
    }
    {
        Target t = target("autoguide::matchName", Text, { "fuzz/names.txt" }, [](const QByteArray &d) {
            forLines(d, 20000, [](const QString &line) {
                QString country;
                int plus = 0;
                autoguide::matchName(line, &country, &plus);
                autoguide::countryOfId(line);
            });
        });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.ladder("matchName: ( never closed", kLadder, [](long long n) { return QByteArray(qsizetype(n), '('); });
            b.ladder("matchName: [ never closed", kLadder, [](long long n) { return QByteArray(qsizetype(n), '['); });
            b.ladder("matchName: +1+1+1...", kLadder, [](long long n) { return fill("+1", n); });
            b.ladder("matchName: UK: UK: UK: ...", kLadder, [](long long n) { return fill("UK: ", n); });
            b.ladder("matchName: combining accents", kLadder, [](long long n) { return fill("e\xCC\x81", n); });
        };
        list.push_back(t);
    }
    {
        Target t = target("autoguide::Matcher", Json, { ag + "iptv-org-channels.json" }, [](const QByteArray &d) {
            const QVector<autoguide::Known> known = autoguide::parseIptvOrgChannels(d);
            const autoguide::Matcher matcher(known);
            int n = 0;
            for (const autoguide::Known &k : known)
            {
                if (++n > 3000)
                    break;
                matcher.match(k.id, k.name, QStringLiteral("GB"), QStringLiteral("US"));
                matcher.match(QString(), k.name + QStringLiteral(" +1"), QString(), QStringLiteral("IL"));
                matcher.match(QStringLiteral("x.uk@HD"), k.name, QString(), QString());
                for (const QString &alt : k.altNames)
                    matcher.match(QString(), QStringLiteral("UK: ") + alt + QStringLiteral(" HD"), QString(), QString());
            }
        });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.ladder("Matcher: many channels of one name", kSmallLadder, [](long long n) {
                return "[" + numbered(n, [](long long i) {
                           return "{\"id\":\"Same" + num(i) + "." + QByteArray(1, char('a' + i % 26))
                                  + QByteArray(1, char('a' + (i / 26) % 26)) + "\",\"name\":\"Same\",\"country\":\""
                                  + QByteArray(1, char('A' + i % 26)) + QByteArray(1, char('A' + (i / 26) % 26)) + "\"}";
                       }, ",") + "]";
            });
        };
        list.push_back(t);
    }
    {
        Target t = target("autoguide::pickGuides", Json, { ag + "catalog-guides.json" }, [](const QByteArray &d) {
            const QJsonDocument doc = QJsonDocument::fromJson(d);
            const QJsonArray sources = doc.isArray() ? doc.array() : doc.object().value(QLatin1String("sources")).toArray();
            QHash<QString, int> byCountry = { { QStringLiteral("GB"), 9 }, { QStringLiteral("US"), 5 },
                                              { QStringLiteral("IL"), 3 }, { QStringLiteral("FR"), 2 },
                                              { QStringLiteral("DE"), 2 }, { QStringLiteral("IT"), 1 } };
            for (const QJsonValue v : sources)
            {
                for (const QJsonValue c : v.toObject().value(QLatin1String("countries")).toArray())
                    if (byCountry.size() < 300 && !byCountry.contains(c.toString()))
                        byCountry.insert(c.toString(), 3);
            }
            const QString first = sources.at(0).toObject().value(QLatin1String("url")).toString();
            autoguide::pickGuides(sources, byCountry, {}, 4, 2);
            autoguide::pickGuides(sources, byCountry, { first }, 1, 1);
            autoguide::pickGuides(sources, byCountry, {}, 100, 100);
        });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.ladder("pickGuides: many guides of hosts it does not rank", kSmallLadder, [](long long n) {
                return "{\"sources\":[" + numbered(n, [](long long i) {
                           return "{\"kind\":\"epg\",\"url\":\"https://other" + num(i) + ".example/g.xml\",\"countries\":[\""
                                  + QByteArray(1, char('A' + i % 26)) + QByteArray(1, char('A' + (i / 26) % 12)) + "\"]}";
                       }, ",") + "]}";
            });
        };
        list.push_back(t);
    }

    // ----- Subtitle services
    const std::string ss = "subtitle-services/";
    auto both = [](const std::function<void(const subs::Query &)> &f) {
        f(filmQuery());
        f(episodeQuery());
    };
    {
        Target t = target("subs::subSourceMovieId", Json,
                          { ss + "subsource-movies.json", ss + "subsource-series.json", ss + "subsource-empty.json",
                            ss + "subsource-key-refused.json" },
                          [both](const QByteArray &d) {
                              both([&](const subs::Query &q) {
                                  QString error;
                                  subs::subSourceMovieId(d, q, &error);
                              });
                          });
        list.push_back(t);
    }
    {
        Target t = target("subs::parseSubSource", Json,
                          { ss + "subsource-subtitles.json", ss + "subsource-empty.json", ss + "subsource-key-refused.json" },
                          [both](const QByteArray &d) {
                              both([&](const subs::Query &q) {
                                  QString error;
                                  checkResults(subs::parseSubSource(d, q, languageOfName, &error));
                              });
                          });
        list.push_back(t);
    }
    {
        Target t = target("subs::parseSubDl", Json,
                          { ss + "subdl-movie.json", ss + "subdl-episode.json", ss + "subdl-not-found.json",
                            ss + "subdl-v1-key-refused.json", ss + "subdl-v2-key-refused.json" },
                          [both](const QByteArray &d) {
                              both([&](const subs::Query &q) {
                                  QString error;
                                  checkResults(subs::parseSubDl(d, q, languageOfName, &error));
                              });
                          });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.add(R"({"status":true,"subtitles":[{"url":"/subtitle/1-2.zip","language":"EN","rating":"nan","downloads":5}]})",
                  "SubDL: a rating \"nan\"");
            b.add(R"({"status":true,"subtitles":[{"url":"/subtitle/1-2.zip","language":"EN","rating":"-inf","downloads":2147483648}]})",
                  "SubDL: a rating \"-inf\"");
        };
        list.push_back(t);
    }
    {
        Target t = target("subs::parseOpenSubtitlesCom", Json, { ss + "oscom-search.json", ss + "oscom-key-refused.json" },
                          [](const QByteArray &d) { checkResults(subs::parseOpenSubtitlesCom(d, languageOfName)); });
        list.push_back(t);
    }
    {
        Target t = target("subs::openSubtitlesComSession", Json,
                          { ss + "oscom-login.json", ss + "oscom-login-other-host.json", ss + "oscom-key-refused.json" },
                          [](const QByteArray &d) {
                              const subs::Session s = subs::openSubtitlesComSession(d);
                              if (!s.base.isEmpty() && s.base.host() != QLatin1String("api.opensubtitles.com")
                                  && s.base.host() != QLatin1String("vip-api.opensubtitles.com"))
                                  note("openSubtitlesComSession took the host " + s.base.host().toStdString());
                          });
        list.push_back(t);
    }
    {
        Target t = target("subs::openSubtitlesComLink", Json, { ss + "oscom-download.json", ss + "oscom-download-http.json" },
                          [](const QByteArray &d) { subs::openSubtitlesComLink(d); });
        list.push_back(t);
    }
    {
        Target t = target("subs::titloviSession", Json, { ss + "titlovi-login.json" },
                          [](const QByteArray &d) { subs::titloviSession(d); });
        list.push_back(t);
    }
    {
        Target t = target("subs::parseTitlovi", Json, { ss + "titlovi-search.json" }, [both](const QByteArray &d) {
            both([&](const subs::Query &q) { checkResults(subs::parseTitlovi(d, q)); });
        });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.add(R"({"SubtitleResults":[{"Id":1,"Link":"https://titlovi.com/x","Lang":"Hrvatski","Rating":"nan"}]})",
                  "Titlovi: a rating \"nan\"");
        };
        list.push_back(t);
    }
    {
        Target t = target("subs::parseAssrt", Json, { ss + "assrt-search.json", ss + "assrt-invalid-token.json" },
                          [both](const QByteArray &d) {
                              both([&](const subs::Query &q) {
                                  QString error;
                                  bool refused = false;
                                  checkResults(subs::parseAssrt(d, q, &error, &refused));
                              });
                          });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.add(R"({"status":0,"sub":{"subs":[{"id":5,"lang":{"langlist":{"langeng":true}},"vote_score":"nan"}]}})",
                  "Assrt: a vote_score \"nan\"");
        };
        list.push_back(t);
    }
    {
        Target t = target("subs::assrtFileOf", Json, { ss + "assrt-detail.json" }, [](const QByteArray &d) {
            for (const char *l : { "zh", "zh-TW", "en", "fr" })
                subs::assrtFileOf(d, QString::fromLatin1(l));
        });
        list.push_back(t);
    }
    {
        Target t = target("subs::jimakuEntryId", Json, { ss + "jimaku-entries.json" }, [both](const QByteArray &d) {
            both([&](const subs::Query &q) { subs::jimakuEntryId(d, q); });
        });
        list.push_back(t);
    }
    {
        Target t = target("subs::parseJimaku", Json, { ss + "jimaku-files.json", ss + "jimaku-unauthorized.json" },
                          [both](const QByteArray &d) {
                              both([&](const subs::Query &q) { checkResults(subs::parseJimaku(d, q)); });
                          });
        list.push_back(t);
    }
    {
        Target t = target("subs::betaSeriesShowId", Json, { ss + "betaseries-show.json" },
                          [](const QByteArray &d) { subs::betaSeriesShowId(d); });
        list.push_back(t);
    }
    {
        Target t = target("subs::parseBetaSeries", Json,
                          { ss + "betaseries-episodes.json", ss + "betaseries-bad-key.json", ss + "betaseries-not-found.json" },
                          [](const QByteArray &d) {
                              QString error;
                              bool refused = false;
                              checkResults(subs::parseBetaSeries(d, &error, &refused));
                          });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.add(R"({"episode":{"subtitles":[{"id":1,"url":"https://www.betaseries.com/x","language":"VF","quality":"nan"}]}})",
                  "BetaSeries: a quality \"nan\"");
        };
        list.push_back(t);
    }
    {
        Target t = target("subs::parseSubsRo", Json, { ss + "subsro-search.json", ss + "subsro-invalid-key.json" },
                          [both](const QByteArray &d) {
                              both([&](const subs::Query &q) {
                                  QString error;
                                  checkResults(subs::parseSubsRo(d, q, &error));
                              });
                          });
        list.push_back(t);
    }
    {
        Target t = target("subs::parseNapisy24", Text,
                          { ss + "napisy24-found.bin", ss + "napisy24-login-error.txt", ss + "napisy24-video-info.txt" },
                          [](const QByteArray &d) { subs::parseNapisy24(d); });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.ladder("Napisy24: OK-2 and many |", kLadder, [](long long n) { return "OK-2" + QByteArray(qsizetype(n), '|'); });
            b.add("OK-2|imdb:" + QByteArray(4096, '9') + "||PK", "Napisy24: an IMDb id of 4096 digits");
            b.add("OK-2|imdb:-5|ftitle:||", "Napisy24: a negative IMDb id");
        };
        list.push_back(t);
    }
    {
        Target t = target("subs::legendasSession", Json,
                          { ss + "legendas-login.json", ss + "legendas-login-refused.json" },
                          [](const QByteArray &d) { subs::legendasSession(d); });
        list.push_back(t);
    }
    {
        Target t = target("subs::parseLegendas", Json, { ss + "legendas-movies.json", ss + "legendas-tv.json" },
                          [both](const QByteArray &d) {
                              both([&](const subs::Query &q) {
                                  for (const subs::Result &r : subs::parseLegendas(d, q))
                                      if (r.url.host() != QLatin1String("legendas.net"))
                                          note("a Legendas.net download on another host: " + r.url.host().toStdString());
                              });
                          });
        list.push_back(t);
    }
    {
        Target t = target("subs::parseYify", Text, { ss + "yify-movie.html" }, [](const QByteArray &d) {
            checkResults(subs::parseYify(d, languageOfName));
        });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.ladder("YIFY: rows never closed", kSmallLadder, [](long long n) { return fill("<tr data-id=\"1\">", n); });
            b.ladder("YIFY: links never closed in a row", kSmallLadder, [](long long n) {
                return "<tr data-id=\"1\">" + fill("<a href=\"/subtitles/", n) + "</tr>";
            });
            b.ladder("YIFY: a row of rating labels never closed", kSmallLadder, [](long long n) {
                return "<tr data-id=\"1\"><a href=\"/subtitles/x\">x</a><span class=\"sub-lang\">English</span>"
                       + fill("<span class=\"label", n) + "</tr>";
            });
            b.ladder("YIFY: many rows", kSmallLadder, [](long long n) {
                return fill("<tr data-id=\"7\"><td><span class=\"label label-success\">12</span></td><td><span "
                            "class=\"sub-lang\">English</span></td><td><a href=\"/subtitles/the-matrix-english\">"
                            "<span class=\"text-muted\">subtitle</span> The.Matrix.1999</a></td></tr>\n", n);
            });
            b.add("<tr data-id=\"1\"><a href=\"/subtitles/x\">x</a><span class=\"sub-lang\">English</span>"
                  "<span class=\"label\">99999999999999999999</span></tr>",
                  "YIFY: a rating of 20 digits");
        };
        list.push_back(t);
    }
    {
        Target t = target("subs::gunzip", Binary, {}, [](const QByteArray &d) {
            const qsizetype limit = 8 << 20;
            if (subs::gunzip(d, limit).size() > limit)
                note("SubtitleProviders::gunzip gave more than its limit");
        });
        t.moreSeeds = [](const std::vector<QByteArray> &) {
            return std::vector<QByteArray> { gzipOf(readFile(S.fixtures + QStringLiteral("/timing.srt"))),
                                             gzipOf(QByteArray()) };
        };
        t.extra = [](Builder &b, const std::vector<QByteArray> &seeds) {
            const long long allow = 24 << 20;
            b.lazy([] { return gzipBomb(1, 64); }, "gzip bomb: one member of 64 MB (limit 8 MB)", allow);
            b.lazy([] { return gzipBomb(1, 8); }, "gzip: exactly 8 MB (the limit)", allow);
            b.add(seeds[0] + seeds[0], "gzip: two members");
            b.add(seeds[0] + QByteArray(8, '\0'), "gzip: zero bytes after the data");
        };
        list.push_back(t);
    }
    {
        Target t = target("subs::unzip", Binary, {}, [](const QByteArray &d) {
            qsizetype total = 0;
            const QList<QPair<QString, QByteArray>> files = subs::unzip(d);
            for (const auto &f : files)
            {
                total += f.second.size();
                subs::isSubtitleName(f.first);
                if (f.second.size() > (8 << 20))
                    note("unzip gave a file past 8 MiB");
            }
            if (total > (16 << 20))
                note("unzip gave more than 16 MiB in all");
            if (files.size() > 1000)
                note("unzip listed more than 1000 files");
        });
        t.moreSeeds = [](const std::vector<QByteArray> &) {
            const QByteArray srt = readFile(S.fixtures + QStringLiteral("/timing.srt"));
            return std::vector<QByteArray> {
                zipOf({ { "Movie.srt", srt, 0 } }),
                zipOf({ { "dir/", {}, 0 }, { "__MACOSX/._Movie.srt", "x", 0 }, { "Movie.en.srt", srt, 8 },
                        { "readme.nfo", "nfo", 8 }, { "Movie.he.ass", srt, 8 } }, "a comment"),
            };
        };
        t.extra = [](Builder &b, const std::vector<QByteArray> &seeds) {
            const QByteArray eocd = le32(0x06054b50) + le16(0) + le16(0);
            b.add(eocd + le16(0xFFFF) + le16(0xFFFF) + le32(0) + le32(0) + le16(0), "zip: an end that counts 65535 files, nothing else");
            b.add(eocd + le16(1) + le16(1) + le32(46) + le32(0xFFFFFFFFu) + le16(0), "zip: its directory past the end");
            b.add(seeds[0] + "PK\x05\x06", "zip: an end's signature cut short after the real end");
            b.add(seeds[0].left(seeds[0].size() - 2) + le16(0xFFFF), "zip: a comment past the end");
            b.add(zipOf({ { "a.srt", "hello", 0 } }, eocd + le16(9) + le16(9) + le32(9) + le32(0) + le16(0)),
                  "zip: a second end inside the comment");
            {
                // one deflated subtitle of 8 MiB with a wrong CRC, named by 64 entries
                ZipFile f { "a.srt", QByteArray(8 << 20, 'a'), 8, true };
                const QByteArray packed = rawDeflate(f.data);
                QByteArray zip = zipHeaders(0x04034b50, f, packed, false, 0) + packed;
                QByteArray directory;
                for (int i = 0; i < 1000; ++i)
                    directory += zipHeaders(0x02014b50, f, packed, true, 0);
                const quint32 at = quint32(zip.size());
                zip += directory + eocd + le16(1000) + le16(1000) + le32(quint32(directory.size())) + le32(at) + le16(0);
                b.add(zip, "zip: 1000 entries naming one 8 MiB file of a wrong CRC (overlap bomb)", 32 << 20);
            }
            {
                ZipFile f { "big.srt", QByteArray(64 << 20, '\0'), 8 };
                const QByteArray packed = rawDeflate(f.data);
                f.data = QByteArray(1024, '\0');   // the sizes lie: 1 KB said, 64 MB inside
                const QByteArray local = zipHeaders(0x04034b50, f, packed, false, 0);
                const QByteArray central = zipHeaders(0x02014b50, f, packed, true, 0);
                b.add(local + packed + central + eocd + le16(1) + le16(1) + le32(quint32(central.size()))
                          + le32(quint32(local.size() + packed.size())) + le16(0),
                      "zip: 1 KB said, 64 MB inside", 32 << 20);
            }
            {
                std::vector<ZipFile> many;
                for (int i = 0; i < 2000; ++i)
                    many.push_back({ "s" + num(i) + ".srt", "1\n00:00:01,000 --> 00:00:02,000\nx\n", 0 });
                b.add(zipOf(many), "zip: 2000 subtitles");
            }
            b.add(zipOf({ { "../../../etc/x.srt", "x", 0 }, { QByteArray(300, 'n') + ".srt", "y", 0 },
                          { bytes("\xFF\xFE.srt", 6), "z", 0 } }),
                  "zip: odd names");
            b.add(zipOf({ { "bzip2.srt", "data", 12 }, { "stored-lie.srt", "data", 0, true } }), "zip: an unknown method, a bad CRC");
        };
        list.push_back(t);
    }
    {
        Target t = target("subs::cleanRelease/releaseOfEpisode", Text, { "fuzz/releases.txt" }, [](const QByteArray &d) {
            forLines(d, 20000, [](const QString &line) {
                subs::cleanRelease(line);
                subs::releaseOfEpisode(line, 2, 3);
                subs::releaseOfEpisode(line, 99, 999);
            });
        });
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            b.ladder("cleanRelease: x---...---x", kSmallLadder,
                     [](long long n) { return "x" + QByteArray(qsizetype(n), '-') + "x"; });
            b.ladder("cleanRelease: x   ...   x", kSmallLadder,
                     [](long long n) { return "x" + QByteArray(qsizetype(n), ' ') + "x"; });
            b.ladder("releaseOfEpisode: s1---...---y", kSmallLadder,
                     [](long long n) { return " s1" + QByteArray(qsizetype(n), '-') + "y"; });
            b.ladder("releaseOfEpisode: s01e01 s01e01 ...", kSmallLadder, [](long long n) { return fill("s01e01 ", n); });
            b.ladder("releaseOfEpisode: 1x1x1x...", kSmallLadder, [](long long n) { return fill("1x", n); });
        };
        list.push_back(t);
    }

    // ----- Subtitle files (their lines on the seek bar)
    {
        Target t = target("subtitlecues::parse", Text,
                          { "subtitles/corpus/*", "timing.srt", "fuzz/vobsub.idx", "fuzz/microdvd.sub",
                            "fuzz/subviewer.sub", "fuzz/cues.ass" },
                          [](const QByteArray &d) {
                              const std::string s(d.constData(), size_t(d.size()));
                              static const std::pair<double, int> modes[] = { { 25.0, -1 }, { 23.976, 0 }, { 0.0, 1 } };
                              for (const auto &[fps, index] : modes)
                              {
                                  const std::vector<subtitlecues::Cue> cues = subtitlecues::parse(s, fps, index);
                                  for (size_t i = 0; i < cues.size(); ++i)
                                  {
                                      if (i > 0 && cues[i - 1].startMs > cues[i].startMs)
                                          note("cues not sorted by start");
                                      if (cues[i].endMs < cues[i].startMs)
                                          note("a cue ends before it starts");
                                      if (cues[i].text.size() > subtitlecues::maxTextBytes + 3)
                                          note("a cue's text is longer than maxTextBytes");
                                  }
                              }
                          });
        t.mutations = 4000;
        t.extra = [](Builder &b, const std::vector<QByteArray> &) {
            const QByteArray vobsub = "# VobSub index file, v7\nid: en, index: 0\n";
            b.lazy([vobsub] { return vobsub + fill("delay: 999999999:59:59:999\n", 2600 * 27)
                                     + "timestamp: 00:00:01:000, filepos: 0\n"; },
                   "VobSub: 2600 delays of 999999999 hours");
            b.lazy([vobsub] { return vobsub + fill("delay: -999999999:59:59:999\n", 2600 * 28)
                                     + "timestamp: 00:00:01:000, filepos: 0\n"; },
                   "VobSub: 2600 delays of -999999999 hours");
            b.add("1\n999999999:59:59,999 --> 999999999:59:59,999\nx\n", "SRT: times of 999999999 hours");
            b.add("{999999999}{999999999}x\n{1}{1}1.0000001\n", "MicroDVD: frames of 999999999");
            b.add("{1}{1}199.99\n{999999999}{}x\n", "MicroDVD: a last open end");
            b.add(bytes("\xFF\xFE" "1\0\n\0\0\xD8", 8), "UTF-16: odd length, a surrogate cut");
            b.add("[Events]\nFormat: Text, End, Start\nDialogue: x,0:00:02.00,0:00:01.00\n", "ASS: fields in another order");
            b.add("[Events]\nFormat: Start\nDialogue: 0:00:01.00\n", "ASS: a Format without End nor Text");
            b.lazy([] { return QByteArray(33 << 20, 'a'); }, "a 33 MB file (past the limit)");
            b.ladder("SAMI: many SYNC, no </BODY>", kSmallLadder,
                     [](long long n) { return "<SAMI><BODY>\n" + fill("<SYNC Start=1000><P>x\n", n); });
            b.ladder("SAMI: many SYNC, then </BODY>", kSmallLadder,
                     [](long long n) { return "<SAMI><BODY>\n" + fill("<SYNC Start=1000><P>x\n", n) + "</BODY>"; });
            b.ladder("SRT: a cue of < never closed", kLadder, [](long long n) {
                return "1\n00:00:01,000 --> 00:00:02,000\n" + QByteArray(qsizetype(n), '<') + "\n";
            });
            b.ladder("SRT: a cue of { never closed", kLadder, [](long long n) {
                return "1\n00:00:01,000 --> 00:00:02,000\n" + QByteArray(qsizetype(n), '{') + "\n";
            });
            b.ladder("SRT: a cue of &", kLadder, [](long long n) {
                return "1\n00:00:01,000 --> 00:00:02,000\n" + QByteArray(qsizetype(n), '&') + "\n";
            });
            b.ladder("SRT: a cue of many lines", kSmallLadder, [](long long n) {
                return "1\n00:00:01,000 --> 00:00:02,000\n" + fill("text\n", n);
            });
            b.ladder("SRT: many cues", kSmallLadder, [](long long n) {
                return numbered(n, [](long long i) {
                    return num(i) + "\n00:00:01,000 --> 00:00:02,000\nx\n\n";
                });
            });
            b.ladder("ASS: a Format of many fields", kLadder, [](long long n) {
                const QByteArray fields = fill("F, ", n / 2);
                return "[Events]\nFormat: " + fields + "Start, End, Text\nDialogue: " + fill("x,", n / 2 * 2 / 3)
                       + "0:00:01.00,0:00:02.00,hi\n";
            });
            b.ladder("MicroDVD: many lines", kSmallLadder, [](long long n) { return fill("{10}{20}x|y\n", n); });
            b.ladder("VobSub: many timestamps", kSmallLadder, [](long long n) {
                return "# VobSub index file\nid: en, index: 0\n" + fill("timestamp: 00:00:01:000, filepos: 0\n", n);
            });
        };
        list.push_back(t);
    }

    // the longest runs start first
    static const std::map<std::string, int> weights = {
        { "subtitlecues::parse", 10 }, { "parseXmltv(QByteArray)", 9 }, { "parseM3u", 8 },
        { "autoguide::matchName", 7 }, { "xtream::parseAccountInfo", 6 }, { "xtream::parseLiveStreams", 6 },
        { "xtream::parseVodStreams", 6 }, { "xtream::parseSeriesInfo", 6 }, { "xtream::parseSeries", 6 },
        { "catchupUrl", 6 }, { "subs::cleanRelease/releaseOfEpisode", 5 }, { "autoguide::pickGuides", 5 },
        { "autoguide::Matcher", 5 }, { "parseXmltv(QIODevice)", 4 }, { "parseXmltvFile", 4 },
        { "parseXmltv(cancelled)", 4 }, { "autoguide::parseIptvOrgChannels", 4 }, { "xtream::parseCategories", 4 },
    };
    for (Target &t : list)
    {
        const auto it = weights.find(t.name);
        if (it != weights.end())
            t.weight = it->second;
    }
    return list;
}

// ---------------------------------------------------------------------------
// A child: one parser, its inputs from start

[[noreturn]] void runChild(const Target &t, std::vector<Case> &cases, Slot *slot, long start)
{
    g_slot = slot;
    std::set<int> skip(slot->skipGroups, slot->skipGroups + slot->nSkip);
    std::map<int, std::pair<long long, double>> last;
    std::map<int, double> lastExponent;
    std::map<int, std::string> trail;
    for (long i = start; i < long(cases.size()); ++i)
    {
        Case &c = cases[size_t(i)];
        if (c.group >= 0 && skip.count(c.group))
        {
            slot->skipped++;
            continue;
        }
        if (c.make)
        {
            c.input = c.make();
            c.make = nullptr;
        }
        g_currentWhat = c.what;
        copyTo(slot->current, sizeof slot->current, c.what);
        slot->group = c.group;
        slot->caseIndex = i;
        const long long base = g_live;
        g_peak = g_live;
        const long long t0 = nowNs();
        slot->callStart = t0;
        t.run(c.input);
        const long long t1 = nowNs();
        slot->callStart = 0;
        const double secs = double(t1 - t0) / 1e9;
        const long long peak = g_peak - base;
        slot->tried++;
        if (secs > slot->maxSecs)
        {
            slot->maxSecs = secs;
            copyTo(slot->maxSecsWhat, sizeof slot->maxSecsWhat, c.what);
        }
        if (peak > slot->maxPeak)
        {
            slot->maxPeak = peak;
            copyTo(slot->maxPeakWhat, sizeof slot->maxPeakWhat, c.what);
        }
        const long long allowance = (64LL << 20) + 48LL * c.input.size() + c.allow;
        if (g_measure && peak > allowance)
            addFinding(Alloc, double(peak), c.what,
                       "peak " + mb(peak) + " allocated for an input of " + mb(c.input.size()));
        if (c.group < 0)
        {
            if (secs > S.slowSecs)
            {
                char buf[160];
                std::snprintf(buf, sizeof buf, "%.2f s for an input of %s", secs, mb(c.input.size()).c_str());
                addFinding(Slow, secs, c.what, buf);
            }
        }
        else
        {
            // a ladder: how the time grows with the size
            std::string &tr = trail[c.group];
            char buf[64];
            std::snprintf(buf, sizeof buf, "%s%s %.3fs", tr.empty() ? "" : ", ", sizeLabel(c.scale).c_str(), secs);
            tr += buf;
            const auto it = last.find(c.group);
            const double prevSecs = it != last.end() ? it->second.second : 0;
            double exponent = -1;   // unknown (times too short to tell: noise)
            if (it != last.end() && prevSecs >= 0.01 && c.scale > it->second.first)
                exponent = std::log(std::max(secs, 1e-6) / prevSecs) / std::log(double(c.scale) / double(it->second.first));
            last[c.group] = { c.scale, secs };
            const auto pe = lastExponent.find(c.group);
            const double prevExponent = pe != lastExponent.end() ? pe->second : -1;
            lastExponent[c.group] = exponent;
            // the next size, and its time if it grows on so
            const bool hasNext = size_t(i + 1) < cases.size() && cases[size_t(i + 1)].group == c.group;
            double predicted = 0;
            if (hasNext)
            {
                const double grow = exponent < 0 || prevSecs < 0.02 ? 1.3 : std::max(1.2, exponent);
                predicted = secs * std::pow(double(cases[size_t(i + 1)].scale) / double(c.scale), grow);
            }
            const bool pastBudget = hasNext && predicted > S.hangSecs * 0.6;
            char more[200];
            if (!c.judged)
            {
                if (pastBudget)
                    skip.insert(c.group);
            }
            else if (secs > S.slowSecs)
            {
                // more than 2 s: a problem, but for a big input read in a time that grows as its size
                const bool linear = exponent >= 0 && exponent <= 1.35;
                std::snprintf(more, sizeof more, "; %.2f s for %s%s", secs, mb(c.input.size()).c_str(),
                              linear ? ", the time growing as the size (no more than slow)" : "");
                addFinding(linear ? Note : Slow, secs, c.what, tr + more);
                skip.insert(c.group);
            }
            // faster than the size: by a lot, or confirmed by the step before
            // (one step can be the noise of the other jobs), or so that the
            // next size would be past the time it may take
            else if (secs >= 0.25
                     && ((exponent > 1.6 && (prevExponent > 1.2 || exponent > 1.8 || !hasNext))
                         || (exponent >= 1.35 && pastBudget)))
            {
                std::snprintf(more, sizeof more, "; time grows as size^%.1f: about %.0f s at 10 MB", exponent,
                              secs * std::pow(double(10 << 20) / double(c.scale), exponent));
                addFinding(Superlinear, exponent, c.ladder, tr + more);
                skip.insert(c.group);
            }
            else if (pastBudget)
            {
                std::snprintf(more, sizeof more, "; the next size (%s) would take about %.0f s: not run",
                              sizeLabel(cases[size_t(i + 1)].scale).c_str(), predicted);
                addFinding(Note, predicted, c.ladder, tr + more);
                skip.insert(c.group);
            }
        }
        c.input = QByteArray();
    }
    std::fflush(stdout);
    if (__lsan_do_recoverable_leak_check && __lsan_do_recoverable_leak_check())
    {
        std::fflush(stderr);
        _exit(3);
    }
    _exit(0);
}

// ---------------------------------------------------------------------------
// The parent: the children, what they found, the summary

std::string readText(const std::string &path)
{
    std::string out;
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f)
        return out;
    char buf[65536];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0 && out.size() < (8u << 20))
        out.append(buf, n);
    std::fclose(f);
    return out;
}

std::string trim(const std::string &s)
{
    const size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return {};
    const size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string stripSrc(std::string s)
{
    for (size_t at; (at = s.find("/src/")) != std::string::npos;)
        s.erase(at, 5);
    return s;
}

// The report of a sanitizer, shortened: its error, the first frames and
// those in our sources, its summary
std::string digestOf(const std::string &report, std::string *signature)
{
    std::vector<std::string> lines;
    size_t at = 0;
    while (at < report.size())
    {
        size_t nl = report.find('\n', at);
        if (nl == std::string::npos)
            nl = report.size();
        lines.push_back(report.substr(at, nl - at));
        at = nl + 1;
    }
    std::string error, summary;
    std::vector<std::string> frames;
    bool inStack = false, stackDone = false;
    for (const std::string &l : lines)
    {
        if (error.empty()
            && (l.find("ERROR: AddressSanitizer") != std::string::npos
                || l.find("ERROR: LeakSanitizer") != std::string::npos
                || l.find("runtime error:") != std::string::npos))
            error = stripSrc(trim(l));
        if (summary.empty() && l.rfind("SUMMARY:", 0) == 0)
            summary = stripSrc(l);
        const std::string t = trim(l);
        if (!stackDone && !error.empty() && t.size() > 2 && t[0] == '#' && isDigitChar(t[1]))
        {
            // "#3 0x55d0c in func file:line"
            std::string frame = t;
            const size_t in = frame.find(" in ");
            const size_t hash = frame.find(' ');
            if (in != std::string::npos && hash != std::string::npos)
                frame = frame.substr(0, hash) + frame.substr(in + 3);
            frame = stripSrc(frame);
            if (frame.size() > 200)
                frame = frame.substr(0, 140) + " ... " + frame.substr(frame.size() - 55);
            frames.push_back(frame);
            inStack = true;
        }
        else if (inStack)
            stackDone = true;
    }
    std::string out = error.empty() ? std::string("(no sanitizer report: a signal?)") : error;
    int shown = 0;
    for (size_t i = 0; i < frames.size() && shown < 10; ++i)
    {
        const bool ours = frames[i].find("modules/") != std::string::npos || frames[i].find("extras/") != std::string::npos;
        if (i < 3 || ours)
        {
            out += "\n      " + frames[i];
            ++shown;
        }
    }
    if (!summary.empty())
        out += "\n      " + summary;
    // (one bug a place: UBSan's message carries the value, "nan is outside...")
    const size_t ub = error.find(": runtime error:");
    *signature = ub != std::string::npos ? error.substr(0, ub) : summary.empty() ? error : summary;
    return out;
}

std::string hexOf(const QByteArray &d)
{
    std::string out;
    char buf[4];
    for (int i = 0; i < d.size(); ++i)
    {
        std::snprintf(buf, sizeof buf, "%02x", uchar(d[i]));
        out += buf;
    }
    return out;
}

struct Report
{
    int kind;
    std::string target;
    double value;
    std::string what;
    std::string detail;
    std::string signature;
    int count = 1;
    std::string input = {};   // a crash's input, saved
};

struct TargetState
{
    long tried = 0;
    long skipped = 0;
    double maxSecs = 0;
    std::string maxSecsWhat;
    long long maxPeak = 0;
    std::string maxPeakWhat;
    int crashes = 0;
    int restarts = 0;
    std::vector<int> skipGroups;
    std::set<std::string> notes;
    double wall = 0;
};

std::vector<Report> g_reports;
// a parser that crashes more is left (each crash restarts its child)
constexpr int kMaxCrashes = 150;

void addReport(Report r)
{
    // the same crash (or note) of the same parser: counted once
    for (Report &o : g_reports)
        if (o.kind == r.kind && o.target == r.target && !r.signature.empty() && o.signature == r.signature)
        {
            ++o.count;
            return;
        }
    g_reports.push_back(std::move(r));
}

void printReport(const Report &r)
{
    std::printf("[%s] %s: %s", kindNames[r.kind], r.target.c_str(), r.what.c_str());
    if (r.count > 1)
        std::printf(" (and %d more inputs)", r.count - 1);
    std::printf("\n      %s\n", r.detail.c_str());
    std::fflush(stdout);
}

struct Job
{
    int target = -1;
    pid_t pid = -1;
    long start = 0;
    long long began = 0;
    bool killed = false;
    std::string err;
};

void resetSlot(Slot *s, const std::vector<int> &skip)
{
    s->caseIndex = -1;
    s->callStart = 0;
    s->tried = 0;
    s->skipped = 0;
    s->group = -1;
    s->current[0] = 0;
    s->maxSecs = 0;
    s->maxSecsWhat[0] = 0;
    s->maxPeak = 0;
    s->maxPeakWhat[0] = 0;
    s->nSkip = int(std::min<size_t>(maxSkip, skip.size()));
    for (int i = 0; i < s->nSkip; ++i)
        s->skipGroups[i] = skip[size_t(i)];
    s->nFindings = 0;
}

// (the inputs are built once by the parent: a child restarted after a crash
// has them at once)
pid_t launch(const Target &t, std::vector<Case> &cases, Slot *slot, long start, const std::string &err)
{
    std::fflush(stdout);
    std::fflush(stderr);
    const pid_t pid = fork();
    if (pid == 0)
    {
        const int fd = open(err.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0)
        {
            dup2(fd, 2);
            close(fd);
        }
        runChild(t, cases, slot, start);
    }
    return pid;
}

QByteArray inputOf(const Target &t, long index)
{
    const Seeds seeds = seedsOf(t);
    std::vector<Case> cases = buildCases(t, seeds);
    if (index < 0 || index >= long(cases.size()))
        return {};
    Case &c = cases[size_t(index)];
    return c.make ? c.make() : c.input;
}

// A crash run again alone, in a new process, with the stacks of UBSan: its
// report
std::string stackOf(const std::string &target, const std::string &input, const std::string &err)
{
    const pid_t pid = fork();
    if (pid == 0)
    {
        const int fd = open(err.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0)
        {
            dup2(fd, 1);
            dup2(fd, 2);
            close(fd);
        }
        const char *old = getenv("UBSAN_OPTIONS");
        const std::string options = std::string(old ? old : "") + (old && *old ? ":" : "") + "print_stacktrace=1";
        setenv("UBSAN_OPTIONS", options.c_str(), 1);
        const std::string fixtures = S.fixtures.toStdString();
        execl("/proc/self/exe", "parsers-fuzz", fixtures.c_str(), "--repro", target.c_str(), input.c_str(),
              static_cast<char *>(nullptr));
        _exit(127);
    }
    int status = 0;
    const long long until = nowNs() + 60 * 1000000000LL;
    while (waitpid(pid, &status, WNOHANG) == 0)
    {
        if (nowNs() > until)
        {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            break;
        }
        usleep(5000);
    }
    return readText(err);
}

int runAll(const std::vector<Target> &targets)
{
    const long long began = nowNs();
    mkdir(S.work.c_str(), 0755);
    const int jobs = std::max(1, S.jobs);
    void *mem = mmap(nullptr, sizeof(Slot) * size_t(jobs), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED)
    {
        std::perror("mmap");
        return 2;
    }
    Slot *shared = static_cast<Slot *>(mem);
    for (int i = 0; i < jobs; ++i)
        new (&shared[i]) Slot();
    std::vector<Job> running(static_cast<size_t>(jobs));
    std::vector<TargetState> state(targets.size());
    std::vector<std::vector<Case>> built(targets.size());
    // the heavy ones first: the run ends with the light ones
    std::vector<int> order(targets.size());
    for (size_t i = 0; i < order.size(); ++i)
        order[i] = int(i);
    std::stable_sort(order.begin(), order.end(),
                     [&](int a, int b) { return targets[size_t(a)].weight > targets[size_t(b)].weight; });
    size_t next = 0;
    int finished = 0;
    int saved = 0;

    auto start = [&](int j, int ti, long from) {
        Job &job = running[size_t(j)];
        job.target = ti;
        job.start = from;
        job.killed = false;
        job.began = nowNs();
        job.err = S.work + "/child-" + std::to_string(j) + ".err";
        if (built[size_t(ti)].empty())
            built[size_t(ti)] = buildCases(targets[size_t(ti)], seedsOf(targets[size_t(ti)]));
        resetSlot(&shared[j], state[size_t(ti)].skipGroups);
        job.pid = launch(targets[size_t(ti)], built[size_t(ti)], &shared[j], from, job.err);
    };

    for (;;)
    {
        bool any = false;
        for (int j = 0; j < jobs; ++j)
        {
            Job &job = running[size_t(j)];
            if (job.pid < 0 && next < targets.size())
                start(j, order[next++], 0);
            if (job.pid < 0)
                continue;
            any = true;
            Slot &slot = shared[j];
            int status = 0;
            const pid_t r = waitpid(job.pid, &status, WNOHANG);
            if (r == 0)
            {
                const long long cs = slot.callStart.load();
                if (cs && nowNs() - cs > (long long)(S.hangSecs * 1e9))
                {
                    kill(job.pid, SIGKILL);
                    waitpid(job.pid, &status, 0);
                    job.killed = true;
                }
                else
                    continue;
            }
            // the child is done: what it found
            const Target &t = targets[size_t(job.target)];
            TargetState &st = state[size_t(job.target)];
            st.tried += slot.tried.load();
            st.skipped += slot.skipped.load();
            if (slot.maxSecs > st.maxSecs)
            {
                st.maxSecs = slot.maxSecs;
                st.maxSecsWhat = slot.maxSecsWhat;
            }
            if (slot.maxPeak > st.maxPeak)
            {
                st.maxPeak = slot.maxPeak;
                st.maxPeakWhat = slot.maxPeakWhat;
            }
            for (int k = 0; k < slot.nFindings.load(); ++k)
            {
                const Finding &f = slot.findings[k];
                Report rep { f.kind, t.name, f.value, f.what, f.detail, f.kind == Note ? std::string(f.detail) : std::string() };
                if (f.kind == Note && !st.notes.insert(f.detail).second)
                    continue;
                printReport(rep);
                addReport(rep);
            }
            const long ci = slot.caseIndex.load();
            const bool inCall = slot.callStart.load() != 0;
            long restartAt = -1;
            if (job.killed)
            {
                char buf[200];
                std::snprintf(buf, sizeof buf, "killed after %.0f s in one call", S.hangSecs);
                Report rep { Hang, t.name, S.hangSecs, slot.current, buf, "" };
                printReport(rep);
                addReport(rep);
                if (slot.group.load() >= 0)
                    st.skipGroups.push_back(slot.group.load());
                restartAt = ci + 1;
            }
            else if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                ;
            else if (WIFEXITED(status) && WEXITSTATUS(status) == 3)
            {
                std::string sig;
                Report rep { Leak, t.name, 0, "the inputs of the run", digestOf(readText(job.err), &sig), sig };
                printReport(rep);
                addReport(rep);
            }
            else
            {
                std::string sig;
                std::string detail = digestOf(readText(job.err), &sig);
                if (WIFSIGNALED(status))
                    detail += "\n      (signal " + std::to_string(WTERMSIG(status)) + ")";
                else
                    detail += "\n      (exit code " + std::to_string(WEXITSTATUS(status)) + ")";
                std::string what = inCall ? std::string(slot.current) : std::string("outside a call, after input #")
                                                                            + std::to_string(ci);
                const bool known = std::any_of(g_reports.begin(), g_reports.end(), [&](const Report &o) {
                    return o.kind == Crash && o.target == t.name && o.signature == sig;
                });
                std::string path;
                if (inCall && !known && saved < 64 && ci >= 0 && ci < long(built[size_t(job.target)].size()))
                {
                    const Case &c = built[size_t(job.target)][size_t(ci)];
                    const QByteArray input = c.make ? c.make() : c.input;
                    path = S.work + "/crash-" + std::to_string(++saved) + ".bin";
                    QFile f(QString::fromStdString(path));
                    if (f.open(QIODevice::WriteOnly))
                        f.write(input);
                    detail += "\n      input: " + std::to_string(input.size()) + " bytes, saved as " + path;
                    if (input.size() <= 160)
                        detail += "\n      hex: " + hexOf(input);
                }
                Report rep { Crash, t.name, 0, what, detail, sig, 1, path };
                if (!known)
                    printReport(rep);
                addReport(rep);
                if (inCall && ++st.crashes < kMaxCrashes)
                    restartAt = ci + 1;
                else if (inCall)
                {
                    Report stop { Note, t.name, 0, "its inputs",
                                  "stopped after " + std::to_string(kMaxCrashes) + " crashes", "stopped" };
                    printReport(stop);
                    addReport(stop);
                }
            }
            st.wall += double(nowNs() - job.began) / 1e9;
            if (restartAt >= 0)
            {
                ++st.restarts;
                start(j, job.target, restartAt);
                continue;
            }
            ++finished;
            std::printf("[%2d/%zu] %-40s %8ld inputs  %6.1f s\n", finished, targets.size(), t.name.c_str(), st.tried,
                        st.wall);
            std::fflush(stdout);
            built[size_t(job.target)].clear();
            built[size_t(job.target)].shrink_to_fit();
            job.pid = -1;
            job.target = -1;
            if (next < targets.size())
                start(j, order[next++], 0);
        }
        if (!any && next >= targets.size())
            break;
        usleep(2000);
    }

    // each crash found, run again alone with its stack
    int stacks = 0;
    for (Report &r : g_reports)
    {
        if (r.kind != Crash || r.input.empty())
            continue;
        std::string sig;
        const std::string again = digestOf(stackOf(r.target, r.input, S.work + "/stack-" + std::to_string(++stacks) + ".err"), &sig);
        const size_t tail = r.detail.find("\n      (");
        if (again.find("\n      #") != std::string::npos)
            r.detail = again + (tail == std::string::npos ? std::string() : r.detail.substr(tail));
    }

    // the summary
    long total = 0;
    std::printf("\n== summary: %zu parsers\n", targets.size());
    std::printf("%-40s %9s %9s  %-9s %-11s\n", "parser", "inputs", "skipped", "max time", "max alloc");
    for (size_t i = 0; i < targets.size(); ++i)
    {
        const TargetState &st = state[i];
        total += st.tried;
        char secs[32];
        std::snprintf(secs, sizeof secs, "%.3f s", st.maxSecs);
        std::printf("%-40s %9ld %9ld  %-9s %-11s  (slowest: %s)\n", targets[i].name.c_str(), st.tried, st.skipped, secs,
                    g_measure ? mb(st.maxPeak).c_str() : "-", st.maxSecsWhat.c_str());
    }
    int problems = 0, notes = 0;
    for (const Report &r : g_reports)
        (r.kind == Note ? notes : problems)++;
    std::printf("\n== %d problems, %d notes\n", problems, notes);
    for (int kind = Crash; kind <= Note; ++kind)
        for (const Report &r : g_reports)
            if (r.kind == kind)
                printReport(r);
    std::printf("\n%ld inputs in %.1f s (%d at a time%s)\n", total, double(nowNs() - began) / 1e9, jobs,
                g_measure ? ", allocations measured" : ", allocations not measured: no AddressSanitizer");
    std::puts(problems ? "FAIL" : "PASS");
    for (int i = 0; i < jobs; ++i)
        shared[i].~Slot();
    munmap(mem, sizeof(Slot) * size_t(jobs));
    return problems ? 1 : 0;
}

const Target *findTarget(const std::vector<Target> &targets, const std::string &name)
{
    for (const Target &t : targets)
        if (t.name == name)
            return &t;
    std::fprintf(stderr, "no parser %s (--list)\n", name.c_str());
    return nullptr;
}

} // namespace

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string repro, reproFile, dumpFile;
    long dumpIndex = -1;
    bool listOnly = false;
    for (size_t i = 0; i < args.size(); ++i)
    {
        const std::string &a = args[i];
        auto value = [&]() { return i + 1 < args.size() ? args[++i] : std::string(); };
        if (a == "--list")
            listOnly = true;
        else if (a == "--only")
        {
            const std::string v = value();
            size_t at = 0;
            while (at <= v.size())
            {
                size_t comma = v.find(',', at);
                if (comma == std::string::npos)
                    comma = v.size();
                S.only.insert(v.substr(at, comma - at));
                at = comma + 1;
            }
        }
        else if (a == "--jobs")
            S.jobs = std::atoi(value().c_str());
        else if (a == "--mutations")
            S.mutations = std::atoi(value().c_str());
        else if (a == "--work")
            S.work = value();
        else if (a == "--repro")
        {
            repro = value();
            reproFile = value();
        }
        else if (a == "--dump")
        {
            repro = value();
            dumpIndex = std::atol(value().c_str());
            dumpFile = value();
        }
        else if (a == "--cases")
        {
            repro = value();
            dumpIndex = -2;
        }
        else
            S.fixtures = QString::fromLocal8Bit(a.c_str());
    }
    if (__sanitizer_install_malloc_and_free_hooks && __sanitizer_get_allocated_size)
        g_measure = __sanitizer_install_malloc_and_free_hooks(onMalloc, onFree) != 0;

    const std::vector<Target> all = makeTargets();
    if (listOnly)
    {
        for (const Target &t : all)
            std::printf("%s\n", t.name.c_str());
        return 0;
    }
    if (!repro.empty())
    {
        const Target *t = findTarget(all, repro);
        if (!t)
            return 2;
        if (dumpIndex == -2)
        {
            const std::vector<Case> cases = buildCases(*t, seedsOf(*t));
            for (size_t i = 0; i < cases.size(); ++i)
                std::printf("%zu\t%s\n", i, cases[i].what.c_str());
            return 0;
        }
        if (dumpIndex >= 0)
        {
            const QByteArray input = inputOf(*t, dumpIndex);
            QFile f(QString::fromStdString(dumpFile));
            if (!f.open(QIODevice::WriteOnly))
                return 2;
            f.write(input);
            std::printf("input #%ld of %s: %lld bytes in %s\n", dumpIndex, t->name.c_str(), (long long)input.size(),
                        dumpFile.c_str());
            return 0;
        }
        const QByteArray input = readFile(QString::fromStdString(reproFile));
        g_currentWhat = reproFile;
        const long long base = g_live;
        g_peak = g_live;
        const long long t0 = nowNs();
        t->run(input);
        const double secs = double(nowNs() - t0) / 1e9;
        std::printf("%s on %s (%lld bytes): %.3f s, peak %s\n", t->name.c_str(), reproFile.c_str(),
                    (long long)input.size(), secs, g_measure ? mb(g_peak - base).c_str() : "-");
        return 0;
    }
    std::vector<Target> chosen;
    for (const Target &t : all)
        if (S.only.empty() || S.only.count(t.name))
            chosen.push_back(t);
    if (chosen.empty())
    {
        std::fprintf(stderr, "no parser chosen (--list)\n");
        return 2;
    }
    std::printf("parsers-fuzz: %zu parsers, fixtures in %s, %d mutations a parser\n", chosen.size(),
                qPrintable(S.fixtures), S.mutations);
    return runAll(chosen);
}
