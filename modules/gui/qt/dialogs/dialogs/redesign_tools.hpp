/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifndef REDESIGN_TOOLS_HPP
#define REDESIGN_TOOLS_HPP

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <QAbstractListModel>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <atomic>
#include <memory>

class MainCtx;
Q_MOC_INCLUDE("maininterface/mainctx.hpp")
class PlayerController;
struct input_stats_t;

/**
 * The data of the small tool windows redrawn in QML (VLC.Dialogs):
 * Statistics, Plugins and extensions, Folder information
 */

// The statistics of what plays (RedesignStatisticsDialog.qml), as the
// Statistics tab of Media Information; collected only with the "stats"
// option on
class MediaStatistics : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MainCtx* ctx READ ctx WRITE setCtx NOTIFY ctxChanged FINAL)
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    // the input bitrate (kb/s), the last ones (one a second)
    Q_PROPERTY(QVariantList bitrates READ bitrates NOTIFY valuesChanged FINAL)
    Q_PROPERTY(double bitrateMax READ bitrateMax NOTIFY valuesChanged FINAL)
    Q_PROPERTY(bool collecting READ collecting NOTIFY collectingChanged FINAL)

public:
    explicit MediaStatistics(QObject *parent = nullptr);

    MainCtx *ctx() const { return m_ctx; }
    void setCtx(MainCtx *ctx);
    QVariantMap values() const { return m_values; }
    QVariantList bitrates() const;
    double bitrateMax() const;
    bool collecting() const { return m_collecting; }

    // turns the collection on (the "stats" option, kept), for the next
    // file played
    Q_INVOKABLE void startCollecting();
    Q_INVOKABLE void clear();

signals:
    void ctxChanged();
    void valuesChanged();
    void collectingChanged();

private:
    void update(const input_stats_t &stats);

    QPointer<MainCtx> m_ctx;
    QVariantMap m_values;
    QVector<double> m_bitrates;
    bool m_collecting = false;
    QMetaObject::Connection m_connection;
};

// The modules of VLC (RedesignPluginsDialog.qml), as the Plugins tab
class PluginListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchPattern READ searchPattern WRITE setSearchPattern NOTIFY searchPatternChanged FINAL)
    // "name", "capability" or "score"
    Q_PROPERTY(QString sortCriteria READ sortCriteria WRITE setSortCriteria NOTIFY sortCriteriaChanged FINAL)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
    Q_PROPERTY(int total READ total CONSTANT FINAL)

public:
    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        CapabilityRole,
        ScoreRole,
        ShortNameRole,
        HelpRole,
    };
    Q_ENUM(Roles)

    explicit PluginListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString searchPattern() const { return m_pattern; }
    void setSearchPattern(const QString &pattern);
    QString sortCriteria() const { return m_sort; }
    void setSortCriteria(const QString &criteria);
    int total() const { return static_cast<int>(m_all.size()); }

signals:
    void searchPatternChanged();
    void sortCriteriaChanged();
    void countChanged();

private:
    struct Plugin
    {
        QString name, capability, shortName, help;
        int score = 0;
    };
    void refilter();

    QVector<Plugin> m_all;
    QVector<int> m_shown;
    QString m_pattern;
    QString m_sort = QStringLiteral("name");
};

// The Lua extensions (RedesignPluginsDialog.qml), as the Extensions tab
class ExtensionInfoModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(MainCtx* ctx READ ctx WRITE setCtx NOTIFY ctxChanged FINAL)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    enum Roles
    {
        TitleRole = Qt::UserRole + 1,
        SummaryRole,
        DescriptionRole,
        VersionRole,
        AuthorRole,
        UrlRole,
        FileNameRole,
    };
    Q_ENUM(Roles)

    explicit ExtensionInfoModel(QObject *parent = nullptr);

    MainCtx *ctx() const { return m_ctx; }
    void setCtx(MainCtx *ctx);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // reads the scripts again (as Reload extensions)
    Q_INVOKABLE void reload();

signals:
    void ctxChanged();
    void countChanged();

private:
    struct Extension
    {
        QString title, summary, description, version, author, url, fileName;
    };
    void updateList();

    QPointer<MainCtx> m_ctx;
    QVector<Extension> m_extensions;
};

// What a folder holds (RedesignFolderInfoDialog.qml): counted on a thread
class FolderInformation : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY urlChanged FINAL)
    Q_PROPERTY(QString path READ path NOTIFY urlChanged FINAL)
    Q_PROPERTY(bool exists READ exists NOTIFY urlChanged FINAL)
    Q_PROPERTY(QString modified READ modified NOTIFY urlChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY countsChanged FINAL)
    // stopped at its limit (a whole drive): the counts are "at least"
    Q_PROPERTY(bool partial READ partial NOTIFY countsChanged FINAL)
    Q_PROPERTY(int files READ files NOTIFY countsChanged FINAL)
    Q_PROPERTY(int folders READ folders NOTIFY countsChanged FINAL)
    Q_PROPERTY(int videos READ videos NOTIFY countsChanged FINAL)
    Q_PROPERTY(int audios READ audios NOTIFY countsChanged FINAL)
    Q_PROPERTY(int subtitles READ subtitles NOTIFY countsChanged FINAL)
    Q_PROPERTY(QString size READ size NOTIFY countsChanged FINAL)

public:
    explicit FolderInformation(QObject *parent = nullptr);
    ~FolderInformation() override;

    QUrl url() const { return m_url; }
    void setUrl(const QUrl &url);
    QString name() const;
    QString path() const;
    bool exists() const { return m_exists; }
    QString modified() const { return m_modified; }
    bool busy() const { return m_busy; }
    bool partial() const { return m_counts.partial; }
    int files() const { return m_counts.files; }
    int folders() const { return m_counts.folders; }
    int videos() const { return m_counts.videos; }
    int audios() const { return m_counts.audios; }
    int subtitles() const { return m_counts.subtitles; }
    QString size() const;

    struct Counts
    {
        int files = 0, folders = 0, videos = 0, audios = 0, subtitles = 0;
        qint64 bytes = 0;
        bool partial = false;
    };

signals:
    void urlChanged();
    void countsChanged();

private:
    QUrl m_url;
    bool m_exists = false;
    QString m_modified;
    bool m_busy = false;
    Counts m_counts;
    std::shared_ptr<std::atomic_bool> m_cancel;
};

#endif // REDESIGN_TOOLS_HPP
