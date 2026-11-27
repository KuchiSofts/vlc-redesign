/*****************************************************************************
 * win_media_controls.cpp: Windows System Media Transport Controls
 *****************************************************************************
 * SPDX-License-Identifier: GPL-2.0-or-later
 *****************************************************************************/

#if HAVE_CONFIG_H
# include "config.h"
#endif

// llvm-mingw's windows.foundation.h (the ARM64 build's toolchain) gives
// IReference both a BYTE and a boolean specialization, the same unsigned char
// for C++: "redefinition of IReference<unsigned char>". Nothing here uses
// IReference<BYTE>, so its block is left out (gcc's headers build either way).
#ifndef ____FIReference_1_BYTE_INTERFACE_DEFINED__
# define ____FIReference_1_BYTE_INTERFACE_DEFINED__
#endif

#include "win_media_controls.hpp"

#if __has_include(<systemmediatransportcontrolsinterop.h>) && __has_include(<windows.media.h>)
# include <systemmediatransportcontrolsinterop.h>
# include <windows.media.h>
#endif

#ifdef ____x_ABI_CWindows_CMedia_CISystemMediaTransportControls_INTERFACE_DEFINED__

#include "player/player_controller.hpp"
#include "playlist/playlist_controller.hpp"

#include <roapi.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>

#include <atomic>
#include <mutex>
#include <new>
#include <utility>

// MinGW-w64 12 and 13 only forward declare this interface. The layout is
// from Windows.Media.IVideoDisplayProperties in the Windows SDK.
#ifndef ____x_ABI_CWindows_CMedia_CIVideoDisplayProperties_INTERFACE_DEFINED__
namespace ABI::Windows::Media
{
    struct IVideoDisplayProperties : IInspectable
    {
        virtual HRESULT STDMETHODCALLTYPE get_Title(HSTRING *value) = 0;
        virtual HRESULT STDMETHODCALLTYPE put_Title(HSTRING value) = 0;
        virtual HRESULT STDMETHODCALLTYPE get_Subtitle(HSTRING *value) = 0;
        virtual HRESULT STDMETHODCALLTYPE put_Subtitle(HSTRING value) = 0;
    };
}
#endif

namespace {

using namespace ABI::Windows::Media;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Wrappers::HStringReference;
using vlc::playlist::PlaylistController;

using ButtonPressedHandler =
    __FITypedEventHandler_2_Windows__CMedia__CSystemMediaTransportControls_Windows__CMedia__CSystemMediaTransportControlsButtonPressedEventArgs;

MediaPlaybackStatus playbackStatus(PlayerController::PlayingState state)
{
    switch (state)
    {
        case PlayerController::PLAYING_STATE_PLAYING:
            return MediaPlaybackStatus_Playing;
        case PlayerController::PLAYING_STATE_PAUSED:
            return MediaPlaybackStatus_Paused;
        case PlayerController::PLAYING_STATE_STARTED:
        case PlayerController::PLAYING_STATE_STOPPING:
            return MediaPlaybackStatus_Changing;
        case PlayerController::PLAYING_STATE_STOPPED:
            return MediaPlaybackStatus_Stopped;
    }
    return MediaPlaybackStatus_Closed;
}

} // namespace

struct WinMediaControls::Private
{
    // MinGW-w64 has ComPtr and HStringReference but does not ship wrl/event.h.
    class ButtonHandler final : public ButtonPressedHandler, public IAgileObject
    {
    public:
        explicit ButtonHandler(WinMediaControls *owner) : m_owner(owner) {}

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **object) override
        {
            if (!object)
                return E_POINTER;

            if (iid == __uuidof(IUnknown) || iid == __uuidof(ButtonPressedHandler))
                *object = static_cast<ButtonPressedHandler *>(this);
            else if (iid == __uuidof(IAgileObject))
                *object = static_cast<IAgileObject *>(this);
            else
            {
                *object = nullptr;
                return E_NOINTERFACE;
            }
            AddRef();
            return S_OK;
        }

        ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refs; }

        ULONG STDMETHODCALLTYPE Release() override
        {
            const ULONG refs = --m_refs;
            if (!refs)
                delete this;
            return refs;
        }

        HRESULT STDMETHODCALLTYPE Invoke(ISystemMediaTransportControls *,
                                         ISystemMediaTransportControlsButtonPressedEventArgs *args) override
        {
            SystemMediaTransportControlsButton button;
            const HRESULT hr = args->get_Button(&button);
            if (FAILED(hr))
                return hr;

            // Unregistering a WinRT event can leave a callback in flight.
            // Hold the lock through enqueueing so teardown can safely detach.
            const std::lock_guard lock(m_mutex);
            if (m_owner)
                QMetaObject::invokeMethod(m_owner, [owner = m_owner, button]() {
                    if (owner->isActive())
                        owner->d->handleButton(button);
                }, Qt::QueuedConnection);
            return S_OK;
        }

        void detach()
        {
            const std::lock_guard lock(m_mutex);
            m_owner = nullptr;
        }

    private:
        std::atomic<ULONG> m_refs{1};
        std::mutex m_mutex;
        WinMediaControls *m_owner;
    };

    explicit Private(qt_intf_t *intf) : p_intf(intf), hasInput(THEMIM->hasInput()) {}

    HRESULT initialize(WinMediaControls *owner, HWND hwnd)
    {
        // Qt uses OleInitialize on the GUI thread. Reuse the existing apartment
        // if its mode differs, and only balance a successful RoInitialize call.
        HRESULT hr = RoInitialize(RO_INIT_SINGLETHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
            return hr;
        uninitialize = SUCCEEDED(hr);

        const HStringReference className(RuntimeClass_Windows_Media_SystemMediaTransportControls);
        ComPtr<ISystemMediaTransportControlsInterop> interop;
        hr = RoGetActivationFactory(className.Get(), IID_PPV_ARGS(interop.GetAddressOf()));
        if (FAILED(hr))
            return hr;
        hr = interop->GetForWindow(hwnd, IID_PPV_ARGS(controls.GetAddressOf()));
        if (FAILED(hr))
            return hr;

        if (FAILED(hr = controls->put_IsEnabled(false)) ||
            FAILED(hr = controls->get_DisplayUpdater(updater.GetAddressOf())) ||
            FAILED(hr = controls->put_IsPlayEnabled(true)) ||
            FAILED(hr = controls->put_IsPauseEnabled(true)) ||
            FAILED(hr = controls->put_IsStopEnabled(true)))
            return hr;

        handler.Attach(new (std::nothrow) ButtonHandler(owner));
        if (!handler.Get())
            return E_OUTOFMEMORY;
        hr = controls->add_ButtonPressed(handler.Get(), &buttonToken);
        if (FAILED(hr))
            return hr;
        registered = true;

        return update();
    }

    void handleButton(SystemMediaTransportControlsButton button)
    {
        switch (button)
        {
            case SystemMediaTransportControlsButton_Play:
                if (THEMIM->isStarted())
                    THEMPL->resume();
                else
                    THEMPL->play();
                break;
            case SystemMediaTransportControlsButton_Pause:
                THEMPL->pause();
                break;
            case SystemMediaTransportControlsButton_Next:
                THEMPL->next();
                break;
            case SystemMediaTransportControlsButton_Previous:
                THEMPL->prev();
                break;
            case SystemMediaTransportControlsButton_Stop:
                THEMPL->stop();
                break;
            default:
                break;
        }
    }

    HRESULT updateMetadata()
    {
        const bool video = THEMIM->hasVideoOutput();
        HRESULT hr = updater->put_Type(video ? MediaPlaybackType_Video : MediaPlaybackType_Music);
        if (FAILED(hr))
            return hr;

        const QString title = THEMIM->getTitle();
        const auto titleText = (title.isEmpty() ? THEMIM->getName() : title).toStdWString();
        const HStringReference titleRef(titleText.c_str(), titleText.size());
        const QString artist = THEMIM->getArtist();
        const QString album = THEMIM->getAlbum();
        if (video)
        {
            ComPtr<IVideoDisplayProperties> properties;
            hr = updater->get_VideoProperties(properties.GetAddressOf());
            if (FAILED(hr) || FAILED(hr = properties->put_Title(titleRef.Get())))
                return hr;

            // Video has one subtitle field for the artist and album.
            QString subtitle = artist;
            if (!artist.isEmpty() && !album.isEmpty())
                subtitle += QStringLiteral(" · ");
            subtitle += album;
            const auto subtitleText = subtitle.toStdWString();
            const HStringReference subtitleRef(subtitleText.c_str(), subtitleText.size());
            hr = properties->put_Subtitle(subtitleRef.Get());
        }
        else
        {
            ComPtr<IMusicDisplayProperties> properties;
            hr = updater->get_MusicProperties(properties.GetAddressOf());
            if (FAILED(hr))
                return hr;

            const auto artistText = artist.toStdWString();
            const HStringReference artistRef(artistText.c_str(), artistText.size());
            if (FAILED(hr = properties->put_Title(titleRef.Get())) ||
                FAILED(hr = properties->put_Artist(artistRef.Get())))
                return hr;

            ComPtr<IMusicDisplayProperties2> properties2;
            hr = properties.As(&properties2);
            if (FAILED(hr))
                return hr;
            const auto albumText = album.toStdWString();
            const HStringReference albumRef(albumText.c_str(), albumText.size());
            hr = properties2->put_AlbumTitle(albumRef.Get());
        }
        return SUCCEEDED(hr) ? updater->Update() : hr;
    }

    HRESULT update()
    {
        const auto state = THEMIM->getPlayingState();
        const bool showMedia = hasInput && state != PlayerController::PLAYING_STATE_STOPPED;
        const auto status = hasInput ? playbackStatus(state) : MediaPlaybackStatus_Closed;
        HRESULT hr;
        if (FAILED(hr = controls->put_PlaybackStatus(status)) ||
            FAILED(hr = controls->put_IsNextEnabled(THEMPL->hasNext())) ||
            FAILED(hr = controls->put_IsPreviousEnabled(THEMPL->hasPrev())))
            return hr;

        if (enabled != showMedia)
        {
            hr = controls->put_IsEnabled(showMedia);
            if (FAILED(hr))
                return hr;
            enabled = showMedia;
        }
        return showMedia ? updateMetadata() : updater->ClearAll();
    }

    void scheduleUpdate(WinMediaControls *owner)
    {
        if (!controls.Get() || updatePending)
            return;
        updatePending = true;
        QMetaObject::invokeMethod(owner, [this]() {
            updatePending = false;
            if (!controls.Get())
                return;
            const HRESULT hr = update();
            if (FAILED(hr))
            {
                msg_Warn(p_intf, "Failed to update Windows media controls (0x%08lx)",
                         static_cast<unsigned long>(hr));
                shutdown();
            }
        }, Qt::QueuedConnection);
    }

    void shutdown()
    {
        enabled = false;
        auto session = std::move(controls);
        if (handler.Get())
            handler.Get()->detach();
        if (registered)
            session->remove_ButtonPressed(buttonToken);
        registered = false;
        if (session.Get())
            session->put_IsEnabled(false);
        handler.Reset();
        updater.Reset();
        session.Reset();
        if (uninitialize)
            RoUninitialize();
        uninitialize = false;
    }

    qt_intf_t *const p_intf;
    ComPtr<ISystemMediaTransportControls> controls;
    ComPtr<ISystemMediaTransportControlsDisplayUpdater> updater;
    ComPtr<ButtonHandler> handler;
    EventRegistrationToken buttonToken{};
    bool hasInput;
    bool enabled = false;
    bool uninitialize = false;
    bool registered = false;
    bool updatePending = false;
};

WinMediaControls::WinMediaControls(qt_intf_t *p_intf, HWND hwnd)
    : d(std::make_unique<Private>(p_intf))
{
    const HRESULT hr = d->initialize(this, hwnd);
    if (FAILED(hr))
    {
        msg_Warn(p_intf, "Failed to initialize Windows media controls (0x%08lx)",
                 static_cast<unsigned long>(hr));
        d->shutdown();
        return;
    }

    const auto update = [this]() { d->scheduleUpdate(this); };
    connect(THEMIM, &PlayerController::playingStateChanged, this, update);
    connect(THEMIM, &PlayerController::inputChanged, this, [this, update](bool hasInput) {
        d->hasInput = hasInput;
        update();
    });
    connect(THEMIM, &PlayerController::currentMetaChanged, this, update);
    connect(THEMIM, &PlayerController::nameChanged, this, update);
    connect(THEMIM, &PlayerController::hasVideoOutputChanged, this, update);
    connect(THEMPL, &PlaylistController::hasNextChanged, this, update);
    connect(THEMPL, &PlaylistController::hasPrevChanged, this, update);
}

WinMediaControls::~WinMediaControls()
{
    d->shutdown();
}

bool WinMediaControls::isActive() const
{
    return d->enabled;
}

#else

// Older MinGW-w64 headers can still build the interface with focused media keys.
struct WinMediaControls::Private {};

WinMediaControls::WinMediaControls(qt_intf_t *intf, HWND)
{
    msg_Warn(intf, "Windows media controls are unavailable in the build headers");
}

WinMediaControls::~WinMediaControls() = default;

bool WinMediaControls::isActive() const
{
    return false;
}

#endif
