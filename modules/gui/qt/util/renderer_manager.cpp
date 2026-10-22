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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "renderer_manager.hpp"

#include <QApplication>
#include <QRegularExpression>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <QHash>

#include <memory>

#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_renderer_discovery.h>
#include <vlc_player.h>
#include <vlc_cxx_helpers.hpp>

#ifdef _WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
# include <iphlpapi.h>
# include <windows.h>
# include <ole2.h>
# include <netfw.h>
# include <shellapi.h>
# include <sstream>
# include <string>
# include <vector>
# include <vlc_network.h>
#endif

static void player_renderer_changed(vlc_player_t *, vlc_renderer_item_t *new_item, void *data);

#ifdef _WIN32
/* Whether @ip is one of this computer's addresses: a device there (a test
 * device, a renderer running here) reads the server without the firewall */
static bool isThisComputer(const QString &ip)
{
    if (ip.isEmpty())
        return false;
    if (ip.startsWith(QLatin1String("127.")) || ip == QLatin1String("::1"))
        return true;
    ULONG size = 16 * 1024;
    std::vector<unsigned char> buffer(size);
    const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG ret = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr,
                                     reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data()), &size);
    if (ret == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(size);
        ret = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr,
                                   reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data()), &size);
    }
    if (ret != NO_ERROR)
        return false;
    for (auto *adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data()); adapter;
         adapter = adapter->Next)
        for (auto *unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
        {
            char host[NI_MAXNUMERICHOST];
            if (vlc_getnameinfo(unicast->Address.lpSockaddr, unicast->Address.iSockaddrLength,
                                host, sizeof(host), nullptr, NI_NUMERICHOST) == 0
             && ip == QLatin1String(host))
                return true;
        }
    return false;
}

/* Whether a list of firewall ports ("80,7000-7100") has this one */
static bool portListHas(const wchar_t *list, long port)
{
    std::wstringstream tokens(list);
    std::wstring token;
    while (std::getline(tokens, token, L','))
    {
        long first = 0, last = 0;
        const int count = swscanf(token.c_str(), L"%ld-%ld", &first, &last);
        if (count == 1)
            last = first;
        else if (count != 2)
            continue;
        if (port >= first && port <= last)
            return true;
    }
    return false;
}

/* Whether an enabled rule lets the network in (or keeps it out: wanted
 * NET_FW_ACTION_BLOCK), on one of these profiles, to this program (on any of
 * its ports or that one), or to that TCP port whatever the program */
static bool ruleMatches(INetFwRule *rule, const wchar_t *path, long port, long profiles,
                        NET_FW_ACTION wanted)
{
    VARIANT_BOOL enabled = VARIANT_FALSE;
    NET_FW_RULE_DIRECTION direction = NET_FW_RULE_DIR_OUT;
    NET_FW_ACTION action = wanted == NET_FW_ACTION_ALLOW ? NET_FW_ACTION_BLOCK : NET_FW_ACTION_ALLOW;
    LONG ruleProfiles = 0, protocol = 0;
    if (FAILED(rule->get_Enabled(&enabled)) || enabled == VARIANT_FALSE
     || FAILED(rule->get_Direction(&direction)) || direction != NET_FW_RULE_DIR_IN
     || FAILED(rule->get_Action(&action)) || action != wanted
     || FAILED(rule->get_Profiles(&ruleProfiles)) || !(ruleProfiles & profiles)
     || FAILED(rule->get_Protocol(&protocol))
     // (256: NET_FW_IP_PROTOCOL_ANY, missing from some SDKs)
     || (protocol != NET_FW_IP_PROTOCOL_TCP && protocol != 256))
        return false;

    BSTR application = nullptr, ports = nullptr;
    rule->get_ApplicationName(&application);
    rule->get_LocalPorts(&ports);
    const bool anyPort = ports == nullptr || *ports == L'\0' || !wcscmp(ports, L"*");
    bool lets = false;
    if (application != nullptr && *application != L'\0')
    {
        // the rule may name the program with %ProgramFiles% and the like
        wchar_t expanded[MAX_PATH];
        const DWORD length = ExpandEnvironmentStringsW(application, expanded, MAX_PATH);
        const wchar_t *name = length > 0 && length <= MAX_PATH ? expanded : application;
        lets = !_wcsicmp(name, path) && (anyPort || portListHas(ports, port));
    }
    else if (!anyPort)
        lets = portListHas(ports, port);
    SysFreeString(application);
    SysFreeString(ports);
    return lets;
}

/* The name of the rule allowFirewall() adds */
static const wchar_t firewallRuleName[] = L"VLC Redesign";

/* Whether a rule is the one allowFirewall() adds (an inbound one, by its
 * name), whatever it lets in */
static bool isOwnRule(INetFwRule *rule)
{
    NET_FW_RULE_DIRECTION direction = NET_FW_RULE_DIR_OUT;
    BSTR name = nullptr;
    const bool own = SUCCEEDED(rule->get_Direction(&direction)) && direction == NET_FW_RULE_DIR_IN
                  && SUCCEEDED(rule->get_Name(&name)) && name != nullptr && !wcscmp(name, firewallRuleName);
    SysFreeString(name);
    return own;
}

struct FirewallState
{
    // the devices can connect (true when it can't tell)
    bool allows = true;
    // a network connected now is public, and the firewall keeps them out
    // there: the rule must be for the public networks too
    bool publicNetwork = false;
    // the rule of allowFirewall() is there (off, or not for this network):
    // changed rather than added again
    bool ruleExists = false;
    // a rule keeps them out (made when its question of Windows was
    // declined): it wins over any allowing one, it goes with the request
    bool blockRule = false;
};

/* Whether Windows Firewall lets the devices of the network connect to this
 * program on that TCP port. A TV reads the cast media from VLC's own
 * server: when blocked, it gives up at once. With the firewall on and
 * blocking by default, that takes a rule for VLC or the port; Windows may
 * not even ask (its notifications can be off). On a worker thread: it lists
 * every rule of the firewall. */
static FirewallState firewallState(long port)
{
    FirewallState state;
    wchar_t path[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return state;

    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool allows = true;
    INetFwPolicy2 *policy = nullptr;
    if (SUCCEEDED(CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
                                   __uuidof(INetFwPolicy2), reinterpret_cast<void **>(&policy))))
    {
        // the networks connected now (private, public, domain)
        LONG profiles = 0;
        policy->get_CurrentProfileTypes(&profiles);
        bool blocking = false;
        for (const NET_FW_PROFILE_TYPE2 type : { NET_FW_PROFILE2_DOMAIN, NET_FW_PROFILE2_PRIVATE,
                                                 NET_FW_PROFILE2_PUBLIC })
        {
            VARIANT_BOOL enabled = VARIANT_FALSE, blockAll = VARIANT_FALSE;
            NET_FW_ACTION inbound = NET_FW_ACTION_ALLOW;
            if (!(profiles & type) || FAILED(policy->get_FirewallEnabled(type, &enabled))
             || enabled == VARIANT_FALSE)
                continue;
            policy->get_DefaultInboundAction(type, &inbound);
            policy->get_BlockAllInboundTraffic(type, &blockAll);
            if (blockAll != VARIANT_FALSE)
            {
                blocking = true;
                profiles = 0;   // no rule lets anything in
                break;
            }
            if (inbound == NET_FW_ACTION_BLOCK)
            {
                blocking = true;
                if (type == NET_FW_PROFILE2_PUBLIC)
                    state.publicNetwork = true;
            }
        }

        if (blocking)
        {
            allows = false;
            INetFwRules *rules = nullptr;
            IUnknown *unknown = nullptr;
            IEnumVARIANT *it = nullptr;
            // (all of them, even when none can let anything in: whether
            // the rule of allowFirewall() is there)
            if (SUCCEEDED(policy->get_Rules(&rules))
             && SUCCEEDED(rules->get__NewEnum(&unknown))
             && SUCCEEDED(unknown->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void **>(&it))))
            {
                VARIANT item;
                VariantInit(&item);
                ULONG fetched = 0;
                while (it->Next(1, &item, &fetched) == S_OK)
                {
                    INetFwRule *rule = nullptr;
                    if (item.vt == VT_DISPATCH && item.pdispVal != nullptr
                     && SUCCEEDED(item.pdispVal->QueryInterface(__uuidof(INetFwRule),
                                                                reinterpret_cast<void **>(&rule))))
                    {
                        allows = allows || ruleMatches(rule, path, port, profiles, NET_FW_ACTION_ALLOW);
                        state.blockRule = state.blockRule
                                       || ruleMatches(rule, path, port, profiles, NET_FW_ACTION_BLOCK);
                        state.ruleExists = state.ruleExists || isOwnRule(rule);
                        rule->Release();
                    }
                    VariantClear(&item);
                }
                if (state.blockRule)
                    allows = false;
            }
            if (it)
                it->Release();
            if (unknown)
                unknown->Release();
            if (rules)
                rules->Release();
        }
        policy->Release();
    }
    if (SUCCEEDED(init))
        CoUninitialize();
    state.allows = allows;
    return state;
}
#endif

using RendererItemPtr = ::vlc::vlc_shared_data_ptr<vlc_renderer_item_t, &vlc_renderer_item_hold, &vlc_renderer_item_release>;
using vlc_player_locker = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;

struct ItemEntry
{
    ItemEntry(const RendererItemPtr& renderItem, bool current = true)
        : renderItem(renderItem)
        , currentScan(current)
    {}

    ItemEntry(){}

    RendererItemPtr renderItem;
    /**
     * we scan for 20 seconds, when the scan ends render discoverer are destroyed but
     * items are kept in the model, at this moment the items are marked as "old", if
     * a new scan starts and finish without finding them again, then they will be removed
     * from the model
     */
    bool currentScan = true;
};

class RendererManagerPrivate
{
public:
    RendererManagerPrivate(RendererManager* pub, qt_intf_t* const intf, vlc_player_t* const player)
        : p_intf(intf)
        , m_player(player)
        , q_ptr(pub)
    {
        assert(m_player);
        QObject::connect( &m_stop_scan_timer, &QTimer::timeout, q_ptr, [this](){ timerCountdown(); });
#ifdef _WIN32
        m_firewallWait.setInterval( 200 );
        QObject::connect( &m_firewallWait, &QTimer::timeout, q_ptr, [this](){ firewallRuleAdded(); });
#endif

        // read by the input when it starts casting (InitSout)
        m_displayHere = p_intf->mainSettings->value( "redesign/cast-display-here", false ).toBool();
        // devices not to show (by name)
        m_hidden = p_intf->mainSettings->value( "redesign/cast-hidden-devices" ).toStringList();
        vlc_object_t *libvlc = VLC_OBJECT( vlc_object_instance( p_intf ) );
        var_Create( libvlc, "renderer-display", VLC_VAR_BOOL );
        var_SetBool( libvlc, "renderer-display", m_displayHere );
        /* The controls of the device while it plays (the renderer's stream
         * output follows them, see PlayerController): paused, volume (0 to
         * 100), muted (-1: as it is); its delay behind this computer, for
         * the picture here; its volume as it says it */
        var_Create( libvlc, "renderer-paused", VLC_VAR_BOOL );
        var_Create( libvlc, "renderer-volume", VLC_VAR_INTEGER );
        var_SetInteger( libvlc, "renderer-volume", -1 );
        var_Create( libvlc, "renderer-muted", VLC_VAR_INTEGER );
        var_SetInteger( libvlc, "renderer-muted", -1 );
        var_Create( libvlc, "renderer-latency", VLC_VAR_INTEGER );
        var_Create( libvlc, "renderer-volume-device", VLC_VAR_INTEGER );
        var_SetInteger( libvlc, "renderer-volume-device", -1 );
        /* where the device is in the media (the input's time, -1: unknown),
         * said by its stream output as it plays */
        var_Create( libvlc, "renderer-time", VLC_VAR_INTEGER );
        var_SetInteger( libvlc, "renderer-time", -1 );
        /* stopped here: the device stops now (at the end of a media, it
         * shows what it has first) */
        var_Create( libvlc, "renderer-stop", VLC_VAR_VOID );
        /* how the device does, said by its stream output: "connecting",
         * "playing" or "failed:<why>" (unreachable, unreadable, refused,
         * stopped, taken), then "|" and the device (its url= or ip= option) */
        var_Create( libvlc, "renderer-state", VLC_VAR_STRING );
        var_AddCallback( libvlc, "renderer-volume-device", &deviceVolumeChanged, this );
        var_AddCallback( libvlc, "renderer-state", &castStateChanged, this );

        static struct vlc_player_cbs cbs = {};
        cbs.on_renderer_changed = &player_renderer_changed;
        {
            vlc_player_locker lock{ m_player };
            m_playerListener = vlc_player_AddListener(m_player, &cbs, this);
        }
    }

    ~RendererManagerPrivate()
    {
        var_DelCallback( VLC_OBJECT( vlc_object_instance( p_intf ) ), "renderer-volume-device",
                         &deviceVolumeChanged, this );
        var_DelCallback( VLC_OBJECT( vlc_object_instance( p_intf ) ), "renderer-state",
                         &castStateChanged, this );
#ifdef _WIN32
        // Windows' question still open runs the code of this module: its
        // answer first (the process it starts is not waited for)
        if (m_firewallAsking)
        {
            m_firewallAsking->wait();
            delete m_firewallAsking;
        }
        if (m_firewallProcess)
            CloseHandle(m_firewallProcess);
        // the check going on runs the code of this module: done before
        // it goes (its answer is dropped with the manager)
        if (m_firewallThread)
        {
            m_firewallThread->wait();
            delete m_firewallThread;
        }
#endif
        if (m_playerListener)
        {
            vlc_player_locker lock{ m_player };
            vlc_player_RemoveListener(m_player, m_playerListener);
        }
    }

    // the device's volume, said by its stream output (on its thread)
    static int deviceVolumeChanged( vlc_object_t *, const char *, vlc_value_t,
                                    vlc_value_t value, void *data )
    {
        RendererManagerPrivate *self = static_cast<RendererManagerPrivate *>( data );
        const int volume = value.i_int;
        if ( volume >= 0 )
            QMetaObject::invokeMethod( self->q_func(), [self, volume]() {
                emit self->q_func()->deviceVolumeReported( volume / 100.f );
            }, Qt::QueuedConnection );
        return VLC_SUCCESS;
    }

    // how the device does, said by its stream output (on its thread)
    static int castStateChanged( vlc_object_t *, const char *, vlc_value_t,
                                 vlc_value_t value, void *data )
    {
        RendererManagerPrivate *self = static_cast<RendererManagerPrivate *>( data );
        const QString state = qfu( value.psz_string );
        QMetaObject::invokeMethod( self->q_func(), [self, state]() {
            self->onCastState( state );
        }, Qt::QueuedConnection );
        return VLC_SUCCESS;
    }

    // the device it is about plays now (not one cast to before)
    bool isSelected( const QString &device ) const
    {
        if ( !m_selectedItem || device.isEmpty() )
            return false;
        const QString sout = qfu( vlc_renderer_item_sout( m_selectedItem.get() ) );
        return sout.contains( QStringLiteral( "url=" ) + device + QLatin1Char( ',' ) )
            || sout.contains( QStringLiteral( "url=" ) + device + QLatin1Char( '}' ) )
            || sout.contains( QStringLiteral( "ip=" ) + device + QLatin1Char( ',' ) )
            || sout.contains( QStringLiteral( "ip=" ) + device + QLatin1Char( '}' ) );
    }

    void setCastStatus( RendererManager::CastStatus status, const QString &error )
    {
        Q_Q(RendererManager);
        if ( status == m_castStatus && error == m_castError )
            return;
        m_castStatus = status;
        m_castError = error;
        emit q->castStatusChanged();
    }

    void onCastState( const QString &state )
    {
        const qsizetype bar = state.indexOf( QLatin1Char( '|' ) );
        if ( bar < 0 || !isSelected( state.mid( bar + 1 ) ) )
            return;
        const QString what = state.left( bar );
        if ( what == QLatin1String( "connecting" ) )
            setCastStatus( RendererManager::CastConnecting, {} );
        else if ( what == QLatin1String( "playing" ) )
            setCastStatus( RendererManager::CastPlaying, {} );
        else if ( what.startsWith( QLatin1String( "failed:" ) ) )
        {
            const bool wasFailed = m_castStatus == RendererManager::CastFailed;
            setCastStatus( RendererManager::CastFailed, what.mid( 7 ) );
            // nothing plays anywhere: the media waits where it is, for the
            // user to try again or to play it here
            vlc_player_locker lock{ m_player };
            if ( !wasFailed && vlc_player_GetState( m_player ) == VLC_PLAYER_STATE_PLAYING )
            {
                vlc_player_Pause( m_player );
                m_pausedForFailure = true;
            }
        }
    }

    // the user chose where it plays: what waited after a failure goes on
    void resumeAfterFailure()
    {
        if ( !m_pausedForFailure )
            return;
        m_pausedForFailure = false;
        vlc_player_locker lock{ m_player };
        vlc_player_Resume( m_player );
    }

    void timerCountdown()
    {
        Q_Q(RendererManager);
        if( m_stop_scan_timer.isActive() && m_scan_remain > 0 )
        {
            m_scan_remain--;
            emit q->scanRemainChanged();
        }
        else
        {
            q->StopScan();
        }
    }

    void setStatus(RendererManager::RendererStatus status)
    {
        Q_Q(RendererManager);
        if (status == m_status)
            return;
        m_status = status;
        emit q->statusChanged();
    }


    // a device not to show: by its name, with or without its model after
    // it ("Living Room TV (Chromecast Ultra)", as mDNS names it)
    bool isHidden(const QString &name) const
    {
        for (const QString &hidden : m_hidden)
        {
            const QString h = hidden.trimmed();
            if (!h.isEmpty() && (name.compare(h, Qt::CaseInsensitive) == 0
                                 || name.startsWith(h + QStringLiteral(" ("), Qt::CaseInsensitive)))
                return true;
        }
        return false;
    }

    void onRendererAdded(const RendererItemPtr& renderItem) {
        Q_Q(RendererManager);
        if (isHidden(qfu(vlc_renderer_item_name(renderItem.get()))))
            return;
        QString key = getItemKey(renderItem);
        if( !m_items.contains( key ) )
        {
            emit q->beginInsertRows({}, m_itemsIndex.size(), m_itemsIndex.size());
            m_items.emplace( key, renderItem );
            m_itemsIndex.push_back(key);
            emit q->endInsertRows();
        }
        else
        {
            /* mark the item as seen during this scan */
            ItemEntry& entry = m_items[key];
            entry.currentScan = true;
            /* the same device found the other way: the longer name (with
             * its model) shows; the one playing stays */
            const QString known = qfu(vlc_renderer_item_name(entry.renderItem.get()));
            const QString found = qfu(vlc_renderer_item_name(renderItem.get()));
            if (entry.renderItem != m_selectedItem && found.size() > known.size()
                && found.startsWith(known))
            {
                entry.renderItem = renderItem;
                const auto row = q->index(m_itemsIndex.indexOf(key));
                emit q->dataChanged(row, row);
            }
        }
    }

    void onRendererRemoved(const RendererItemPtr& renderItem) {
        Q_Q(RendererManager);

        QString key = getItemKey(renderItem);
        if( m_items.contains( key ) )
        {
            //only remove the item if we are using it
            //and if we are actively scanning
            if( m_selectedItem != renderItem && m_status == RendererManager::RUNNING )
            {
                qsizetype index = m_itemsIndex.indexOf(key);
                emit q->beginRemoveRows({}, index, index);
                m_items.remove(key);
                m_itemsIndex.remove(index);
                emit q->endRemoveRows();
            }
        }
    }

    void onPlayerRendererChanged(const RendererItemPtr& renderItem)
    {
        Q_Q(RendererManager);

        if (m_selectedItem == renderItem)
            return;

        if (m_selectedItem)
        {
            int oldIndexRow = getItemRow(m_selectedItem);
            m_selectedItem.reset();
            if (oldIndexRow >= 0)
            {
                auto oldIndex = q->index(oldIndexRow);
                emit q->dataChanged(oldIndex, oldIndex, { Qt::CheckStateRole, RendererManager::SELECTED });
            }
        }

        if (renderItem)
        {
            m_selectedItem = renderItem;

            int newIndexRow= getItemRow(m_selectedItem);
            if (newIndexRow >= 0)
            {
                auto newIndex = q->index(newIndexRow);
                emit q->dataChanged(newIndex, newIndex, { Qt::CheckStateRole, RendererManager::SELECTED });
            }
            else
            {
                QString key = getItemKey(m_selectedItem);
                emit q->beginInsertRows({}, m_itemsIndex.size(), m_itemsIndex.size());
                //renderer is not part of the current scan, mark it like it
                m_items.emplace( key, renderItem, false );
                m_itemsIndex.push_back(key);
                emit q->endInsertRows();
            }
        }
        setCastStatus( renderItem ? RendererManager::CastConnecting : RendererManager::CastIdle, {} );
        emit q->useRendererChanged();
        checkFirewall();
    }

    // can the device read from VLC (see firewallState): asked on a worker
    // thread, one check at a time, the last one asked counting
    void checkFirewall()
    {
        const quint64 generation = ++m_firewallGeneration;
#ifdef _WIN32
        long port = 0;
        QString address;
        if (m_selectedItem)
        {
            address = deviceAddress(m_selectedItem);
            // the port of the server the device reads from
            const char *type = vlc_renderer_item_type(m_selectedItem.get());
            const char *option = !strcmp(type, "stream_out_dlna") ? "sout-dlna-http-port"
                               : !strcmp(type, "chromecast") ? "sout-chromecast-http-port"
                               : nullptr;
            if (option && config_FindConfig(option))
                port = config_GetInt(option);
        }
        if (port > 0)
        {
            if (m_firewallThread)
            {
                // once the one going on is done
                m_firewallRecheck = true;
                return;
            }
            auto result = std::make_shared<FirewallState>();
            // a device on this computer reads the server without the firewall
            m_firewallThread = QThread::create([result, port, address]() {
                if (!isThisComputer(address))
                    *result = firewallState(port);
            });
            // (by the finished signal: dropped if the manager is gone)
            QObject::connect(m_firewallThread, &QThread::finished, q_ptr, [this, result, generation]() {
                m_firewallThread->deleteLater();
                m_firewallThread = nullptr;
                if (m_firewallRecheck)
                {
                    m_firewallRecheck = false;
                    checkFirewall();
                }
                else if (generation == m_firewallGeneration)
                {
                    m_firewallBlockRule = result->blockRule;
                    setFirewall(!result->allows, result->publicNetwork, result->ruleExists);
                }
            });
            m_firewallThread->start(QThread::LowPriority);
            return;
        }
        m_firewallRecheck = false;
#endif
        Q_UNUSED(generation);
        setFirewall(false, false, false);
    }

    void setFirewall(bool blocked, bool publicNetwork, bool ruleExists)
    {
        Q_Q(RendererManager);
        const bool retry = m_retryWhenAllowed;
        m_retryWhenAllowed = false;
        m_firewallRuleExists = ruleExists;
        if (blocked != m_firewallBlocked || publicNetwork != m_firewallPublic)
        {
            m_firewallBlocked = blocked;
            m_firewallPublic = publicNetwork;
            emit q->firewallChanged();
            emit q->castStatusChanged();    // (its message)
        }
        // Casting, kept out: Windows is asked at once for the rule (its own
        // approval, once), not after a question here; declined, the card
        // offers it again
        if (blocked && m_selectedItem && !m_firewallAutoAsked && !m_firewallAllowing)
        {
            m_firewallAutoAsked = true;
            q->allowFirewall();
        }
        // the rule added: the device gave up reading, it starts again from
        // where it is
        if (retry && !blocked && m_selectedItem)
            q->retry();
    }

#ifdef _WIN32
    // netsh (allowFirewall) is done: cast again if the device can connect now
    void firewallRuleAdded()
    {
        Q_Q(RendererManager);
        if (WaitForSingleObject(m_firewallProcess, 0) != WAIT_OBJECT_0)
            return;
        m_firewallWait.stop();
        CloseHandle(m_firewallProcess);
        m_firewallProcess = nullptr;
        m_firewallAllowing = false;
        emit q->firewallChanged();

        // (there now, unless the check says otherwise: not added twice)
        m_firewallRuleExists = true;
        m_retryWhenAllowed = true;
        checkFirewall();
    }
#endif

    // The address of a device (the "ip" of its output), empty when unknown
    static QString deviceAddress(const RendererItemPtr& item)
    {
        static const QRegularExpression ipRe(QStringLiteral(R"re([{,]ip=([^,}]+))re"));
        const QRegularExpressionMatch m = ipRe.match(qfu(vlc_renderer_item_sout(item.get())));
        return m.hasMatch() ? m.captured(1) : QString();
    }

    // A device: a Chromecast by its address (its name differs with the way
    // it was found), the others by their output
    QString getItemKey(const RendererItemPtr& item) const
    {
        const char* type = vlc_renderer_item_type(item.get());
        const QString sout = qfu(vlc_renderer_item_sout(item.get()));
        if (!strcmp(type, "chromecast"))
        {
            static const QRegularExpression addressRe(QStringLiteral(R"re([{,]ip=([^,}]+)(?:.*[{,]port=(\d+))?)re"));
            const QRegularExpressionMatch m = addressRe.match(sout);
            if (m.hasMatch())
                return QStringLiteral("chromecast-%1:%2").arg(m.captured(1), m.captured(2));
        }
        return QString("%1-%2").arg(type).arg(sout);
    }

    // The device playing stops at once (another one is chosen, or this
    // computer): its output does not wait for it to show what it has
    void stopCurrent()
    {
        if (m_selectedItem)
            var_TriggerCallback(VLC_OBJECT(vlc_object_instance(p_intf)), "renderer-stop");
    }

    int getItemRow(const RendererItemPtr& item) const
    {
        QString newKey = getItemKey(item);
        return m_itemsIndex.indexOf(newKey);
    }

    qt_intf_t* const p_intf;
    vlc_player_t* const m_player;

    vlc_player_listener_id* m_playerListener = nullptr;

    RendererManager::RendererStatus m_status = RendererManager::IDLE;

    std::vector<vlc_renderer_discovery_t*> m_rds;


    //the elements of the model (in model order), the string is the key of m_items
    QVector<QString> m_itemsIndex;

    QHash<QString, ItemEntry> m_items;

    //renderer currently used by the player
    RendererItemPtr m_selectedItem;

    //model automatically stops scanning after a while
    QTimer m_stop_scan_timer;

    bool m_firewallBlocked = false;
    bool m_firewallAllowing = false;
    // see FirewallState
    bool m_firewallPublic = false;
    bool m_firewallRuleExists = false;
    // a rule of the firewall keeps this program out: removed with the request
    bool m_firewallBlockRule = false;
    // allowFirewall() was called by itself once (not again if declined)
    bool m_firewallAutoAsked = false;
    // the last check asked (the answers of the others are dropped)
    quint64 m_firewallGeneration = 0;
    // cast again once the check says the device can connect (the rule added)
    bool m_retryWhenAllowed = false;
    bool m_displayHere = false;
    RendererManager::CastStatus m_castStatus = RendererManager::CastIdle;
    // why it failed (see "renderer-state")
    QString m_castError;
    // paused when the device failed, until the user chooses
    bool m_pausedForFailure = false;
    QStringList m_hidden;
#ifdef _WIN32
    HANDLE m_firewallProcess = nullptr;
    // asking Windows for the rule (allowFirewall)
    QThread *m_firewallAsking = nullptr;
    QTimer m_firewallWait;
    // the check going on, and whether to check again once it is done
    QThread *m_firewallThread = nullptr;
    bool m_firewallRecheck = false;
#endif
    //remaining time in seconds
    unsigned m_scan_remain = 0;

    RendererManager* q_ptr;
    Q_DECLARE_PUBLIC(RendererManager);
};

void RendererManager::disableRenderer()
{
    Q_D(RendererManager);
    if (!d->m_selectedItem)
        return;

    d->stopCurrent();
    {
        vlc_player_locker lock{ d->m_player };
        //vlc_player synchronously report the renderer change
        vlc_player_SetRenderer( d->m_player, nullptr );
    }
    d->resumeAfterFailure();
}

// The same device again: the input makes its output again (a new stream,
// from where the media is), without playing here in between (nor this
// computer's volume going to the device)
void RendererManager::retry()
{
    Q_D(RendererManager);
    if (!d->m_selectedItem)
        return;
    RendererItemPtr item = d->m_selectedItem;
    d->setCastStatus( CastConnecting, {} );
    d->stopCurrent();
    {
        vlc_player_locker lock{ d->m_player };
        vlc_player_SetRenderer( d->m_player, item.get() );
    }
    d->resumeAfterFailure();
}

RendererManager::CastStatus RendererManager::castStatus() const
{
    Q_D(const RendererManager);
    return d->m_castStatus;
}

QString RendererManager::castMessage() const
{
    Q_D(const RendererManager);
    const QString name = rendererName();
    if (d->m_castStatus != CastFailed)
        return {};
    if (d->m_castError == QLatin1String("unreachable"))
        return qtr("%1 doesn't answer. Check that it's on, and on the same network as this computer.").arg(name);
    if (d->m_castError == QLatin1String("unreadable"))
        return d->m_firewallBlocked
            ? qtr("Windows Firewall doesn't let %1 connect to this computer to read the video.").arg(name)
            : qtr("%1 couldn't read the video from this computer.").arg(name);
    if (d->m_castError == QLatin1String("refused"))
        return qtr("%1 refused to play this video.").arg(name);
    if (d->m_castError == QLatin1String("stopped"))
        return qtr("%1 stopped playing.").arg(name);
    if (d->m_castError == QLatin1String("taken"))
        return qtr("%1 is playing something else now.").arg(name);
    return qtr("%1 couldn't play the video.").arg(name);
}

QVariant RendererManager::data(const QModelIndex &index, int role) const
{
    Q_D(const RendererManager);
    if (index.row() >= d->m_itemsIndex.size())
        return {};

    const ItemEntry& entry = d->m_items.value(d->m_itemsIndex[index.row()]);
    vlc_renderer_item_t* item = entry.renderItem.get();
    switch (role)
    {
    case Qt::DisplayRole:
    case NAME:
        return QString(vlc_renderer_item_name(item));
    case TYPE:
        return QString(vlc_renderer_item_type(item));
    case DEMUX_FILTER:
        return QString(vlc_renderer_item_demux_filter(item));
    case SOUT:
        return QString(vlc_renderer_item_sout(item));
    case ICON_URI:
    case Qt::DecorationRole:
    {
        const char* iconUri = vlc_renderer_item_icon_uri(item);
        if (!iconUri)
        {
            if (vlc_renderer_item_flags( item ) & VLC_RENDERER_CAN_VIDEO)
                return QString("qrc://menu/movie.svg");
            else
                return QString("qrc://menu/music.svg");
        }
        return QString(iconUri);
    }
    case FLAGS:
        return vlc_renderer_item_flags(entry.renderItem.get());
    case SELECTED:
        return entry.renderItem == d->m_selectedItem;
    case Qt::CheckStateRole:
        return entry.renderItem == d->m_selectedItem ? Qt::Checked : Qt::Unchecked;
    }
    return {};
}

bool RendererManager::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_D(RendererManager);
    if (index.row() >= d->m_itemsIndex.size())
        return false;

    QString sout = d->m_itemsIndex[index.row()];
    ItemEntry& entry = d->m_items[sout];
    switch (role)
    {
    case Qt::CheckStateRole:
    case SELECTED:
    {
        bool enableRenderer;

        if ( value.canConvert<bool>() )
            enableRenderer = value.toBool();
        else
            return false;
        // already playing there: nothing to do
        if ( enableRenderer && entry.renderItem == d->m_selectedItem )
            return true;
        d->stopCurrent();

        {
            vlc_player_locker lock{ d->m_player };
            //vlc_player synchronously report the renderer change
            vlc_player_SetRenderer(
                d->m_player,
                enableRenderer ? entry.renderItem.get() : nullptr
            );
        }
        d->resumeAfterFailure();
        return true;
    }
    default:
        return false;
    }
}

QHash<int,QByteArray> RendererManager::roleNames() const
{
    return {
        { NAME, QByteArrayLiteral("name") },
        { TYPE, QByteArrayLiteral("type") },
        { DEMUX_FILTER, QByteArrayLiteral("demuxFilter") },
        { SOUT, QByteArrayLiteral("sout") },
        { ICON_URI, QByteArrayLiteral("iconUri") },
        { FLAGS, QByteArrayLiteral("flags") },
        { SELECTED, QByteArrayLiteral("selected") },
    };
}

int RendererManager::rowCount(const QModelIndex&) const
{
    Q_D(const RendererManager);
    return d->m_itemsIndex.size();
}


static void renderer_event_item_added( vlc_renderer_discovery_t* p_rd, vlc_renderer_item_t *p_item )
{
    RendererManagerPrivate *self = reinterpret_cast<RendererManagerPrivate*>( p_rd->owner.sys );
    RendererItemPtr renderItem(p_item);
    QMetaObject::invokeMethod(self->q_func(), [self, renderItem = std::move(renderItem)](){
        self->onRendererAdded(renderItem);
    }, Qt::QueuedConnection);
}

static void renderer_event_item_removed( vlc_renderer_discovery_t *p_rd,
                                 vlc_renderer_item_t *p_item )
{
    RendererManagerPrivate *self = reinterpret_cast<RendererManagerPrivate*>( p_rd->owner.sys );
    RendererItemPtr renderItem(p_item);
    QMetaObject::invokeMethod(self->q_func(), [self, renderItem = std::move(renderItem)](){
        self->onRendererRemoved(renderItem);
    }, Qt::QueuedConnection);
}

// The player calls with its lock taken, and on the interface thread itself
// when the renderer is set from here (vlc_player_SetRenderer): queued, so
// the interface does not read the player (locking it again) from in there
static void player_renderer_changed(vlc_player_t *, vlc_renderer_item_t *new_item, void *data)
{
    RendererManagerPrivate *self = reinterpret_cast<RendererManagerPrivate*>( data);
    RendererItemPtr renderItem(new_item);
    QMetaObject::invokeMethod(self->q_func(), [self, renderItem = std::move(renderItem)](){
        self->onPlayerRendererChanged(renderItem);
    }, Qt::QueuedConnection);
}

RendererManager::RendererManager( qt_intf_t *p_intf_, vlc_player_t* player )
    : d_ptr(new RendererManagerPrivate(this, p_intf_, player))
{
}

RendererManager::~RendererManager()
{
    StopScan();
}

int RendererManager::getScanRemain() const
{
    Q_D(const RendererManager);
    return d->m_scan_remain;
}

RendererManager::RendererStatus RendererManager::getStatus() const
{
    Q_D(const RendererManager);
    return d->m_status;
}

void RendererManager::select(int row)
{
    if (row >= 0 && row < rowCount())
        setData(index(row), true, SELECTED);
}

QString RendererManager::rendererName() const
{
    Q_D(const RendererManager);
    return d->m_selectedItem ? qfu(vlc_renderer_item_name(d->m_selectedItem.get())) : QString();
}

bool RendererManager::rendererVideo() const
{
    Q_D(const RendererManager);
    return d->m_selectedItem
        && (vlc_renderer_item_flags(d->m_selectedItem.get()) & VLC_RENDERER_CAN_VIDEO);
}

bool RendererManager::firewallBlocked() const
{
    Q_D(const RendererManager);
    return d->m_firewallBlocked;
}

bool RendererManager::firewallAllowing() const
{
    Q_D(const RendererManager);
    return d->m_firewallAllowing;
}

bool RendererManager::firewallPublic() const
{
    Q_D(const RendererManager);
    return d->m_firewallPublic;
}

bool RendererManager::displayHereAvailable() const
{
    Q_D(const RendererManager);
    return !d->m_selectedItem
        || !strcmp( vlc_renderer_item_type( d->m_selectedItem.get() ), "stream_out_dlna" );
}

bool RendererManager::displayHere() const
{
    Q_D(const RendererManager);
    return d->m_displayHere;
}

void RendererManager::setDisplayHere(bool display)
{
    Q_D(RendererManager);
    if (display == d->m_displayHere)
        return;
    d->m_displayHere = display;
    d->p_intf->mainSettings->setValue( "redesign/cast-display-here", display );
    var_SetBool( VLC_OBJECT( vlc_object_instance( d->p_intf ) ), "renderer-display", display );
    emit displayHereChanged();

    // (casting: a DLNA device's output follows the variable, it goes on; a
    // Chromecast has no picture here)
}

void RendererManager::allowFirewall()
{
#ifdef _WIN32
    Q_D(RendererManager);
    if (d->m_firewallAllowing)
        return;
    wchar_t path[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return;

    // A rule for this program, on the private and domain networks, and on
    // the public ones when this computer is on one (no rule of the others
    // counts there). The rule there already is changed, not added again.
    // Windows asks for an administrator's approval (declined: nothing).
    const std::wstring name = L"name=\"" + std::wstring(firewallRuleName) + L"\" dir=in ";
    const std::wstring rule = L"action=allow program=\"" + std::wstring(path) + L"\" enable=yes profile="
                            + (d->m_firewallPublic ? L"private,domain,public" : L"private,domain");
    std::wstring file, parameters;
    if (d->m_firewallBlockRule)
    {
        // A rule keeping it out (Windows' own question declined once) wins
        // over any allowing one: this program's inbound rules go, then ours
        // comes, in one approval (cmd /S strips the outer quotes)
        file = L"cmd.exe";
        parameters = L"/S /C \"netsh advfirewall firewall delete rule name=all dir=in program=\""
                   + std::wstring(path) + L"\" & netsh advfirewall firewall add rule " + name + rule + L"\"";
    }
    else
    {
        file = L"netsh.exe";
        parameters = d->m_firewallRuleExists
            ? L"advfirewall firewall set rule " + name + L"new " + rule
            : L"advfirewall firewall add rule " + name + rule;
    }

    d->m_firewallAllowing = true;
    emit firewallChanged();
    // The call waits for the answer to Windows' question, which can come
    // behind other windows: on a thread of its own, the interface goes on
    auto process = std::make_shared<HANDLE>(nullptr);
    d->m_firewallAsking = QThread::create([file, parameters, process]() {
        const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        SHELLEXECUTEINFOW info = {};
        info.cbSize = sizeof(info);
        info.fMask = SEE_MASK_NOCLOSEPROCESS;
        info.lpVerb = L"runas";
        info.lpFile = file.c_str();
        info.lpParameters = parameters.c_str();
        info.nShow = SW_HIDE;
        if (ShellExecuteExW(&info) && info.hProcess)
            *process = info.hProcess;
        if (SUCCEEDED(init))
            CoUninitialize();
    });
    connect(d->m_firewallAsking, &QThread::finished, this, [this, process]() {
        Q_D(RendererManager);
        d->m_firewallAsking->deleteLater();
        d->m_firewallAsking = nullptr;
        if (*process == nullptr)
        {
            // declined: the card offers it again
            d->m_firewallAllowing = false;
            emit firewallChanged();
            return;
        }
        d->m_firewallProcess = *process;
        d->m_firewallWait.start();
    });
    d->m_firewallAsking->start();
#endif
}

bool RendererManager::useRenderer() const
{
    Q_D(const RendererManager);
    return d->m_selectedItem.get() != nullptr;
}

void RendererManager::StartScan()
{
    Q_D(RendererManager);
    if( d->m_stop_scan_timer.isActive() )
        return;

    /* SD subnodes */
    char **ppsz_longnames;
    char **ppsz_names;
    if( vlc_rd_get_names( d->p_intf, &ppsz_names, &ppsz_longnames ) != VLC_SUCCESS )
    {
        d->setStatus( RendererManager::RendererStatus::FAILED );
        return;
    }

    struct vlc_renderer_discovery_owner owner =
    {
        d,
        renderer_event_item_added,
        renderer_event_item_removed,
    };

    char **ppsz_name = ppsz_names, **ppsz_longname = ppsz_longnames;
    for( ; *ppsz_name; ppsz_name++, ppsz_longname++ )
    {
        msg_Dbg( d->p_intf, "starting renderer discovery service %s", *ppsz_longname );
        vlc_renderer_discovery_t* p_rd = vlc_rd_new( VLC_OBJECT(d->p_intf), *ppsz_name, &owner );
        if( p_rd != NULL )
            d->m_rds.push_back( p_rd );
        free( *ppsz_name );
        free( *ppsz_longname );
    }
    free( ppsz_names );
    free( ppsz_longnames );

    d->m_scan_remain = 20;
    d->m_stop_scan_timer.setInterval( 1000 );
    d->m_stop_scan_timer.start();
    d->setStatus( RendererManager::RendererStatus::RUNNING );
    emit scanRemainChanged();
}

void RendererManager::StopScan()
{
    Q_D(RendererManager);
    d->m_stop_scan_timer.stop();

    for ( vlc_renderer_discovery_t* p_rd : d->m_rds )
        vlc_rd_release( p_rd );
    d->m_rds.clear();

    /* Cleanup of outdated items, and notify removal */
    for (int i = d->m_itemsIndex.size() - 1; i >= 0; --i)
    {
        QString key = d->m_itemsIndex[i];
        ItemEntry& entry = d->m_items[key];
        if( !entry.currentScan && entry.renderItem != d->m_selectedItem )
        {
            //remove items from previous scans
            emit beginRemoveRows({}, i, i);
            d->m_items.remove(key);
            d->m_itemsIndex.remove(i);
            emit endRemoveRows();
        }
        else
        {
            /* don't keep if not updated by next detect */
            entry.currentScan = false;
        }
    }
    d->setStatus( RendererManager::RendererStatus::IDLE );
}
