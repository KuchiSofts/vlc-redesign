/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *****************************************************************************/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "mlsubtitleassociations.hpp"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QSet>

#include <utility>

#include <vlc_input_item.h>
#include <vlc_interface.h>
#include <vlc_media_library.h>

#include "medialib.hpp"
#include "mlhelper.hpp"
#include "qt.hpp"

namespace {
QString baseName(const QUrl& url)
{
    return QFileInfo(url.toLocalFile()).completeBaseName().toCaseFolded();
}

QString baseName(const QString& filename)
{
    return QFileInfo(filename).completeBaseName().toCaseFolded();
}

QUrl fileUrl(const QVariant& value)
{
    QUrl url = value.toUrl();
    if (url.isValid() && url.isLocalFile())
        return QUrl::fromLocalFile(QFileInfo(url.toLocalFile()).absoluteFilePath());
    return {};
}

// The MRL of a local file as the core and the media library write it
// (vlc_path2uri): QUrl::FullyEncoded leaves ( ) ' ! & + , ; = @ $ as they
// are, the media would not be found and a second one would be created
QByteArray coreMrl(const QUrl& url)
{
    return MediaLib::mrlFromUrl(url).toUtf8();
}
}

MLSubtitleAssociations::MLSubtitleAssociations(QObject* parent) : QObject(parent) {}

MediaLib* MLSubtitleAssociations::ml() const
{
    return m_ml;
}

void MLSubtitleAssociations::setMl(MediaLib* ml)
{
    m_ml = ml;
}

QVariantList MLSubtitleAssociations::destinations() const
{
    QVariantList list;
    for (const auto& destination : m_destinations)
        list.push_back(QVariantMap{{"label", destination.label}, {"url", destination.url}});
    return list;
}

QVariantList MLSubtitleAssociations::sources() const
{
    QVariantList list;
    for (const auto& source : m_sources)
        list.push_back(QVariantMap{{"label", source.label.isEmpty() ?
                                       QFileInfo(source.url.toLocalFile()).fileName() : source.label},
                                   {"url", source.url}, {"destination", source.destination},
                                   {"issue", source.issue}});
    return list;
}

void MLSubtitleAssociations::setError(const QString& error)
{
    m_error = error;
    emit changed();
}

void MLSubtitleAssociations::cancel()
{
    if (m_saving)
        return;
    // Also drops the reply of a link download in flight.
    ++m_revision;
    ++m_targetRevision;
    m_destinations.clear();
    m_sources.clear();
    m_targets.clear();
    m_selectedTarget = 0;
    m_folder.clear();
    m_error.clear();
    m_busy = false;
    emit changed();
    emit targetsChanged();
}

void MLSubtitleAssociations::open(const MLItemId& target)
{
    if (m_saving)
        return;
    cancel();
    refreshTargets({target, {}, tr("This media")});
    loadTarget(target);
}

void MLSubtitleAssociations::loadTarget(const MLItemId& target)
{
    if (!m_ml || !target.isValid() ||
        (target.type != VLC_ML_PARENT_UNKNOWN && target.type != VLC_ML_PARENT_FOLDER &&
         target.type != VLC_ML_PARENT_PLAYLIST))
    {
        setError(tr("Select a media file, folder, or playlist from the library."));
        return;
    }

    ++m_revision;
    m_destinations.clear();
    m_error.clear();
    for (auto& source : m_sources)
        source.destination = -1;
    m_busy = true;
    emit changed();
    const quint64 revision = m_revision;
    struct Context { QVector<Destination> destinations; };
    m_ml->runOnMLThread<Context>(this,
        [target](vlc_medialibrary_t* ml, Context& ctx) {
            auto append = [&ctx](const vlc_ml_media_t& media) {
                if (!media.p_files)
                    return;
                for (size_t i = 0; i < media.p_files->i_nb_items; ++i)
                {
                    const auto& file = media.p_files->p_items[i];
                    if (file.i_type != VLC_ML_FILE_TYPE_MAIN || !file.psz_mrl)
                        continue;
                    QUrl url = QUrl::fromEncoded(file.psz_mrl);
                    if (!url.isLocalFile())
                        return;
                    ctx.destinations.push_back({media.i_id, url, QFileInfo(url.toLocalFile()).fileName()});
                    return;
                }
            };
            if (target.type == VLC_ML_PARENT_UNKNOWN)
            {
                ml_unique_ptr<vlc_ml_media_t> media(vlc_ml_get_media(ml, target.id));
                if (media)
                    append(*media);
            }
            else
            {
                vlc_ml_query_params_t query = vlc_ml_query_params_create();
                ml_unique_ptr<vlc_ml_media_list_t> list(vlc_ml_list_media_of(ml, &query, target.type, target.id));
                if (!list)
                    return;
                QSet<int64_t> seen;
                for (size_t i = 0; i < list->i_nb_items; ++i)
                {
                    const auto& media = list->p_items[i];
                    if (!seen.contains(media.i_id))
                    {
                        append(media);
                        seen.insert(media.i_id);
                    }
                }
            }
        },
        [this, revision](quint64, Context& ctx) {
            if (revision != m_revision)
                return;
            m_destinations = std::move(ctx.destinations);
            m_busy = false;
            if (m_destinations.isEmpty())
                m_error = tr("No local media files were found in this destination.");
            else
                scanFolder(QUrl::fromLocalFile(QFileInfo(m_destinations.front().url.toLocalFile()).absolutePath()));
            match();
        });
}

void MLSubtitleAssociations::openUrl(const QUrl& target)
{
    if (m_saving)
        return;
    cancel();
    refreshTargets({{}, target, tr("This file")});
    loadUrlTarget(target);
}

void MLSubtitleAssociations::setAudio(bool audio)
{
    if (m_audio == audio || m_busy)
        return;
    m_audio = audio;
    // the files listed were of the other kind
    m_sources.clear();
    m_error.clear();
    emit changed();
}

bool MLSubtitleAssociations::accepts(const QString& filename) const
{
    enum slave_type type;
    const QByteArray name = QFile::encodeName(filename);
    return input_item_slave_GetType(name.constData(), &type)
        && type == (m_audio ? SLAVE_TYPE_GENERIC : SLAVE_TYPE_SPU);
}

void MLSubtitleAssociations::openFolderUrl(const QUrl& folder)
{
    if (m_saving)
        return;
    cancel();
    refreshTargets({{}, folder, tr("This folder")});

    ++m_revision;
    m_destinations.clear();
    m_error.clear();
    const QUrl url = fileUrl(folder);
    const QDir dir(url.toLocalFile());
    if (url.isEmpty() || !dir.exists())
    {
        setError(tr("Choose an existing local folder."));
        return;
    }
    QStringList patterns;
    // audio tracks go with the videos
    const QString extensions = m_audio ? QStringLiteral(EXTENSIONS_VIDEO)
                                       : QStringLiteral(EXTENSIONS_VIDEO ";" EXTENSIONS_AUDIO);
    for (const QString& pattern : extensions.split(QLatin1Char(';'), Qt::SkipEmptyParts))
        patterns.push_back(pattern.trimmed());
    const auto entries = dir.entryInfoList(patterns, QDir::Files | QDir::Readable, QDir::Name);
    for (const auto& info : entries)
        m_destinations.push_back({0, QUrl::fromLocalFile(info.absoluteFilePath()), info.fileName()});
    if (m_destinations.isEmpty())
        m_error = tr("No local media files were found in this destination.");
    scanFolder(QUrl::fromLocalFile(dir.absolutePath()));
    match();
}

void MLSubtitleAssociations::loadUrlTarget(const QUrl& target)
{
    ++m_revision;
    m_destinations.clear();
    m_error.clear();
    for (auto& source : m_sources)
        source.destination = -1;
    const QUrl url = fileUrl(target);
    if (url.isEmpty() || !QFileInfo(url.toLocalFile()).isFile())
    {
        setError(tr("Choose an existing local media file."));
        return;
    }
    m_destinations.push_back({0, url, QFileInfo(url.toLocalFile()).fileName()});
    scanFolder(QUrl::fromLocalFile(QFileInfo(url.toLocalFile()).absolutePath()));
    match();
}

void MLSubtitleAssociations::addFiles(const QVariantList& urls)
{
    if (m_busy)
        return;
    QSet<QString> seen;
    QVector<Source> pending;
    for (const auto& source : m_sources)
        seen.insert(source.url.toString(QUrl::FullyEncoded));
    for (const auto& value : urls)
    {
        const QUrl url = fileUrl(value);
        if (url.isEmpty())
        {
            setError(m_audio ? tr("Choose existing local audio files only.")
                             : tr("Choose existing local subtitle files only."));
            return;
        }
        if (seen.contains(url.toString(QUrl::FullyEncoded)))
            continue;
        const QFileInfo info(url.toLocalFile());
        if (!info.isFile() || !info.isReadable())
        {
            setError((m_audio ? tr("A selected audio file is missing or unreadable: %1")
                              : tr("A selected subtitle file is missing or unreadable: %1")).arg(info.fileName()));
            return;
        }
        if (!accepts(info.fileName()))
        {
            setError((m_audio ? tr("Unsupported audio file: %1")
                              : tr("Unsupported subtitle file: %1")).arg(info.fileName()));
            return;
        }
        seen.insert(url.toString(QUrl::FullyEncoded));
        pending.push_back({url, -1, {}, info.fileName()});
    }
    for (const auto& source : pending)
        m_sources.push_back(source);
    m_error.clear();
    match();
}

void MLSubtitleAssociations::chooseFiles()
{
    const auto urls = m_audio
        ? QFileDialog::getOpenFileUrls(nullptr, tr("Choose audio files"), {},
                                       tr("Audio files (*.aac *.ac3 *.dts *.dtshd *.eac3 *.flac *.m4a *.mp3 *.pcm);;All files (*)"),
                                       nullptr, QFileDialog::Options(), {"file"})
        : QFileDialog::getOpenFileUrls(nullptr, tr("Choose subtitle files"), {},
                                       tr("Subtitle files (*.srt *.ass *.ssa *.sub *.vtt *.ttml *.smi);;All files (*)"),
                                       nullptr, QFileDialog::Options(), {"file"});
    QVariantList values;
    for (const auto& url : urls)
        values.push_back(url);
    addFiles(values);
}

void MLSubtitleAssociations::scanFolder(const QUrl& folder)
{
    // the files found before go, the ones chosen stay
    QVector<Source> kept;
    for (const auto& source : m_sources)
        if (!source.scanned)
            kept.push_back(source);
    m_sources = std::move(kept);
    m_folder = folder;

    QSet<QString> seen;
    for (const auto& source : m_sources)
        seen.insert(source.url.toString(QUrl::FullyEncoded));
    // one media: the files named after it (The_Wand_S01E01_English.srt)
    const QString prefix = m_destinations.size() == 1 ? baseName(m_destinations.front().url) : QString();
    const auto entries = QDir(folder.toLocalFile()).entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
    for (const auto& info : entries)
    {
        if (!accepts(info.fileName()))
            continue;
        if (!prefix.isEmpty() && !baseName(info.fileName()).startsWith(prefix))
            continue;
        const QUrl url = QUrl::fromLocalFile(info.absoluteFilePath());
        if (seen.contains(url.toString(QUrl::FullyEncoded)))
            continue;
        Source source;
        source.url = url;
        source.label = info.fileName();
        source.scanned = true;
        m_sources.push_back(source);
    }
}

void MLSubtitleAssociations::chooseFolder()
{
    if (m_busy)
        return;
    const QUrl folder = QFileDialog::getExistingDirectoryUrl(nullptr, m_audio ? tr("Choose the folder of the audio tracks")
                                                                             : tr("Choose the folder of the subtitles"),
                                                             m_folder, QFileDialog::ShowDirsOnly, {"file"});
    if (!folder.isValid() || !folder.isLocalFile())
        return;
    m_error.clear();
    scanFolder(folder);
    match();
}

void MLSubtitleAssociations::match()
{
    QHash<QString, QVector<int>> destinationsByName;
    for (int i = 0; i < m_destinations.size(); ++i)
        destinationsByName[baseName(m_destinations[i].url)].push_back(i);

    for (auto& source : m_sources)
    {
        if (source.destination >= 0)
            continue;
        source.issue.clear();
        const auto matches = destinationsByName.value(baseName(source.label));
        // the files found in the folder: the one of the same name only
        if (source.scanned)
        {
            if (matches.size() == 1)
                source.destination = matches.front();
            continue;
        }
        if (matches.size() == 1)
            source.destination = matches.front();
        else if (m_destinations.size() == 1)
            source.destination = 0;
        else if (matches.size() > 1)
            source.issue = tr("Several media files have this name. Choose one.");
        else
            source.issue = tr("No exact filename match. Choose a destination.");
    }
    emit changed();
}

void MLSubtitleAssociations::assign(int sourceIndex, int destinationIndex)
{
    // -1 unchecks the file (not saved)
    if (m_busy || sourceIndex < 0 || sourceIndex >= m_sources.size() ||
        destinationIndex < -1 || destinationIndex >= m_destinations.size())
        return;
    m_sources[sourceIndex].destination = destinationIndex;
    m_sources[sourceIndex].issue.clear();
    m_error.clear();
    emit changed();
}

void MLSubtitleAssociations::save()
{
    if (!m_ml || m_busy || m_destinations.isEmpty() || m_sources.isEmpty())
    {
        setError(m_audio ? tr("Choose a destination and at least one audio file.")
                         : tr("Choose a destination and at least one subtitle file."));
        return;
    }
    bool any = false;
    for (const auto& source : m_sources)
    {
        // the files not checked are not saved
        if (source.destination < 0 || source.destination >= m_destinations.size())
            continue;
        any = true;
        if (!QFileInfo(source.url.toLocalFile()).isReadable())
        {
            setError((m_audio ? tr("An audio file is no longer readable: %1")
                              : tr("A subtitle file is no longer readable: %1")).arg(source.url.fileName()));
            return;
        }
    }

    if (!any)
    {
        setError(m_audio ? tr("Choose at least one audio file.") : tr("Choose at least one subtitle file."));
        return;
    }

    QVector<Link> links;
    for (const auto& source : m_sources)
    {
        if (source.destination < 0 || source.destination >= m_destinations.size())
            continue;
        const auto& destination = m_destinations[source.destination];
        links.push_back({destination.id, coreMrl(destination.url), coreMrl(source.url)});
    }

    m_busy = true;
    m_saving = true;
    m_error.clear();
    emit changed();
    const quint64 revision = m_revision;
    link(m_ml, this, links, m_audio, [this, revision](int count, const QString& error) {
        if (revision != m_revision)
            return;
        m_busy = false;
        m_saving = false;
        m_error = error;
        if (!m_error.isEmpty() && count > 0)
            m_error += tr(" %n association(s) were saved before this error; those changes remain saved.",
                          nullptr, count);
        emit changed();
        if (error.isEmpty())
            emit saved(count);
    });
}

QByteArray MLSubtitleAssociations::mrlOf(const QUrl& url)
{
    return coreMrl(url);
}

void MLSubtitleAssociations::link(MediaLib* library, const QObject* context, const QVector<Link>& links, bool audio,
                                  std::function<void(int saved, const QString& error)> done)
{
    const int fileType = audio ? VLC_ML_FILE_TYPE_SOUNDTRACK : VLC_ML_FILE_TYPE_SUBTITLE;
    struct Context { int saved = 0; QString error; };
    library->runOnMLThread<Context>(context,
        [links, audio, fileType](vlc_medialibrary_t* ml, Context& ctx) {
            for (const auto& entry : links)
            {
                const QUrl subtitle = QUrl::fromEncoded(entry.fileMrl);
                const QFileInfo file(subtitle.toLocalFile());
                if (!subtitle.isLocalFile() || !file.isFile() || !file.isReadable())
                {
                    ctx.error = (audio ? QObject::tr("An audio file is missing or unreadable: %1")
                                       : QObject::tr("A subtitle file is missing or unreadable: %1"))
                                    .arg(file.fileName());
                    return;
                }
            }
            for (const auto& entry : links)
            {
                ml_unique_ptr<vlc_ml_media_t> media(entry.mediaId ?
                    vlc_ml_get_media(ml, entry.mediaId) :
                    vlc_ml_get_media_by_mrl(ml, entry.mediaMrl.constData()));
                if (!media && !entry.mediaId)
                    media.reset(vlc_ml_new_external_media(ml, entry.mediaMrl.constData()));
                if (!media)
                {
                    ctx.error = QObject::tr("The destination media could not be found in the library.");
                    break;
                }
                bool exists = false;
                if (media->p_files)
                    for (size_t i = 0; i < media->p_files->i_nb_items; ++i)
                    {
                        const auto& file = media->p_files->p_items[i];
                        if (file.i_type == fileType && file.psz_mrl && entry.fileMrl == file.psz_mrl)
                            exists = true;
                    }
                if (exists)
                    continue;
                if (vlc_ml_media_add_external_mrl(ml, media->i_id,
                        entry.fileMrl.constData(), fileType) != VLC_SUCCESS)
                {
                    ctx.error = (audio ? QObject::tr("Could not save audio track association for %1.")
                                       : QObject::tr("Could not save subtitle association for %1."))
                                    .arg(QUrl::fromEncoded(entry.mediaMrl).fileName());
                    break;
                }
                ++ctx.saved;
            }
        },
        [done](quint64, Context& ctx) { done(ctx.saved, ctx.error); });
}
