# Settings: Controls

**⋯ → Settings → Controls** (<kbd>Ctrl</kbd>+<kbd>P</kbd>; the header reads *Mouse, Keyboard & Remote Controls*).
Keyboard shortcuts, the lengths of the skip jumps, what the mouse does on the video, and the web remote. All rows
apply when the Settings window closes, without a restart. See
[how the Settings window works](Settings#how-the-window-works).

## Keyboard Shortcuts

| Row | Control | What it does |
|---|---|---|
| **Defaults** | Button **Edit Keyboard Shortcuts** | Opens the Keyboard Shortcuts window (also **⋯ → Keyboard Shortcuts**). |
| **Reset all shortcuts** | Button **Reset** | Opens the Keyboard Shortcuts window and asks *Reset all shortcuts?* (*Local and global shortcuts will return to VLC defaults when you save. Cancel keeps your current settings.*). **Reset all** puts the default keys back once you **Save** the window. |

**The Keyboard Shortcuts window** lists every action of VLC with two columns: **Hotkey** (works while VLC is the
active window) and **Global Hotkey** (works from any application). A search box (*Search Keyboard Shortcuts...*)
has a filter: *All Fields* / *Action* / *Hotkey* / *Global Hotkey*.

- Click a key field and press a key combination to add it; <kbd>Esc</kbd> cancels.
- A key already used by another action asks *This key is assigned to … Replace that assignment?*.
- **Save** keeps the changes; they apply at once, global hotkeys included.

See [Keyboard shortcuts](Keyboard-Shortcuts) for the default keys.

## Skip Amounts

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Very short skip** | Number field, in seconds. Default: 3 s. | The length of the *very short jump* forward and backward actions. | `extrashort-jump-size` |
| **Short skip** | Number field, in seconds. Default: 10 s. | The *short jump*. Also used by the **Jump forward / Jump backward** menu items, the skip buttons of the audio player, the arrow keys on the seek bar and on the library's mini player. | `short-jump-size` |
| **Medium skip** | Number field, in seconds. Default: 60 s. | The *medium jump*. | `medium-jump-size` |
| **Long skip** | Number field, in seconds. Default: 300 s (5 minutes). | The *long jump*. | `long-jump-size` |

Which keys do each jump is set in **Edit Keyboard Shortcuts** (the *jump* actions).

## Mouse

These rows set what the mouse does on the video (and on the card shown while casting to a TV). See
[Mouse on the video](The-Player#mouse-on-the-video).

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Single click on the video** | Dropdown: *Play / Pause* / *Nothing*. Default: *Play / Pause*. | What a click on the video does. *Nothing*: a click never pauses, handy if you often click the video by accident. Playback toggles when the button is released. A profile that still has VLC's older value *pause-on-released* shows a third choice, *Play / Pause on release*, which behaves the same. | `mouse-pause-policy` (`auto` / `disabled`) |
| **Double click on the video** | Dropdown: *Nothing* / *Toggle fullscreen*. Default: *Toggle fullscreen*. | What a double click does. With *Toggle fullscreen*, a single click waits for Windows' double-click time before it plays or pauses, so a double click does not also pause. With *Nothing*, a click acts at once. | `redesign/double-click-action` (interface setting, 0 / 1) |
| **Scroll wheel** | Dropdown: *Nothing* / *Volume* / *Seek*. Default: *Volume*. | What the vertical mouse wheel does over the video. | `hotkeys-y-wheel-mode` (−1 / 0 / 2) |
| **Horizontal scroll** | Dropdown: *Nothing* / *Volume* / *Seek*. Default: *Seek*. | What a horizontal scroll (a tilting wheel, a touchpad swipe) does over the video. | `hotkeys-x-wheel-mode` (−1 / 0 / 2) |
| **Reverse scroll direction** | Toggle. Default: off. | The wheels set to *Seek* go backwards when turned forwards. Only seeking is reversed, never the volume. While neither wheel seeks, the toggle keeps its state and applies as soon as one is set to *Seek*. | both wheel options (*Seek* stored as 3 instead of 2) |

## Remotes

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Web remote control** | Toggle. Default: off. | Lets a web browser on this computer or on another device of your network control VLC through VLC's web interface. It starts or stops when the window closes. While it is on without a password, a warning appears under it: *Set a web remote password in the Advanced Options: the remote stays locked without one.* | `http` in `extraintf` |
| **Port** | Number field. Default: 8080. | The TCP port the web remote listens on: open `http://<address of this computer>:<port>` in a browser. The field is empty while **Web remote control** is off. A new port restarts the web remote on it. Ports below 1025 are usually restricted by the operating system. | `http-port` |
| **Scroll wheel**, **Horizontal scroll**, **Reverse scroll direction** | | The three rows of *Mouse*, shown again here as the design places them. They are the same settings: changing one changes the other. | |

## Advanced Options

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Handle mouse clicks in the video** | Toggle. Default: on. | Lets VLC's own video window pass the mouse (clicks, movement, the pointer hiding) to the player. It is read by the separate video window used when **Play video inside main window** ([Interface](Settings-Interface#window)) is off; video inside the main window handles the mouse regardless. | `mouse-events` |
| **Web remote password** | Password field. Default: empty. | The password the browser asks for. Until one is set, the web remote only shows a page asking for one. Changing it restarts the web remote. | `http-password` |
| **Web remote address** | Text field. Default: empty (every network interface). | Restricts the web remote to one network interface: an IP address (`192.168.1.20`, `127.0.0.1`, `::1`) or a host name. `localhost` or `127.0.0.1` keeps it on this computer only. | `http-host` |
| **Ignore the volume keys of the keyboard** | Toggle. Default: on. | On: the volume up, volume down and mute keys of your keyboard always change the system volume. Off: they change VLC's volume while VLC is the active window, and the system volume otherwise. Windows only. | `qt-disable-volume-keys` |

## Good to know

- **Setting up the web remote:** turn on **Web remote control**, open Advanced Options and set a **Web remote
  password**, then close Settings. From a phone on the same network, open `http://<PC address>:8080`, leave the
  user name empty and type the password. Windows Firewall may ask to allow VLC the first time.
- **Seeking with the wheel:** set **Scroll wheel** to *Seek*; with a touchpad, **Reverse scroll direction** often
  feels more natural.
- **Accidental pauses** when clicking to focus the window: set **Single click on the video** to *Nothing*.

---

← [Privacy](Settings-Privacy) · [All settings](Settings) · [Playback](Settings-Playback) →
