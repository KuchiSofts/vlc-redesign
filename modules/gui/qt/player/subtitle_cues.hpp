/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/*****************************************************************************
 * subtitle_cues.hpp: The lines of the subtitle shown, drawn on the seek bar
 *****************************************************************************/
#ifndef VLC_QT_SUBTITLE_CUES_HPP_
#define VLC_QT_SUBTITLE_CUES_HPP_

#include "qt.hpp"
#include "player/player_controller.hpp"
#include "player/subtitle_cues_parser.hpp"

#include <QColor>
#include <QQuickPaintedItem>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <vector>

// Where the lines of the subtitle track shown are, as marks on the seek
// bar: read from its file (a file added, found next to the video, or found
// online), on a worker thread, once per file (kept for the next times).
// The tracks of the media itself are not drawn: VLC reads their lines as
// they play, only reading the whole file would give them all.
// The marks are painted once into a texture, again only when the file,
// the width, the color, the length or the subtitle delay change.
class SubtitleCues : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(PlayerController* player READ player WRITE setPlayer NOTIFY playerChanged FINAL)
    // reads and draws (the setting on, the bar shown)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    // the narrowest mark, and the gap under which two marks are one
    Q_PROPERTY(qreal minimumWidth READ minimumWidth WRITE setMinimumWidth NOTIFY layoutChanged FINAL)
    Q_PROPERTY(qreal mergeDistance READ mergeDistance WRITE setMergeDistance NOTIFY layoutChanged FINAL)
    // lines to draw: a subtitle file is shown and was read
    Q_PROPERTY(bool ready READ ready NOTIFY cuesChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY cuesChanged FINAL)

public:
    explicit SubtitleCues(QQuickItem *parent = nullptr);
    ~SubtitleCues() override;

    PlayerController *player() const { return m_player; }
    void setPlayer(PlayerController *player);
    bool active() const { return m_active; }
    void setActive(bool active);
    QColor color() const { return m_color; }
    void setColor(const QColor &color);
    qreal minimumWidth() const { return m_minimumWidth; }
    void setMinimumWidth(qreal width);
    qreal mergeDistance() const { return m_mergeDistance; }
    void setMergeDistance(qreal distance);
    bool ready() const { return m_cues && !m_cues->empty(); }
    int count() const { return m_cues ? int(m_cues->size()) : 0; }

    // The words shown at this position of the bar (0 to 1), empty between
    // the lines
    Q_INVOKABLE QString textAt(qreal position) const;

    void paint(QPainter *painter) override;

signals:
    void playerChanged();
    void activeChanged();
    void colorChanged();
    void layoutChanged();
    void cuesChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    using CueList = std::vector<subtitlecues::Cue>;

    void scheduleResolve();
    void resolve();
    void load(const QString &path, int streamIndex);
    void setCues(std::shared_ptr<const CueList> cues);
    void readTimes();
    static void startReading(const QString &key, const QString &path, int streamIndex, double fps);
    static void finishReading(const QString &key, const std::shared_ptr<const CueList> &cues);
    static void waitAll();

    PlayerController *m_player = nullptr;
    bool m_active = false;
    QColor m_color = QColor(0x6B, 0x6B, 0x6B);
    qreal m_minimumWidth = 1;
    qreal m_mergeDistance = 1.5;
    std::shared_ptr<const CueList> m_cues;
    // the file read for this bar (its result shared by the bars of the
    // same file)
    QString m_key;
    // the track events coalesced; again a moment later while the file of
    // a track just added is not known yet
    QTimer m_resolveTimer;
    int m_retries = 0;
    qint64 m_lengthMs = 0;
    qint64 m_delayMs = 0;
};

#endif
