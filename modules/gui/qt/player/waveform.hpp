/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/*****************************************************************************
 * waveform.hpp: Local audio envelope request for the Qt Quick player
 *****************************************************************************/
#ifndef VLC_QT_WAVEFORM_HPP_
#define VLC_QT_WAVEFORM_HPP_

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Waveform : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(int durationSeconds READ durationSeconds WRITE setDurationSeconds NOTIFY durationSecondsChanged FINAL)
    Q_PROPERTY(QVariantList bins READ bins NOTIFY binsChanged FINAL)
    Q_PROPERTY(bool ready READ ready NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY stateChanged FINAL)

public:
    explicit Waveform(QObject* parent = nullptr);
    ~Waveform() override;

    QUrl source() const { return m_source; }
    void setSource(const QUrl& source);
    int durationSeconds() const { return m_durationSeconds; }
    void setDurationSeconds(int seconds);
    QVariantList bins() const { return m_bins; }
    bool ready() const { return m_bins.size() == 512; }
    bool busy() const { return !m_key.isEmpty(); }
    QString lastError() const { return m_lastError; }

    // Where the envelopes are kept between two runs of VLC (a folder of its
    // cache, set once by the interface): a song analyzed before shows its
    // waveform at once, without decoding it again. Not set: kept in memory
    // for this run only. One small file for each song, named by a hash of
    // the song's path, size and date: no name of a file is written there.
    static void setCacheFolder(const QString& folder);

signals:
    void sourceChanged();
    void durationSecondsChanged();
    void binsChanged();
    void stateChanged();

private:
    void refresh();
    void release(bool now);
    void setBins(const QVariantList& bins);
    void setError(const QString& error);
    void enqueueAnalysis();
    static void startNextAnalysis();
    static void stopAll();
    static void finishAnalysis(const QString& key, const QVariantList& bins, const QString& error);
    static QString helperPath();
    static QVariantList parseOutput(const QByteArray& bytes, int expectedSeconds);
    static void remember(const QString& key, const QVariantList& bins);
    static QVariantList storedBins(const QString& key);
    static void storeBins(const QString& key, const QVariantList& bins);

    QUrl m_source;
    int m_durationSeconds = 0;
    QVariantList m_bins;
    // the analysis waited for (shared by the sliders of the same file)
    QString m_key;
    QString m_path;
    quint64 m_generation = 0;
    // a source is analyzed once it stays: not the passing ones
    QTimer m_settle;
    QString m_lastError;
};

#endif
