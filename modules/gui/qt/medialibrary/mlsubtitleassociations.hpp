/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *****************************************************************************/
#pragma once

#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QVariantList>
#include <QVector>

#include <functional>

#include "mlqmltypes.hpp"

Q_MOC_INCLUDE("medialibrary/medialib.hpp")

class MediaLib;

class MLSubtitleAssociations : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MediaLib* ml READ ml WRITE setMl)
    Q_PROPERTY(QVariantList destinations READ destinations NOTIFY changed)
    Q_PROPERTY(QVariantList sources READ sources NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QVariantList targets READ targets NOTIFY targetsChanged)
    Q_PROPERTY(int selectedTarget READ selectedTarget NOTIFY targetsChanged)
    // the folder where the subtitle files were looked for
    Q_PROPERTY(QUrl folder READ folder NOTIFY changed)
    // audio files (soundtracks) instead of subtitles
    Q_PROPERTY(bool audio READ audio WRITE setAudio NOTIFY changed)

public:
    explicit MLSubtitleAssociations(QObject* parent = nullptr);

    MediaLib* ml() const;
    void setMl(MediaLib* ml);
    QVariantList destinations() const;
    QVariantList sources() const;
    QString error() const { return m_error; }
    bool busy() const { return m_busy; }
    QVariantList targets() const;
    int selectedTarget() const { return m_selectedTarget; }
    QUrl folder() const { return m_folder; }
    bool audio() const { return m_audio; }
    void setAudio(bool audio);

    Q_INVOKABLE void open(const MLItemId& target);
    Q_INVOKABLE void openUrl(const QUrl& target);
    // the media files of a folder of the disk
    Q_INVOKABLE void openFolderUrl(const QUrl& folder);
    Q_INVOKABLE void addFiles(const QVariantList& urls);
    Q_INVOKABLE void chooseFiles();
    Q_INVOKABLE void chooseFolder();
    Q_INVOKABLE void addLink(const QUrl& url);
    Q_INVOKABLE void addLinkWithName(const QUrl& url, const QString& filename);
    Q_INVOKABLE void selectTarget(int index);
    Q_INVOKABLE void assign(int sourceIndex, int destinationIndex);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void save();

    // a subtitle (or audio) file to link to a media: by its id, else by its
    // MRL (a media of a folder not in the library is added to it)
    struct Link { int64_t mediaId; QByteArray mediaMrl; QByteArray fileMrl; };
    // Links them on the thread of the media library, then done(the links
    // added, the error that stopped them) on context's; a link there
    // already is not added again (nor counted)
    static void link(MediaLib* ml, const QObject* context, const QVector<Link>& links, bool audio,
                     std::function<void(int saved, const QString& error)> done);
    // the MRL of a local file as the media library keeps it
    static QByteArray mrlOf(const QUrl& url);

signals:
    void changed();
    void saved(int count);
    void targetsChanged();

private:
    struct Destination {
        int64_t id = 0;
        QUrl url;
        QString label;
    };
    struct Source {
        QUrl url;
        int destination = -1;
        QString issue;
        QString label;
        // found in the folder (checked only when its name is the media's)
        bool scanned = false;
    };
    struct Target {
        MLItemId id;
        QUrl url;
        QString label;
    };

    void setError(const QString& error);
    void match();
    void loadTarget(const MLItemId& target);
    void loadUrlTarget(const QUrl& target);
    void refreshTargets(const Target& initial);
    void scanFolder(const QUrl& folder);
    // a subtitle file, or an audio file in audio
    bool accepts(const QString& filename) const;
    bool m_audio = false;
    QUrl m_folder;
    QPointer<MediaLib> m_ml;
    QVector<Destination> m_destinations;
    QVector<Source> m_sources;
    QVector<Target> m_targets;
    int m_selectedTarget = 0;
    quint64 m_targetRevision = 0;
    QString m_error;
    bool m_busy = false;
    bool m_saving = false;
    quint64 m_revision = 0;
};
