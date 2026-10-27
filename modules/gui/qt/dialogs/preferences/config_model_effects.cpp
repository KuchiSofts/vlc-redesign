/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "config_model.hpp"
#include "config_values.hpp"
#include "dialogs/extended/audio_effects.hpp"
#include "dialogs/extended/video_effects.hpp"
#include "maininterface/mainctx.hpp"
#include "playlist/playlist_controller.hpp"
#include "qt.hpp"

#include <vlc_configuration.h>
#include <vlc_player.h>
#include <vlc_vout.h>

#include <algorithm>
#include <cstdlib>
#include <iterator>

namespace {

// The subtitles shown take the look just kept: the size at once, the font,
// colours, outline... from the next text drawn (the renderer of each video
// loaded again); before, from the next video only, the size after a restart
void applySubtitleLook(MainCtx *ctx, const QHash<QString, QVariant> &values)
{
    vlc_player_t *player = ctx->getIntf()->p_player;
    if (!player)
        return;
    const auto scale = values.constFind(QStringLiteral("sub-text-scale"));
    if (scale != values.cend())
        vlc_player_SetSubtitleTextScale(player, std::clamp<int>(scale->toInt(), 10, 500));
    bool text = false;
    for (auto it = values.cbegin(); it != values.cend() && !text; ++it)
        text = it.key().startsWith(QLatin1String("freetype-")) || it.key() == QLatin1String("text-renderer");
    if (!text)
        return;
    size_t count = 0;
    vout_thread_t **vouts;
    {
        vlc_player_Lock(player);
        vouts = vlc_player_vout_HoldAll(player, &count);
        vlc_player_Unlock(player);
    }
    for (size_t i = 0; i < count; ++i)
    {
        var_TriggerCallback(vouts[i], "sub-text-reload");
        vout_Release(vouts[i]);
    }
    free(vouts);
}

} // namespace

// VLC options written for a combined entry (see addCombinedEntries)
bool ConfigModel::combinedValues(const Entry &entry, QHash<QString, QVariant> *values,
                                 QHash<QString, QVariant> *expected)
{
    const qlonglong mode = entry.staged.toLongLong();
    QStringList keys;
    if (entry.key == QLatin1String(repeatKey)) {
        values->insert(QStringLiteral("repeat"), mode == 1);
        values->insert(QStringLiteral("loop"), mode == 2);
        keys = { QStringLiteral("repeat"), QStringLiteral("loop") };
    } else if (entry.key == QLatin1String(playbackEndKey)) {
        values->insert(QStringLiteral("play-and-exit"), mode == 1);
        keys = { QStringLiteral("play-and-exit") };
    } else if (entry.key == QLatin1String(seekPreciseKey)) {
        values->insert(QStringLiteral("input-fast-seek"), !entry.staged.toBool());
        keys = { QStringLiteral("input-fast-seek") };
    } else if (entry.key == QLatin1String(postprocKey)) {
        QStringList filters = ConfigValues::read(QStringLiteral("video-filter")).toString()
                                  .split(':', Qt::SkipEmptyParts);
        filters.removeAll(QStringLiteral("postproc"));
        if (mode > 0) {
            filters.append(QStringLiteral("postproc"));
            values->insert(QStringLiteral("postproc-q"), QVariant::fromValue<int64_t>(mode));
            keys.append(QStringLiteral("postproc-q"));
        }
        values->insert(QStringLiteral("video-filter"), filters.join(':'));
        keys.append(QStringLiteral("video-filter"));
    } else if (entry.key == QLatin1String(visualSizeKey)) {
        // the size of the user stays while it is chosen
        if (mode < 0 || mode >= qlonglong(std::size(visualSizes)))
            return true;
        values->insert(QStringLiteral("effect-width"), QVariant::fromValue<int64_t>(visualSizes[mode][0]));
        values->insert(QStringLiteral("effect-height"), QVariant::fromValue<int64_t>(visualSizes[mode][1]));
        keys = { QStringLiteral("effect-width"), QStringLiteral("effect-height") };
    } else if (entry.key == QLatin1String(visualizerKey)) {
        // an effect of the visualizer filter, or a visualization
        const QString value = entry.staged.toString();
        if (QStringList{ QStringLiteral("spectrometer"), QStringLiteral("scope"), QStringLiteral("spectrum"),
                         QStringLiteral("waveform"), QStringLiteral("vuMeter") }.contains(value)) {
            values->insert(QStringLiteral("audio-visual"), QStringLiteral("visual"));
            values->insert(QStringLiteral("effect-list"), value);
            keys = { QStringLiteral("audio-visual"), QStringLiteral("effect-list") };
        } else {
            values->insert(QStringLiteral("audio-visual"), value);
            keys = { QStringLiteral("audio-visual") };
        }
    } else if (entry.key == QLatin1String(customSkinKey)) {
        // Off: back to the default interface, unless another one was chosen
        const QString intf = ConfigValues::read(QStringLiteral("intf")).toString();
        values->insert(QStringLiteral("intf"), entry.staged.toBool() ? QStringLiteral("skins2")
                                               : intf == QLatin1String("skins2") ? QString() : intf);
        keys = { QStringLiteral("intf") };
    } else {
        return false;
    }
    for (const QString &key : keys)
        expected->insert(key, ConfigValues::read(key));
    return true;
}

bool ConfigModel::applyWithEffects(AudioEffects *audio, VideoEffects *video)
{
    const auto cancelPreview = [audio, video]() {
        bool restored = true;
        if (audio && !audio->cancel()) restored = false;
        if (video && !video->cancel()) restored = false;
        return restored;
    };
    if (!m_ctx || !m_ctx->getIntf()) {
        setError(qtr("VLC preferences are unavailable."));
        cancelPreview();
        return false;
    }
    if ((audio && audio->ctx() != m_ctx) || (video && video->ctx() != m_ctx)) {
        setError(qtr("The effect preview belongs to another VLC window."));
        cancelPreview();
        return false;
    }

    QHash<QString, QVariant> values, expected;
    QVector<int> changed, changedQt;
    for (int i = 0; i < m_entries.size(); ++i) {
        const Entry& entry = m_entries[i];
        if (entry.staged == entry.current) continue;
        if (entry.qtSetting) {
            changedQt.append(i);
            continue;
        }
        // written by the option it stands for
        if (!entry.proxyFor.isEmpty())
            continue;
        if (readValue(entry) != entry.current) {
            setError(qtr("%1 changed outside the settings. Discard your changes to see its new value.")
                         .arg(displayName(entry.key)));
            cancelPreview();
            return false;
        }
        changed.append(i);
        if (!combinedValues(entry, &values, &expected)) {
            values.insert(entry.key, entry.staged);
            expected.insert(entry.key, entry.current);
        }
    }
    if (audio && !audio->prepareConfiguration(values, expected)) {
        setError(audio->lastError());
        cancelPreview();
        return false;
    }
    if (video && !video->prepareConfiguration(values, expected)) {
        setError(video->lastError());
        cancelPreview();
        return false;
    }

    QString conflict;
    int result = 0;
    if (!ConfigValues::commit(m_ctx->getIntf(), values, expected, &conflict, &result)) {
        const bool restored = cancelPreview();
        QString preview;
        if (!restored)
            preview = QStringLiteral(" ") + qtr("The playback preview could not be restored.");
        if (result)
            setError(qtr("Could not save the settings of VLC (error %1). Check that its configuration file can be written.")
                         .arg(result) + preview);
        else
            setError(qtr("%1 changed outside the settings or is unavailable. Your changes were not saved.")
                         .arg(displayName(conflict)) + preview);
        return false;
    }
    if (audio) audio->finalizeConfiguration();
    if (video) video->finalizeConfiguration();
    applySubtitleLook(m_ctx, values);
    for (int i : changedQt)
        m_ctx->setSettingValue(m_entries[i].key, m_entries[i].staged);
    changed += changedQt;
    // the values VLC runs with until it restarts (the first change of each
    // setting): the rows of the settings read at the start say they wait
    // for it; the rows standing for other options as well (after syncDerived)
    QVector<QVariant> before;
    before.reserve(m_entries.size());
    for (const Entry &entry : m_entries)
        before.append(entry.current);
    for (int i : changed) {
        m_entries[i].current = m_entries[i].staged;
        notifyEntry(i);
    }
    syncDerived();
    bool orderChanged = false;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].current == before[i])
            continue;
        if (!startValues().contains(m_entries[i].key))
            startValues().insert(m_entries[i].key, before[i]);
        orderChanged = orderChanged || m_entries[i].key == QLatin1String(repeatKey)
                       || m_entries[i].key == QLatin1String(playbackEndKey)
                       || m_entries[i].key == QLatin1String("random")
                       || m_entries[i].key == QLatin1String("play-and-stop");
        notifyEntry(i);
    }
    // Repeat, Shuffle, Stop after each item, When playback finishes: on the
    // play queue now (they are its state as well as its defaults)
    if (orderChanged)
        m_ctx->applyPlayOrder();
    if (!changed.isEmpty()) {
        // Interface options (pinned controls, seek thumbnail...) apply now
        m_ctx->reloadPrefs();
        emit dirtyChanged();
    }
    setError({});
    return true;
}
