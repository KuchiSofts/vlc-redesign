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
#include "controlbar_profile_model.hpp"

#include <QSettings>
#include <QChar>
#include <QCoreApplication>

#include <algorithm>

#include "qt.hpp"
#include "controlbar_profile.hpp"
#include "player/control_list_model.hpp"
#include "player/player_controlbar_model.hpp"
#include "profile_settings.hpp"
#include "maininterface/mainctx.hpp"
#include "dialogs/dialogs_provider.hpp"

ControlbarProfileModel::ControlbarProfileModel(QSettings* settings, QObject *parent)
    : QAbstractListModel(parent),
    m_settings(settings)
{
    assert(m_settings);

    connect(this, &QAbstractListModel::rowsInserted, this, &ControlbarProfileModel::countChanged);
    connect(this, &QAbstractListModel::rowsRemoved, this, &ControlbarProfileModel::countChanged);
    connect(this, &QAbstractListModel::modelReset, this, &ControlbarProfileModel::countChanged);

    // To make the QML player controlbars update when model is Reset
    connect(this, &QAbstractListModel::modelReset, this, &ControlbarProfileModel::selectedProfileChanged);
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
        // Customize Player still open: its changes are dropped as by its
        // Cancel (it saved the profiles when it opened)
        if (m_editing)
            reload();
        else
            save();
    });

    // When all profiles are removed, insert defaults:
    // Maybe add a dedicate button for this purpose and don't allow removing all profiles ?
    connect(this, &ControlbarProfileModel::countChanged, this, [this] () {
        if (rowCount() == 0)
            insertDefaults();
    });

    // defer initialization reload:
    QMetaObject::invokeMethod(this, [this] () {
        // reload() fails only when no saved profile could be read: saved
        // profiles are never replaced by the defaults
        if (reload() == false)
            insertDefaults();
    }, Qt::QueuedConnection);
}

void ControlbarProfileModel::insertDefaults()
{
    // The profiles of the design (Figma 6648:20666), for a fresh start
    static const Style freshStyles[] = { Style::DEFAULT_STYLE, Style::MOVIES_STYLE,
                                         Style::SHOWS_STYLE, Style::MUSIC_STYLE,
                                         Style::WORK_STYLE };
    for (const Style style : freshStyles)
    {
        const auto it = std::find_if(m_defaults.cbegin(), m_defaults.cend(),
                                     [style](const Profile& i) { return i.id == style; });
        if (it == m_defaults.cend())
            continue;

        const auto ptrNewProfile = newProfile(qfut(it->name));
        if (!ptrNewProfile)
            continue;

        ptrNewProfile->injectModel(it->modelData);
        ptrNewProfile->resetDirty(); // default profiles should not be dirty initially
        // No settings of their own yet: taken now (before the first-run
        // setup) they would bring the theme, the accent, the preferences and
        // the shortcuts of a fresh install back when the profile is chosen.
        // A default profile keeps the settings in use when it is first
        // chosen, and has its own from the first time it is left (or saved).
        ptrNewProfile->m_settingsSnapshot.clear();
    }

    m_selectedProfile = -1;
    setSelectedProfile(0);
}

QString ControlbarProfileModel::generateUniqueName(const QString &name) const
{
    static const auto targetNameGenerator = [](const auto& name, long count) {
        return QString("%1 (%2)").arg(name).arg(count);
    };

    // Actually, the profile model inherently does not allow two
    // profiles to have the same name so it could be sufficient
    // to check only for name existence but for cases when the
    // config file is edited explicitly, using count_if might
    // be helpful.
    const auto sameNameCount = [this](const auto& name) {
        return std::count_if(m_profiles.begin(),
                             m_profiles.end(),
                             [name](const ControlbarProfile* i) {
                                 return i->name() == name;
                                 });
        };

    auto count = sameNameCount(name);

    if (count > 0)
    {
        // suppose the existing profiles with these names:
        // Profile, Profile (1), Profile (2)
        // when the user adds a new profile with name 'Profile',
        // its name will be replaced with 'Profile (3)'.
        // However, the same will not happen if the user adds a
        // new profile with name 'Profile (1)' or 'Profile (2)'.
        // In that case, the new names will be 'Profile (1) (1)',
        // and 'Profile (2) (1)', respectively. This behavior is
        // intended because otherwise profile name assignment
        // would be restrictive.

        auto targetName = targetNameGenerator(name, count);

        // if targetName also exists, increase the count by one
        // and try again:
        while (sameNameCount(targetName) >= 1)
        {
            ++count;
            targetName = targetNameGenerator(name, count);
        }

        return targetName;
    }
    else
        return name;
}

int ControlbarProfileModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_profiles.size();
}

QVariant ControlbarProfileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const auto ptrProfile = m_profiles.at(index.row());

    if (!ptrProfile)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        return ptrProfile->name();
    case MODEL_ROLE:
        return QVariant::fromValue(ptrProfile);
    }

    return QVariant();
}

QHash<int, QByteArray> ControlbarProfileModel::roleNames() const
{
    return {
        {
            Qt::DisplayRole, QByteArrayLiteral("name")
        },
        {
            MODEL_ROLE, QByteArrayLiteral("model")
        }
    };
}

bool ControlbarProfileModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || row > m_profiles.size())
        return false;

    beginInsertRows(parent, row, row + count - 1);

    for (int i = 0; i < count; ++i)
    {
        const auto profile = generateProfileFromStyle(Style::EMPTY_STYLE);
        assert(profile);
        profile->setParent(this);
        profile->setName(qtr("Profile %1").arg(m_profiles.size()));

        m_profiles.insert(row, profile);
    }

    endInsertRows();

    return true;
}

bool ControlbarProfileModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || count < 1 || row + count > m_profiles.size())
        return false;

    beginRemoveRows(parent, row, row + count - 1);

    auto from = m_profiles.begin() + row;
    auto to = from + count;
    std::for_each(from, to, [](auto* item) {
        assert(item);
        item->deleteLater();
    });
    m_profiles.erase(from, to);

    endRemoveRows();

    return true;
}

bool ControlbarProfileModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value)
    {
        auto ptrProfile = m_profiles.at(index.row());

        if (!ptrProfile)
            return false;

        switch (role)
        {
        case Qt::DisplayRole:
            if (value.canConvert<QString>())
                ptrProfile->setName(value.toString());
            else
                return false;
            break;
        case MODEL_ROLE:
            if (value.canConvert<ControlbarProfile*>())
                ptrProfile = qvariant_cast<ControlbarProfile*>(value);
            else
                return false;
            break;
        default:
            return false;
        }

        m_profiles.replace(index.row(), ptrProfile);

        emit dataChanged(index, index, { role });
        return true;
    }
    return false;
}

Qt::ItemFlags ControlbarProfileModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return (Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
}

int ControlbarProfileModel::selectedProfile() const
{
    return m_selectedProfile;
}

QString ControlbarProfileModel::profileAccent() const
{
    return m_settings->value(QStringLiteral("redesign/accent"),
                             QStringLiteral("#D37918")).toString();
}

ControlbarProfile* ControlbarProfileModel::currentModel() const
{
    return getProfile(selectedProfile());
}

bool ControlbarProfileModel::setSelectedProfile(int selectedProfile)
{
    if (m_selectedProfile == selectedProfile)
        return false;

    const auto ptrProfileNew = getProfile(selectedProfile);
    const auto ptrProfileOld = getProfile(m_selectedProfile);

    assert(ptrProfileNew);

    if (!ptrProfileNew)
        return false;

    auto *ctx = qobject_cast<MainCtx *>(parent());
    if (!m_skipNextApply && ptrProfileOld && ctx)
        ptrProfileOld->m_settingsSnapshot = ProfileSettings::capture(ctx, m_settings);
    // an empty snapshot (a default profile never left yet): the settings in
    // use stay
    if (!m_skipNextApply && ctx && !ptrProfileNew->m_settingsSnapshot.isEmpty()) {
        QString error;
        if (!ProfileSettings::apply(ctx, m_settings,
                                    ptrProfileNew->m_settingsSnapshot, &error)) {
            setError(error);
            return false;
        }
    }
    setError({});

    connect(ptrProfileNew, &ControlbarProfile::controlListChanged, this, &ControlbarProfileModel::selectedProfileControlListChanged);
    connect(this, &QAbstractListModel::modelReset, ptrProfileNew, &ControlbarProfile::generateLinearControlList);
    connect(this, &ControlbarProfileModel::selectedProfileChanged, ptrProfileNew, &ControlbarProfile::generateLinearControlList);

    if (ptrProfileOld && (ptrProfileNew != ptrProfileOld))
    {
        disconnect(ptrProfileOld, &ControlbarProfile::controlListChanged, this, &ControlbarProfileModel::selectedProfileControlListChanged);
        disconnect(this, &QAbstractListModel::modelReset, ptrProfileOld, &ControlbarProfile::generateLinearControlList);
        disconnect(this, &ControlbarProfileModel::selectedProfileChanged, ptrProfileOld, &ControlbarProfile::generateLinearControlList);
    }

    m_selectedProfile = selectedProfile;
    setModified(true);

    emit selectedProfileChanged();

    return true;
}

void ControlbarProfileModel::forceSelectedProfile(int index)
{
    const QString error = m_lastError;
    m_skipNextApply = true;
    setSelectedProfile(index);
    m_skipNextApply = false;
    setError(error);
}

void ControlbarProfileModel::setError(const QString &error)
{
    if (m_lastError == error)
        return;
    m_lastError = error;
    emit errorChanged();
}

void ControlbarProfileModel::setEditing(bool editing)
{
    if (m_editing == editing)
        return;
    m_editing = editing;
    emit editingChanged();
}

void ControlbarProfileModel::setModified(bool modified)
{
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit modifiedChanged();
}

std::optional<int> ControlbarProfileModel::findModel(const ControlbarProfile *profile) const
{
    assert(profile);
    for (int i = 0; i < m_profiles.count(); ++i)
    {
        if (m_profiles[i] == profile)
            return i;
        else
        {
            assert(m_profiles[i]);
            if (*m_profiles[i] == *profile)
                return i;
        }
    }
    return std::nullopt;
}

void ControlbarProfileModel::insertProfile(std::unique_ptr<ControlbarProfile> profile, bool select)
{
    assert(profile);
    const auto ptrProfile = profile.release();

    ptrProfile->setParent(this);

    beginInsertRows(QModelIndex(), m_profiles.size(), m_profiles.size());
    m_profiles.append(ptrProfile);
    endInsertRows();

    if (select)
        setSelectedProfile(m_profiles.count() - 1);
}

ControlbarProfile *ControlbarProfileModel::getProfile(int index) const
{
    if (index < 0 || index >= m_profiles.size())
        return nullptr;

    return m_profiles.at(index);
}

ControlbarProfile *ControlbarProfileModel::newProfile(const QString &name)
{
    if (name.isEmpty())
        return nullptr;

    const auto ptrNewProfile = generateProfileFromStyle(Style::EMPTY_STYLE);
    assert(ptrNewProfile);
    ptrNewProfile->setParent(this);
    // named before it is listed: the menus read the name when the row comes
    ptrNewProfile->setName(generateUniqueName(name));
    ptrNewProfile->m_settingsSnapshot = ProfileSettings::capture(
        qobject_cast<MainCtx *>(parent()), m_settings);

    beginInsertRows(QModelIndex(), m_profiles.size(), m_profiles.size());

    m_profiles.append(ptrNewProfile);

    endInsertRows();

    return ptrNewProfile;
}

ControlbarProfile *ControlbarProfileModel::newProfile()
{
    return newProfile(qtr("Profile %1").arg(m_profiles.size() + 1));
}

ControlbarProfile *ControlbarProfileModel::cloneProfile(const ControlbarProfile *profile,
                                                        const QString &name)
{
    const auto ptrNewProfile = newProfile(name.isEmpty() ? profile->name() : name);

    if (!ptrNewProfile)
        return nullptr;

    ptrNewProfile->m_settingsSnapshot = profile->m_settingsSnapshot;
    if (profile == currentModel())
        ptrNewProfile->m_settingsSnapshot = ProfileSettings::capture(
            qobject_cast<MainCtx *>(parent()), m_settings);

    for (auto it = profile->m_models.constBegin(); it != profile->m_models.constEnd(); ++it)
    {
        ptrNewProfile->setModelData(it.key(), profile->getModelData(it.key()));
        ptrNewProfile->resetDirty();
    }

    return ptrNewProfile;
}

void ControlbarProfileModel::requestNewProfile()
{
    m_newProfileRequested = true;
    // Customize Player asks for the name when it is open, else when it opens
    emit newProfileRequested();
    emit DialogsProvider::getInstance()->showToolbarEditorDialog();
}

bool ControlbarProfileModel::takeNewProfileRequest()
{
    const bool requested = m_newProfileRequested;
    m_newProfileRequested = false;
    return requested;
}

QString ControlbarProfileModel::nameError(const QString &name, bool renaming) const
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        return qtr("Enter a name for the profile.");
    const ControlbarProfile *self = renaming ? currentModel() : nullptr;
    for (const ControlbarProfile *profile : m_profiles)
        if (profile != self && profile->name() == trimmed)
            return qtr("A profile named \"%1\" already exists.").arg(trimmed);
    return {};
}

QString ControlbarProfileModel::uniqueName(const QString &name) const
{
    return generateUniqueName(name.trimmed());
}

int ControlbarProfileModel::createProfile(const QString &name)
{
    const QString error = nameError(name, false);
    if (!error.isEmpty())
    {
        setError(error);
        return -1;
    }

    const auto ptrProfile = newProfile(name.trimmed());
    if (!ptrProfile)
        return -1;

    // the layout of the Default profile, arranged from there
    const auto it = std::find_if(m_defaults.cbegin(), m_defaults.cend(),
                                 [](const Profile& i) { return i.id == Style::DEFAULT_STYLE; });
    if (it != m_defaults.cend())
        ptrProfile->injectModel(it->modelData);
    ptrProfile->resetDirty();

    const int row = m_profiles.size() - 1;
    setModified(true);
    if (!setSelectedProfile(row))
        forceSelectedProfile(row);
    return row;
}

int ControlbarProfileModel::cloneSelectedProfile(const QString &name)
{
    const auto ptrModel = currentModel();
    if (!ptrModel)
    {
        setError(qtr("No profile is selected."));
        return -1;
    }

    const QString error = nameError(name, false);
    if (!error.isEmpty())
    {
        setError(error);
        return -1;
    }

    if (!cloneProfile(ptrModel, name.trimmed()))
        return -1;

    const int row = m_profiles.size() - 1;
    setModified(true);
    if (!setSelectedProfile(row))
        forceSelectedProfile(row);
    return row;
}

bool ControlbarProfileModel::renameSelectedProfile(const QString &name)
{
    const auto ptrProfile = currentModel();
    if (!ptrProfile)
    {
        setError(qtr("No profile is selected."));
        return false;
    }

    const QString trimmed = name.trimmed();
    if (trimmed == ptrProfile->name())
    {
        setError({});
        return true;
    }

    const QString error = nameError(trimmed, true);
    if (!error.isEmpty())
    {
        setError(error);
        return false;
    }

    ptrProfile->setName(trimmed);
    const QModelIndex row = index(m_selectedProfile);
    emit dataChanged(row, row, { Qt::DisplayRole });
    setModified(true);
    setError({});
    return true;
}

bool ControlbarProfileModel::deleteSelectedProfile()
{
    const auto ptrSelectedProfile = getProfile(m_selectedProfile);

    if (!ptrSelectedProfile)
    {
        setError(qtr("No profile is selected."));
        return false;
    }
    if (m_profiles.size() <= 1)
    {
        setError(qtr("The last profile cannot be deleted."));
        return false;
    }

    const auto _selectedProfile = m_selectedProfile;

    disconnect(ptrSelectedProfile, nullptr, this, nullptr);
    disconnect(this, nullptr, ptrSelectedProfile, nullptr);

    beginRemoveRows(QModelIndex(), _selectedProfile, _selectedProfile);

    // its settings go with it: the profile selected next brings its own
    m_selectedProfile = -1;
    m_profiles.removeAt(_selectedProfile);

    endRemoveRows();

    // the views may hold it until the next profile is selected
    ptrSelectedProfile->deleteLater();

    const int next = getProfile(_selectedProfile - 1) ? _selectedProfile - 1 : _selectedProfile;
    if (!setSelectedProfile(next))
        forceSelectedProfile(next);
    setModified(true);
    return true;
}

ControlbarProfile *ControlbarProfileModel::generateProfileFromStyle(const Style style)
{
    for (const auto& i : m_defaults)
    {
        if (i.id == style)
        {
            const auto ptrNewProfile = new ControlbarProfile();
            ptrNewProfile->injectModel(i.modelData);
            ptrNewProfile->setName(i.name);
            ptrNewProfile->resetDirty(); // default profiles should not be dirty initially
            return ptrNewProfile;
        }
    }

    // Style does not exist
    return nullptr;
}
