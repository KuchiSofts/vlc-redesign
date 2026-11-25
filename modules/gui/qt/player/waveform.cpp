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
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcess>
#include <QThread>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <utility>

#ifdef Q_OS_WIN
# include <windows.h>
#endif

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
// a source is analyzed once it has stayed this long: a film's sound comes a
// moment before its picture, and its picture goes first when it stops
constexpr int settleMs = 1000;

// an analysis, run by its own thread: Windows may check a new program for
// seconds before it starts, the interface goes on meanwhile
struct Job {
    QString key;
    QString path;
    int seconds = 0;
    std::atomic_bool cancel{false};
    QVariantList bins;
    QString error;
};

struct CacheEntry { QString key; QVariantList bins; };
// The GUI thread owns the queue and reads results after the worker joins.
QList<CacheEntry> s_cache;
QList<std::shared_ptr<Job>> s_pending;
std::shared_ptr<Job> s_active;
QThread* s_thread = nullptr;
bool s_stopAllAtEnd = false;
QList<Waveform*> s_instances;

bool hasAnalysis(const QString& key)
{
    return (s_active && !s_active->cancel && s_active->key == key) ||
        std::any_of(s_pending.cbegin(), s_pending.cend(),
                    [&key](const auto& job) { return job->key == key; });
}
}

Waveform::Waveform(QObject* parent) : QObject(parent)
{
    s_instances.append(this);
    m_settle.setSingleShot(true);
    m_settle.setInterval(settleMs);
    connect(&m_settle, &QTimer::timeout, this, &Waveform::enqueueAnalysis);
}

Waveform::~Waveform()
{
    s_instances.removeAll(this);
    release(true);
}

// the analyses end before the Qt module goes: at the end of the application
void Waveform::stopAll()
{
    s_stopAllAtEnd = false;
    s_pending.clear();
    if (auto job = std::exchange(s_active, {})) {
        job->cancel = true;
        s_thread->wait();
        delete std::exchange(s_thread, nullptr);
    }
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
    const QPointer<Waveform> guard(this);
    const quint64 generation = m_generation;
    emit binsChanged();
    if (guard && m_generation == generation) emit stateChanged();
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
    m_settle.stop();
    ++m_generation;
    const QString key = std::exchange(m_key, QString());
    if (key.isEmpty()) return;
    for (const Waveform* other : std::as_const(s_instances))
        if (other != this && other->m_key == key) return;
    s_pending.removeIf([&key](const auto& job) { return job->key == key; });
    const auto job = s_active;
    if (!job || job->key != key || job->cancel) return;
    if (now) {
        job->cancel = true;
        return;
    }
    QTimer::singleShot(orphanMs, QCoreApplication::instance(), [key, job] {
        if (s_active != job) return;
        for (const Waveform* waveform : std::as_const(s_instances))
            if (waveform->m_key == key) return;
        job->cancel = true;
    });
}

void Waveform::finishAnalysis(const QString& key, const QVariantList& bins, const QString& error)
{
    if (bins.size() == 512) {
        s_cache.prepend({key, bins});
        while (s_cache.size() > maxCacheEntries) s_cache.removeLast();
    }
    // the signals may destroy sliders
    QList<std::pair<QPointer<Waveform>, quint64>> waiting;
    for (Waveform* waveform : std::as_const(s_instances))
        if (waveform->m_key == key) waiting.append({waveform, waveform->m_generation});
    for (const auto& [waveform, generation] : std::as_const(waiting)) {
        if (!waveform || waveform->m_generation != generation || waveform->m_key != key) continue;
        waveform->m_settle.stop();
        waveform->m_key.clear();
        if (bins.size() == 512) waveform->setBins(bins);
        else waveform->setError(error);
        if (waveform && waveform->m_generation == generation) emit waveform->stateChanged();
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
    const bool scheduled = hasAnalysis(key);
    if (!scheduled && !QFileInfo::exists(helperPath())) {
        setError(tr("Waveform analyzer is unavailable."));
        return;
    }
    m_key = key;
    m_path = path;
    emit stateChanged();
    if (!scheduled)
        m_settle.start();
}

void Waveform::enqueueAnalysis()
{
    if (m_key.isEmpty() || hasAnalysis(m_key)) return;
    auto job = std::make_shared<Job>();
    job->key = m_key;
    job->path = m_path;
    job->seconds = m_durationSeconds;
    s_pending.append(std::move(job));
    startNextAnalysis();
}

void Waveform::startNextAnalysis()
{
    if (s_active || s_pending.isEmpty()) return;
    if (!std::exchange(s_stopAllAtEnd, true))
        qAddPostRoutine(stopAll);

    const auto job = s_pending.takeFirst();
    s_active = job;
    const QString program = helperPath();
    s_thread = QThread::create([job, program] {
        if (job->cancel) return;
        QProcess process;
#ifdef Q_OS_WIN
        // Give playback priority over the decoder, including its own threads.
        process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* arguments) {
            arguments->flags |= CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS;
        });
#endif
        // libvlc_media_new_path() of the helper wants native separators on Windows
        process.start(program, {QDir::toNativeSeparators(job->path)});
        const bool started = process.waitForStarted();
        if (started) {
            // a helper that hangs does not keep the sliders busy
            QElapsedTimer elapsed;
            elapsed.start();
            while (!process.waitForFinished(100) && process.state() != QProcess::NotRunning) {
                if (job->cancel || elapsed.hasExpired(watchdogMs)) {
                    process.kill();
                    process.waitForFinished(1000);
                    break;
                }
            }
            if (!job->cancel && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
                job->bins = parseOutput(process.readAllStandardOutput(), job->seconds);
        }
        if (!started)
            job->error = tr("Waveform analyzer could not start.");
        else if (job->bins.size() != 512)
            job->error = tr("Waveform analysis did not complete.");
    });
    connect(s_thread, &QThread::finished, QCoreApplication::instance(), [job] {
        if (s_active != job) return;
        // Keep the slot reserved through cancellation until QProcess has
        // reaped the helper. This job also identifies this run of the key.
        s_thread->wait();
        delete std::exchange(s_thread, nullptr);
        s_active.reset();
        if (!job->cancel)
            finishAnalysis(job->key, job->bins, job->error);
        startNextAnalysis();
    }, Qt::QueuedConnection);
    s_thread->start(QThread::LowPriority);
}
