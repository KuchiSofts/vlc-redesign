/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later. */
#include "modules/gui/qt/iptv/iptv_tune_requests.hpp"

#include <cstdio>
#include <functional>

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

enum class Selection { None, ServerChannel, M3uChannel, Catchup, Vod, Failed };

int main()
{
    IptvTuneRequests tunes;
    Selection applied = Selection::None;
    const auto apply = [&](Selection selection) { applied = selection; };

    CHECK(!tunes.hasSelection());
    const std::function<void(const Selection&)> slowLive = tunes.forNextTune(apply);
    CHECK(tunes.hasSelection());
    const auto fastLive = tunes.forNextTune(apply);
    fastLive(Selection::M3uChannel);
    slowLive(Selection::ServerChannel);
    CHECK(applied == Selection::M3uChannel);

    // A newer selection wins even while its reply is still pending.
    const auto liveBeforeCatchup = tunes.forNextTune(apply);
    const auto catchup = tunes.forNextTune(apply);
    liveBeforeCatchup(Selection::ServerChannel);
    CHECK(applied == Selection::M3uChannel);
    catchup(Selection::Catchup);
    CHECK(applied == Selection::Catchup);

    const auto catchupBeforeLive = tunes.forNextTune(apply);
    const auto failedLive = tunes.forNextTune(apply);
    failedLive(Selection::Failed);
    catchupBeforeLive(Selection::Catchup);
    CHECK(applied == Selection::Failed);

    // VOD starts without a reply and invalidates any pending tune.
    const auto liveBeforeVod = tunes.forNextTune(apply);
    const auto catchupBeforeVod = tunes.forNextTune(apply);
    tunes.advance();
    apply(Selection::Vod);
    liveBeforeVod(Selection::Failed);
    catchupBeforeVod(Selection::Catchup);
    CHECK(applied == Selection::Vod);

    const auto liveAfterVod = tunes.forNextTune(apply);
    liveAfterVod(Selection::ServerChannel);
    CHECK(applied == Selection::ServerChannel);

    return failures ? 1 : 0;
}
