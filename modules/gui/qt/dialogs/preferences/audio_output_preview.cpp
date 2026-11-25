/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "audio_output_preview.hpp"
#include "audio_output_device.hpp"
#include "maininterface/mainctx.hpp"
#include "qt.hpp"

#include <vlc_common.h>
#include <vlc_aout.h>
#include <vlc_player.h>
#include <cstdlib>

AudioOutputPreview::AudioOutputPreview(QObject* parent) : QObject(parent) {}
AudioOutputPreview::~AudioOutputPreview() { cancel(); }
MainCtx* AudioOutputPreview::ctx() const { return m_ctx.data(); }

void AudioOutputPreview::setCtx(MainCtx* ctx)
{
    if (m_ctx == ctx) return;
    cancel();
    m_ctx = ctx;
    emit ctxChanged();
}

bool AudioOutputPreview::fail(const QString& message)
{
    m_error = message;
    emit errorChanged();
    return false;
}

void AudioOutputPreview::clearError()
{
    if (m_error.isEmpty()) return;
    m_error.clear();
    emit errorChanged();
}

void AudioOutputPreview::releaseOutput()
{
    if (m_aout) aout_Release(m_aout);
    m_aout = nullptr;
    m_originalDevice.clear();
    m_originalIsDefault = true;
    m_dirty = false;
    emit stateChanged();
}

bool AudioOutputPreview::sameOutput() const
{
    if (!m_aout || !m_ctx || !m_ctx->getIntf()) return false;
    audio_output_t* current = vlc_player_aout_Hold(m_ctx->getIntf()->p_player);
    const bool same = current == m_aout;
    if (current) aout_Release(current);
    return same;
}

bool AudioOutputPreview::begin()
{
    if (m_aout && sameOutput()) return true;
    if (m_aout) {
        const bool changed = m_dirty;
        releaseOutput();
        if (changed) return fail(qtr("Audio output changed; the previous device could not be restored."));
    }
    clearError();
    if (!m_ctx || !m_ctx->getIntf() || !m_ctx->getIntf()->p_player) return false;
    m_aout = vlc_player_aout_Hold(m_ctx->getIntf()->p_player);
    if (!m_aout) return false;
    char* original = aout_DeviceGet(m_aout);
    m_originalIsDefault = original == nullptr;
    m_originalDevice = original ? QByteArray(original) : QByteArray();
    free(original);
    m_dirty = false;
    emit stateChanged();
    return true;
}

int AudioOutputPreview::currentIndex() const
{
    if (!sameOutput()) return -1;
    char* current = aout_DeviceGet(m_aout);
    const QByteArray selected = current ? QByteArray(current) : QByteArray();
    free(current);
    char** ids = nullptr;
    char** names = nullptr;
    const int count = aout_DevicesList(m_aout, &ids, &names);
    int selectedIndex = -1;
    for (int i = 0; i < count; ++i) {
        if (ids && (ids[i] ? QByteArray(ids[i]) : QByteArray()) == selected)
            selectedIndex = i;
        if (ids) free(ids[i]);
        if (names) free(names[i]);
    }
    free(ids);
    free(names);
    return selectedIndex;
}

bool AudioOutputPreview::preview(const QString& chosenId)
{
    if (!m_aout || !sameOutput())
        return fail(qtr("The audio output changed. Reopen Audio settings to choose a device."));
    char** ids = nullptr;
    char** names = nullptr;
    const int count = aout_DevicesList(m_aout, &ids, &names);
    if (count < 0) return fail(qtr("The available output devices changed. Reopen Audio settings."));

    const QByteArray id = chosenId.toUtf8();
    const bool matches = audioOutputDeviceExists(ids, count, std::string_view(id.constData(), id.size()));
    for (int i = 0; i < count; ++i) {
        free(ids[i]);
        free(names[i]);
    }
    free(ids);
    free(names);
    if (!matches) return fail(qtr("The available output devices changed. Reopen Audio settings."));
    const bool chosenDefault = chosenId.isEmpty();
    if (aout_DeviceSet(m_aout, chosenDefault ? nullptr : id.constData()) != 0)
        return fail(qtr("Could not select this output device."));
    m_dirty = chosenDefault != m_originalIsDefault
              || (!chosenDefault && id != m_originalDevice);
    clearError();
    emit stateChanged();
    return true;
}

bool AudioOutputPreview::canCommit()
{
    return !m_dirty || sameOutput()
        || fail(qtr("The audio output changed; the device preview cannot be kept."));
}

bool AudioOutputPreview::commit()
{
    if (!canCommit()) return false;
    releaseOutput();
    clearError();
    return true;
}

bool AudioOutputPreview::cancel()
{
    if (!m_aout) return true;
    if (!m_dirty) { releaseOutput(); clearError(); return true; }
    const bool same = sameOutput();
    int result = 0;
    if (m_dirty && same)
        result = aout_DeviceSet(m_aout, m_originalIsDefault ? nullptr : m_originalDevice.constData());
    releaseOutput();
    if (!same) return fail(qtr("Audio output changed; the previous device could not be restored."));
    if (result != 0) return fail(qtr("The previous audio device could not be restored."));
    clearError();
    return true;
}
