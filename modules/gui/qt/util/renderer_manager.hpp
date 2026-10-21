/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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
#ifndef RENDERER_MANAGER_HPP
#define RENDERER_MANAGER_HPP

#include "qt.hpp"
#include <QAbstractListModel>

class RendererManagerPrivate;
class RendererManager : public QAbstractListModel
{
    Q_OBJECT

public:
    enum RendererStatus
    {
        FAILED = -2,
        IDLE = -1,
        RUNNING,
    };
    Q_ENUM(RendererStatus)

    // how the device does with the media it was given
    enum CastStatus
    {
        CastIdle,
        CastConnecting,
        CastPlaying,
        CastFailed,
    };
    Q_ENUM(CastStatus)

    enum Roles {
        NAME = Qt::UserRole,
        TYPE,
        DEMUX_FILTER,
        SOUT,
        ICON_URI,
        FLAGS,
        SELECTED
    };

    ///remaining time before scan timeout
    Q_PROPERTY(int scanRemain READ getScanRemain NOTIFY scanRemainChanged FINAL)
    Q_PROPERTY(RendererStatus status READ getStatus NOTIFY statusChanged FINAL)
    Q_PROPERTY(bool useRenderer READ useRenderer NOTIFY useRendererChanged FINAL)
    // the device the media plays on, empty on this computer
    Q_PROPERTY(QString rendererName READ rendererName NOTIFY useRendererChanged FINAL)
    // whether it shows the video (a TV), or plays the sound only (a speaker)
    Q_PROPERTY(bool rendererVideo READ rendererVideo NOTIFY useRendererChanged FINAL)
    // the device reads the media from VLC, and Windows Firewall does not let
    // it connect (allowFirewall)
    Q_PROPERTY(bool firewallBlocked READ firewallBlocked NOTIFY firewallChanged FINAL)
    // waiting for Windows to add the rule
    Q_PROPERTY(bool firewallAllowing READ firewallAllowing NOTIFY firewallChanged FINAL)
    // the network is a public one for Windows: the rule is then for the
    // public networks too
    Q_PROPERTY(bool firewallPublic READ firewallPublic NOTIFY firewallChanged FINAL)
    // the picture here too while it plays on the device (without the sound)
    Q_PROPERTY(bool displayHere READ displayHere WRITE setDisplayHere NOTIFY displayHereChanged FINAL)
    // the device playing can have the picture here too (a DLNA TV: its
    // output shows it in step with it; not a Chromecast)
    Q_PROPERTY(bool displayHereAvailable READ displayHereAvailable NOTIFY useRendererChanged FINAL)
    // connecting, playing there, or it failed (castMessage says why: then
    // retry, or play here)
    Q_PROPERTY(CastStatus castStatus READ castStatus NOTIFY castStatusChanged FINAL)
    Q_PROPERTY(QString castMessage READ castMessage NOTIFY castStatusChanged FINAL)

public:
    RendererManager( qt_intf_t* intf, vlc_player_t* player );
    virtual ~RendererManager();

    // back to this computer
    Q_INVOKABLE void disableRenderer();
    // plays on the device of that row
    Q_INVOKABLE void select(int row);
    // asks Windows (administrator approval) to let the devices of the private
    // networks (and of the public ones, on one: firewallPublic) connect to
    // VLC, then casts again
    Q_INVOKABLE void allowFirewall();
    // the device failed: casts to it again, from where the media is
    Q_INVOKABLE void retry();

public slots:
    void StartScan();
    void StopScan();

    //QAbstractListModel override
public:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int,QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //properties accessor
public:
    int getScanRemain() const;
    RendererStatus getStatus() const;
    bool useRenderer() const;
    QString rendererName() const;
    bool rendererVideo() const;
    bool firewallBlocked() const;
    bool firewallAllowing() const;
    bool firewallPublic() const;
    bool displayHere() const;
    void setDisplayHere(bool display);
    bool displayHereAvailable() const;
    CastStatus castStatus() const;
    QString castMessage() const;
signals:
    void statusChanged();
    void scanRemainChanged();
    void useRendererChanged();
    void firewallChanged();
    void displayHereChanged();
    void castStatusChanged();
    // the volume of the device (0 to 1), when it says it as casting starts
    void deviceVolumeReported(float volume);

private:
    Q_DECLARE_PRIVATE(RendererManager);
    QScopedPointer<RendererManagerPrivate> d_ptr;
};

#endif // RENDERER_MANAGER_HPP
