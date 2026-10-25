/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/****************************************************************************
 * config_model.hpp : Staged VLC configuration for QML preferences
 ****************************************************************************/
#ifndef VLC_QT_PREFERENCES_CONFIG_MODEL_HPP
#define VLC_QT_PREFERENCES_CONFIG_MODEL_HPP

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QPointer>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVector>

class MainCtx;
class AudioEffects;
class VideoEffects;
Q_MOC_INCLUDE("maininterface/mainctx.hpp")
Q_MOC_INCLUDE("dialogs/extended/audio_effects.hpp")
Q_MOC_INCLUDE("dialogs/extended/video_effects.hpp")

class ConfigModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(MainCtx *ctx READ ctx WRITE setCtx NOTIFY ctxChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    // While searching, every setting is listed once (no row shown again):
    // the view matches the text against the labels it shows
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QStringList categories READ categories NOTIFY categoriesChanged)
    // Changes with every staged value: for the bindings reading valueFor()
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    enum Role {
        KeyRole = Qt::UserRole + 1, CategoryRole, LabelRole, HelpRole,
        TypeRole, ValueRole, CurrentRole, DefaultRole, MinimumRole,
        MaximumRole, ChoicesRole, DirtyRole,
        // The value differs from the one VLC runs with (the settings that
        // only apply at the next start say so)
        PendingRestartRole
    };
    Q_ENUM(Role)

    explicit ConfigModel(QObject *parent = nullptr);
    MainCtx *ctx() const;
    void setCtx(MainCtx *ctx);
    QString category() const { return m_category; }
    void setCategory(const QString &category);
    QString search() const { return m_search; }
    void setSearch(const QString &search);
    bool dirty() const;
    QString error() const { return m_error; }
    QStringList categories() const;
    int revision() const { return m_revision; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE bool setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE bool setDirectoryUrl(const QString &key, const QUrl &url);
    Q_INVOKABLE QVariant valueFor(const QString &key) const;
    Q_INVOKABLE bool hasOption(const QString &key) const;
    // What VLC uses when the option is empty (the folder of the recordings...)
    Q_INVOKABLE QString defaultText(const QString &key) const;
    // A folder as the design shows it: from the home of the user ("~/Videos")
    Q_INVOKABLE QString displayPath(const QString &path) const;
    // Regenerates the cache of the plugins (vlc-cache-gen) for the next start
    Q_INVOKABLE bool rescanPlugins();
    // The folder of vlcrc in the file manager
    Q_INVOKABLE bool openConfigFolder();
    // Every option of VLC and every setting of the interface to its default
    Q_INVOKABLE void resetAll();
    // The preamp then the 10 bands (dB) of an equalizer preset of VLC
    Q_INVOKABLE QVariantList equalizerPreset(const QString &name) const;
    Q_INVOKABLE bool audioscrobblerAvailable() const;
    Q_INVOKABLE bool audioscrobblerEnabled() const;
    Q_INVOKABLE bool setAudioscrobblerEnabled(bool enabled);
    Q_INVOKABLE QString keyAt(int visibleRow) const;
    // The text of VLC for a row (its name, its help): what the search looks
    // in, besides the labels of the design the view shows
    Q_INVOKABLE QString labelAt(int visibleRow) const;
    Q_INVOKABLE QString helpAt(int visibleRow) const;
    Q_INVOKABLE bool apply();
    Q_INVOKABLE bool applyWithEffects(AudioEffects *audio, VideoEffects *video);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void resetCategory();
    Q_INVOKABLE void reload();
    // reload(), unless the values were read a moment ago (the dialog made
    // just before it opens: read twice, every row told again)
    Q_INVOKABLE void refresh();

signals:
    void ctxChanged();
    void categoryChanged();
    void searchChanged();
    void dirtyChanged();
    void errorChanged();
    void categoriesChanged();
    void revisionChanged();

private:
    struct Entry {
        QString key, category, label, help, type;
        QVariant current, staged, original, minimum, maximum;
        QVariantList choices;
        bool bounded = false;
        // Kept in the settings of the interface (QSettings), not by VLC
        bool qtSetting = false;
        // A toggle showing a state of other options, written through them
        QString proxyFor;
    };

    // Dropdowns of the design standing for several VLC options
    static constexpr const char *repeatKey = "x-repeat";
    static constexpr const char *playbackEndKey = "x-playback-end";
    static constexpr const char *seekPreciseKey = "x-seek-precise";
    static constexpr const char *customSkinKey = "x-custom-skin";
    static constexpr const char *rememberPositionKey = "x-remember-position";
    static constexpr const char *wheelReverseKey = "x-wheel-reverse";
    static constexpr const char *webRemoteKey = "x-web-remote";
    // "x-filter:<module>": the module is in the audio filters
    static constexpr const char *filterPrefix = "x-filter:";
    static constexpr const char *nightModeKey = "x-night-mode";
    static constexpr const char *subShadowKey = "x-sub-shadow";
    static constexpr const char *subBackgroundKey = "x-sub-background";
    // The opacity of the subtitle background, kept while there is none
    static constexpr const char *subBackgroundOpacityKey = "redesign/subtitle-background-opacity";
    static constexpr const char *fetchMetadataKey = "x-fetch-metadata";
    static constexpr const char *bufferingKey = "x-buffering";
    // "x-mirror:<key>": the row of another entry shown again (the scroll
    // rows of the Remotes of the design)
    static constexpr const char *mirrorPrefix = "x-mirror:";
    // Visualization size (effect-width / effect-height)
    static constexpr const char *visualSizeKey = "x-visual-size";
    // The visualizer (audio-visual, and effect-list for the effects of
    // the "visual" module: spectrometer, scope, spectrum, waveform, vuMeter)
    static constexpr const char *visualizerKey = "x-visualizer";
    // The compressor as it was before Night mode, put back when it is off
    static constexpr const char *nightPreviousKey = "redesign/night-mode-previous";
    // Video post-processing: the level of the postproc filter, 0 when it
    // does not run
    static constexpr const char *postprocKey = "x-postproc";
    // Match window (the size of VLC, the window scales it), 1280 x 720,
    // 1920 x 1080; then the size of the user
    static constexpr qint64 visualSizes[3][2] = { { 800, 500 }, { 1280, 720 }, { 1920, 1080 } };
    bool setFilter(const QString &module, bool on);
    // Toggles standing for a state of other options (Entry::proxyFor)
    int indexOfKey(const QString &key) const;
    void syncDerived();
    bool setDerived(int index, const QVariant &value);
    void addCombinedEntries();
    static bool combinedValues(const Entry &entry, QHash<QString, QVariant> *values,
                               QHash<QString, QVariant> *expected);

    bool visible(const Entry &entry) const;
    void rebuildRows();
    // m_entries from VLC and the settings of the interface (reload())
    void buildEntries();
    // The texts of the rows in the language of the interface
    void retranslate();
    void setError(const QString &error);
    // A setting as the user knows it (its name), for the messages
    QString displayName(const QString &key) const;
    // The value VLC runs with: the one it started with for a setting
    // applied since
    QVariant runningValue(const Entry &entry) const;
    // The values the settings applied since the start had then (by key)
    static QHash<QString, QVariant> &startValues();
    // Repeat, Shuffle, Stop after each item, When playback finishes: on the
    // play queue now (they are its state as well as its defaults)
    void applyOrder();
    void notifyEntry(int entryIndex);
    static bool normalize(const Entry &entry, const QVariant &value, QVariant *normalized);
    static QVariant readValue(const Entry &entry);

    QPointer<MainCtx> m_ctx;
    QString m_category, m_search, m_error;
    QVector<Entry> m_entries;
    QVector<int> m_rows;
    int m_revision = 0;
    QElapsedTimer m_loaded;
};

#endif
