/*****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifndef CONTROLBARPROFILEMODEL_H
#define CONTROLBARPROFILEMODEL_H

#include <QAbstractListModel>
#include <QSettings>
#include <array>

#include "controlbar_profile.hpp"


class ControlbarProfileModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int selectedProfile READ selectedProfile WRITE setSelectedProfile NOTIFY selectedProfileChanged FINAL)
    Q_PROPERTY(ControlbarProfile* currentModel READ currentModel NOTIFY selectedProfileChanged FINAL)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged FINAL)
    Q_PROPERTY(QString profileAccent READ profileAccent NOTIFY selectedProfileChanged FINAL)
    // Profiles added, renamed, deleted or chosen since they were last saved
    // or reloaded (Save of Customize Player)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged FINAL)
    // Set by Customize Player while it is open: its changes are not saved
    // until its Save, and are dropped when VLC quits before that
    Q_PROPERTY(bool editing READ editing WRITE setEditing NOTIFY editingChanged FINAL)

public:
    explicit ControlbarProfileModel(QSettings* settings, QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;

    enum class Style
    {
        EMPTY_STYLE,
        DEFAULT_STYLE,
        MINIMALIST_STYLE,
        ONE_LINER_STYLE,
        SIMPLEST_STYLE,
        CLASSIC_STYLE,
        // the profiles of the design (Figma 6648:20666)
        MOVIES_STYLE,
        SHOWS_STYLE,
        MUSIC_STYLE,
        WORK_STYLE
    };

    // Editable:
    Q_INVOKABLE bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::DisplayRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Add data:
    Q_INVOKABLE bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

public:
    enum Roles {
        MODEL_ROLE = Qt::UserRole,
    };

    int selectedProfile() const;
    QString lastError() const { return m_lastError; }
    QString profileAccent() const;
    ControlbarProfile* currentModel() const;
    bool modified() const { return m_modified; }
    bool editing() const { return m_editing; }
    void setEditing(bool editing);

    // A copy of the profile (named after it when name is empty)
    ControlbarProfile* cloneProfile(const ControlbarProfile* profile, const QString& name = {});

    Q_INVOKABLE ControlbarProfile* getProfile(int index) const;

    ControlbarProfile* newProfile(const QString& name);
    ControlbarProfile* newProfile();

    // "New profile" of every menu: opens Customize Player, which asks for
    // the name of the profile (takeNewProfileRequest())
    Q_INVOKABLE void requestNewProfile();
    // Whether a new profile was asked for since the last call
    Q_INVOKABLE bool takeNewProfileRequest();

    // Why a name can not be given to a new profile (renaming: to the
    // selected one), empty when it can
    Q_INVOKABLE QString nameError(const QString& name, bool renaming) const;
    // The name, or the name followed by a number when a profile has it
    Q_INVOKABLE QString uniqueName(const QString& name) const;

    // These select the profile and return its row, -1 when they fail
    // (lastError says why): a new profile has the layout of the Default
    // one and the settings in use, a clone those of the selected profile
    Q_INVOKABLE int createProfile(const QString& name);
    Q_INVOKABLE int cloneSelectedProfile(const QString& name);
    Q_INVOKABLE bool renameSelectedProfile(const QString& name);
    // The last profile is never deleted
    Q_INVOKABLE bool deleteSelectedProfile();

    // This is a static method, caller takes the ownership
    // of the profile:
    static ControlbarProfile* generateProfileFromStyle(const Style style);

    // ControlbarProfileModel takes the ownership:
    void insertProfile(std::unique_ptr<ControlbarProfile> profile, bool select);

public slots:
    bool save(bool clearDirty = true);
    bool reload();

    bool setSelectedProfile(int selectedProfile);

    std::optional<int> findModel(const ControlbarProfile* profile) const;

signals:
    void countChanged();
    void selectedProfileChanged();
    void errorChanged();
    void modifiedChanged();
    void editingChanged();
    void newProfileRequested();

    void selectedProfileControlListChanged(const QVector<int>& linearControlList);

private:
    QVector<ControlbarProfile *> m_profiles;

    int m_selectedProfile = -1;
    QString m_lastError;
    bool m_skipNextApply = false;
    bool m_modified = false;
    bool m_editing = false;
    bool m_newProfileRequested = false;

    struct Profile {
        const Style id;
        const char* name;
        QVector<ControlbarProfile::Configuration> modelData;
    };

    static const QVector<Profile> m_defaults;

private:
    void insertDefaults();

    QString generateUniqueName(const QString& name) const;
    void setError(const QString& error);
    void setModified(bool modified);
    // Selects a profile even when its settings can not be applied (it
    // stays selected with the settings in use, lastError says why)
    void forceSelectedProfile(int index);

protected:
    QSettings* m_settings = nullptr;
};

#endif // CONTROLBARPROFILEMODEL_H
