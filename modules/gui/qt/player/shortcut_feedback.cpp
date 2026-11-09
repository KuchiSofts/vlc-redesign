/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#include "shortcut_feedback.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

namespace shortcutfeedback {

namespace {

bool startsWith(const std::string &text, const char *prefix)
{
    return text.compare(0, std::strlen(prefix), prefix) == 0;
}

bool oneOf(const std::string &text, std::initializer_list<const char *> names)
{
    for (const char *name : names)
        if (text == name)
            return true;
    return false;
}

// U+2212 MINUS SIGN, as a sign is set in text (a hyphen reads as a dash)
const char minusSign[] = "\xE2\x88\x92";
// U+00D7 MULTIPLICATION SIGN
const char timesSign[] = "\xC3\x97";

// A number with its sign kept together from left to right in any language
// (a right-to-left one put "+" after "10"): U+2066 LEFT-TO-RIGHT ISOLATE ...
// U+2069 POP DIRECTIONAL ISOLATE, which leave the direction of the rest of
// the text to its words
std::string isolate(const std::string &number)
{
    return "\xE2\x81\xA6" + number + "\xE2\x81\xA9";
}

} // namespace

Kind kindOf(const std::string &action)
{
    if (action.empty())
        return None;
    // what shows the controls, opens their menu or needs them: as before
    if (oneOf(action, { "intf-show", "intf-boss", "intf-popup-menu", "quit", "toggle-fullscreen",
                        "leave-fullscreen", "nav-activate", "disc-menu" }))
        return Controls;
    if (startsWith(action, "jump+") || startsWith(action, "jump-") || action == "nav-left" || action == "nav-right")
        return Seek;
    if (action == "position")
        return Position;
    if (oneOf(action, { "play-pause", "play", "pause" }))
        return PlayPause;
    if (oneOf(action, { "frame-next", "frame-previous" }))
        return Frame;
    if (oneOf(action, { "faster", "slower", "rate-normal", "rate-faster-fine", "rate-slower-fine" }))
        return Rate;
    // the arrows up and down change the volume without a menu (VLC's player)
    if (oneOf(action, { "vol-up", "vol-down", "vol-mute", "nav-up", "nav-down" }))
        return Volume;
    return Message;
}

int64_t jumpSeconds(const std::string &action, const int64_t sizes[4])
{
    int64_t sign;
    std::string size;
    if (action == "nav-right" || action == "nav-left")
    {
        sign = action == "nav-right" ? 1 : -1;
        size = "short";
    }
    else if (startsWith(action, "jump+") || startsWith(action, "jump-"))
    {
        sign = action[4] == '+' ? 1 : -1;
        size = action.substr(5);
    }
    else
        return 0;
    static const char *const names[] = { "extrashort", "short", "medium", "long" };
    for (int i = 0; i < 4; ++i)
        if (size == names[i])
            return sizes[i] > 0 ? sign * sizes[i] : 0;
    return 0;
}

int64_t SeekSum::add(int64_t seconds, int64_t nowMs)
{
    const bool inRow = m_started && nowMs - m_lastMs <= feedbackMs && nowMs >= m_lastMs;
    // the other way: from this jump on
    const bool turned = (m_sum > 0 && seconds < 0) || (m_sum < 0 && seconds > 0);
    if (!inRow || turned)
        m_sum = 0;
    m_sum += seconds;
    m_lastMs = nowMs;
    m_started = true;
    return m_sum;
}

int64_t SeekSum::add(int64_t seconds, int64_t nowMs, int64_t fromMs, int64_t lengthMs)
{
    const int64_t before = m_sum;
    const bool wasStarted = m_started;
    add(seconds, nowMs);
    // a new run (the first jump, a pause between, or the other way): it
    // starts where the player is now
    if (!wasStarted || m_sum != before + seconds)
        m_fromMs = fromMs < 0 ? 0 : fromMs;
    int64_t target = m_fromMs + m_sum * 1000;
    if (target < 0)
        target = 0;
    if (lengthMs > 0 && target > lengthMs)
        target = lengthMs;
    const int64_t moved = target - m_fromMs;
    // (to the nearest second, half away from zero)
    return moved >= 0 ? (moved + 500) / 1000 : -((-moved + 500) / 1000);
}

void SeekSum::reset()
{
    m_sum = 0;
    m_started = false;
}

std::string stepText(int64_t seconds, const std::string &secondsFormat)
{
    if (seconds == 0)
        return {};
    const std::string sign = seconds > 0 ? "+" : minusSign;
    const uint64_t total = seconds > 0 ? static_cast<uint64_t>(seconds)
                                       : static_cast<uint64_t>(-(seconds + 1)) + 1;
    char buf[48];
    if (total < 60)
    {
        std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(total));
        std::string text = secondsFormat;
        const size_t at = text.find("%1");
        // (a translation that lost its %1: the number all the same)
        if (at == std::string::npos)
            return isolate(sign + buf) + " s";
        text.replace(at, 2, isolate(sign + buf));
        return text;
    }
    const unsigned long long hours = total / 3600, minutes = total / 60 % 60, secs = total % 60;
    if (hours > 0)
        std::snprintf(buf, sizeof(buf), "%llu:%02llu:%02llu", hours, minutes, secs);
    else
        std::snprintf(buf, sizeof(buf), "%llu:%02llu", minutes, secs);
    return isolate(sign + buf);
}

std::string rateText(double rate)
{
    if (!std::isfinite(rate) || rate <= 0)
        return {};
    // in hundredths, written by hand: a decimal point whatever the C locale
    // (Qt sets the user's on Linux, "%f" would write 1,5 there)
    const long long hundredths = std::llround(rate * 100);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%lld", hundredths / 100);
    std::string text = buf;
    const int fraction = static_cast<int>(hundredths % 100);
    if (fraction != 0)
    {
        text += '.';
        text += static_cast<char>('0' + fraction / 10);
        if (fraction % 10 != 0)
            text += static_cast<char>('0' + fraction % 10);
    }
    return isolate(text + timesSign);
}

} // namespace shortcutfeedback
