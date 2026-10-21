/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/*****************************************************************************
 * waveform.cpp: Cancellable local audio envelope analysis
 *****************************************************************************/
#include "waveform.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr qint64 maxFileBytes = 1024LL * 1024LL * 1024LL;
constexpr int maxDurationSeconds = 8 * 60;
constexpr qsizetype maxJsonBytes = 64 * 1024;
// shared by the sliders (the player's, the mini player's): an analysis
// decodes the whole file
constexpr int maxCacheEntries = 16;
// a helper that hangs is killed after a minute (8 minutes of sound,
// decoded 16 times faster than they play, take 30 s)
constexpr int watchdogMs = 60 * 1000;
// a helper nobody waits for any more is killed after this: a slider
// hidden and shown again (the player, the library) goes on with it
constexpr int orphanMs = 2 * 1000;

struct CacheEntry { QString key; QVariantList bins; };
// all in the GUI thread
QList<CacheEntry> s_cache;
QHash<QString, QPointer<QProcess>> s_jobs;
QList<Waveform*> s_instances;
}

Waveform::Waveform(QObject* parent) : QObject(parent)
{
    s_instances.append(this);
}

Waveform::~Waveform()
{
    s_instances.removeAll(this);
    release(true);
}

void Waveform::setSource(const QUrl& source)
{
    if (m_source == source) return;
    m_source = source;
    emit sourceChanged();
    refresh();
}

void Waveform::setDurationSeconds(int seconds)
{
    if (m_durationSeconds == seconds) return;
    m_durationSeconds = seconds;
    emit durationSecondsChanged();
    refresh();
}

void Waveform::setBins(const QVariantList& bins)
{
    if (m_bins == bins) return;
    m_bins = bins;
    emit binsChanged();
    emit stateChanged();
}

void Waveform::setError(const QString& error)
{
    if (m_lastError == error) return;
    m_lastError = error;
    emit stateChanged();
}

// stops waiting for the analysis; the helper stops too unless another
// slider waits for it
void Waveform::release(bool now)
{
    const QString key = std::exchange(m_key, QString());
    if (key.isEmpty()) return;
    for (const Waveform* other : std::as_const(s_instances))
        if (other != this && other->m_key == key) return;
    QProcess* process = s_jobs.value(key);
    if (!process) return;
    if (now) {
        // finished() deletes it
        s_jobs.remove(key);
        process->kill();
        return;
    }
    QTimer::singleShot(orphanMs, process, [key, process] {
        if (s_jobs.value(key) != process) return;
        for (const Waveform* waveform : std::as_const(s_instances))
            if (waveform->m_key == key) return;
        s_jobs.remove(key);
        process->kill();
    });
}

void Waveform::finishAnalysis(const QString& key, const QVariantList& bins, const QString& error)
{
    if (bins.size() == 512) {
        s_cache.prepend({key, bins});
        while (s_cache.size() > maxCacheEntries) s_cache.removeLast();
    }
    // the signals may destroy sliders
    QList<QPointer<Waveform>> waiting;
    for (Waveform* waveform : std::as_const(s_instances))
        if (waveform->m_key == key) waiting << waveform;
    for (const QPointer<Waveform>& waveform : std::as_const(waiting)) {
        if (!waveform || waveform->m_key != key) continue;
        waveform->m_key.clear();
        if (bins.size() == 512) waveform->setBins(bins);
        else waveform->setError(error);
        if (waveform) emit waveform->stateChanged();
    }
}

QString Waveform::helperPath()
{
#ifdef Q_OS_WIN
    return QCoreApplication::applicationDirPath() + "/vlc-waveform.exe";
#else
    return QCoreApplication::applicationDirPath() + "/vlc-waveform";
#endif
}

QVariantList Waveform::parseOutput(const QByteArray& bytes, int expectedSeconds)
{
    if (bytes.size() > maxJsonBytes) return {};
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return {};
    const QJsonObject object = document.object();
    const double duration = object.value("durationUs").toDouble() / 1000000.0;
    if (object.value("version").toInt() != 1 || object.value("decodedFrames").toDouble() <= 0 ||
        object.value("sampleRate").toInt() != 48000 ||
        std::abs(object.value("playbackRate").toDouble() - 16.0) > 0.01 ||
        !std::isfinite(duration) || duration <= 0 ||
        std::abs(duration - expectedSeconds) > std::max(2.0, expectedSeconds * 0.03))
        return {};
    const QJsonArray values = object.value("bins").toArray();
    if (values.size() != 512) return {};
    QVariantList result;
    result.reserve(values.size());
    for (const QJsonValue& value : values) {
        if (!value.isDouble()) return {};
        const double peak = value.toDouble();
        if (!std::isfinite(peak) || peak < 0 || peak > 16) return {};
        result << peak;
    }
    return result;
}

void Waveform::refresh()
{
    const bool wasBusy = busy();
    release(false);
    setBins({});
    setError({});
    if (wasBusy) emit stateChanged();

    if (!m_source.isLocalFile() || m_durationSeconds <= 0 ||
        m_durationSeconds > maxDurationSeconds) return;
    const QFileInfo file(m_source.toLocalFile());
    if (!file.isFile() || !file.isReadable() || file.size() > maxFileBytes) return;
    const QString path = file.canonicalFilePath();
    if (path.isEmpty()) return;
    const QString key = path + '|' + QString::number(file.size()) + '|' +
                        QString::number(file.lastModified().toMSecsSinceEpoch());
    for (int i = 0; i < s_cache.size(); ++i) {
        if (s_cache[i].key != key) continue;
        const CacheEntry hit = s_cache.takeAt(i);
        s_cache.prepend(hit);
        setBins(hit.bins);
        return;
    }
    // the analysis of another slider: its result comes here too
    const bool running = !s_jobs.value(key).isNull();
    if (!running && !QFileInfo::exists(helperPath())) {
        setError(tr("Waveform analyzer is unavailable."));
        return;
    }
    m_key = key;
    emit stateChanged();
    if (!running)
        startAnalysis(key, path, m_durationSeconds);
}

void Waveform::startAnalysis(const QString& key, const QString& path, int seconds)
{
    // not a child of a slider: the analysis outlives the one that started it
    auto* process = new QProcess(QCoreApplication::instance());
    s_jobs.insert(key, process);
#ifdef Q_OS_WIN
    // CREATE_NO_WINDOW: keep the analysis helper invisible beside the GUI.
    process->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* arguments) {
        arguments->flags |= 0x08000000;
    });
#endif
    process->setProgram(helperPath());
    // libvlc_media_new_path() of the helper wants native separators on Windows
    process->setArguments({QDir::toNativeSeparators(path)});
    connect(process, &QProcess::errorOccurred, process,
            [process, key](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        // no finished() follows
        process->deleteLater();
        if (s_jobs.value(key) != process) return;
        s_jobs.remove(key);
        finishAnalysis(key, {}, tr("Waveform analyzer could not start."));
    });
    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), process,
            [process, key, seconds](int exitCode, QProcess::ExitStatus status) {
        process->deleteLater();
        if (s_jobs.value(key) != process) return;
        s_jobs.remove(key);
        const QVariantList result = status == QProcess::NormalExit && exitCode == 0
            ? parseOutput(process->readAllStandardOutput(), seconds) : QVariantList{};
        finishAnalysis(key, result, result.size() == 512
                                    ? QString() : tr("Waveform analysis did not complete."));
    });
    // a helper that hangs does not keep the sliders busy
    QTimer::singleShot(watchdogMs, process, [process, key] {
        if (s_jobs.value(key) != process) return;
        s_jobs.remove(key);
        process->kill();
        finishAnalysis(key, {}, tr("Waveform analysis did not complete."));
    });
    process->start();
}
