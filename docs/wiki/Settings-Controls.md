# Settings: Controls

**⋯ → Settings → Controls** (or <kbd>Ctrl</kbd>+<kbd>P</kbd>). Keyboard shortcuts, the jump lengths of the skip buttons, the mouse wheel and remotes.

Rows under **Advanced Options** are in the collapsed bar at the bottom of the page. Rows marked *Developer mode only* appear there once **Developer mode** is on ([General](Settings-General)). See [how the Settings window works](Settings).

### Keyboard Shortcuts
- **Defaults**: button **Edit Keyboard Shortcuts**. Opens the Keyboard Shortcuts window: every action of VLC with its **Hotkey** and its **Global Hotkey** (a global hotkey works while VLC is not the active window), a search box (*Search Keyboard Shortcuts...*) with a filter (All Fields / Action / Hotkey / Global Hotkey). Click a key field and press a key combination to add it (<kbd>Esc</kbd> cancels); a key already used by another action asks before it is moved ("This key is assigned to … Replace that assignment?"). **Save** keeps the changes.
- **Reset all shortcuts**: button **Reset**. Opens the Keyboard Shortcuts window and asks "Reset all shortcuts?" ("Local and global shortcuts will return to VLC defaults when you save. Cancel keeps your current settings."). **Reset all** puts the default keys back once you save the window.

### Skip Amounts
- **Very short skip**: number field, in seconds. Default: 3 s. Length of the very short jump (the very short jump forward / backward shortcuts). <sub>VLC option: `extrashort-jump-size`</sub>
- **Short skip**: number field, in seconds. Default: 10 s. Length of the short jump. It is also the jump of the **Jump forward / Jump backward** menu items, of the skip buttons of the audio player and of the arrow keys on the mini player. <sub>VLC option: `short-jump-size`</sub>
- **Medium skip**: number field, in seconds. Default: 60 s. Length of the medium jump. <sub>VLC option: `medium-jump-size`</sub>
- **Long skip**: number field, in seconds. Default: 300 s (5 minutes). Length of the long jump. <sub>VLC option: `long-jump-size`</sub>

Which keys do each jump is set in **Edit Keyboard Shortcuts** (the *jump* actions).

### Mouse
- **Single click on the video**: dropdown: Play / Pause / Nothing. Default: Play / Pause. What a click on the video does. With Nothing, a click on the video does not pause it (useful if you often click the video by accident). The pause toggles when the button is released. The older choice *Play / Pause on release* did the same as Play / Pause; it only shows for a profile that already has it. <sub>VLC option: `mouse-pause-policy` (`auto` / `disabled` / `pause-on-released`)</sub>
- **Double click on the video**: dropdown: Nothing / Toggle fullscreen. Default: Toggle fullscreen. What a double click on the video does. With Toggle fullscreen, a single click waits for the double-click time of Windows before it plays or pauses (so that a double click does not also pause). <sub>Interface setting: `redesign/double-click-action`</sub>
- **Scroll wheel**: dropdown: Nothing / Volume / Seek. Default: Volume. What the vertical mouse wheel does over the video. <sub>VLC option: `hotkeys-y-wheel-mode` (Nothing = −1, Volume = 0, Seek = 2, reversed Seek = 3)</sub>
- **Horizontal scroll**: dropdown: Nothing / Volume / Seek. Default: Seek. What the horizontal wheel (a tilting wheel, a horizontal scroll) does over the video. <sub>VLC option: `hotkeys-x-wheel-mode`</sub>
- **Reverse scroll direction**: toggle. Default: off. The wheels set to Seek go backwards when turned forwards. Only seeking is reversed, not the volume. While neither wheel seeks, the toggle keeps its state and applies as soon as one is set to Seek. <sub>VLC options: `hotkeys-y-wheel-mode` and `hotkeys-x-wheel-mode` (Seek stored as 3 instead of 2)</sub>

### Remotes
- **Web remote control**: toggle. Default: off. Lets a web browser (on this computer or another device of the network) control VLC through VLC's web interface. It starts or stops at once. On adds the `http` interface to VLC's additional interfaces. While it is on without a password, the note "Set a web remote password in the Advanced Options: the remote stays locked without one." appears: the web interface only asks for a password until one is set. <sub>VLC option: `extraintf` (with `http`)</sub>
- **Port**: number field (1–65535). Default: 8080. The TCP port the web remote listens on: open `http://<address of the computer>:<port>` in a browser. Ports below 1025 are usually restricted by the operating system. The field is empty while **Web remote control** is off. Changing it restarts the web remote on the new port. <sub>VLC option: `http-port`</sub>
- **Scroll wheel**, **Horizontal scroll**, **Reverse scroll direction**: the three rows of Mouse, shown again here as the design places them. They are the same settings: changing one changes the other.

### Advanced Options
- **Handle mouse clicks in the video**: toggle. Default: on. Lets VLC react to mouse clicks on the video. <sub>VLC option: `mouse-events`</sub>
- **Web remote password**: password field. Default: empty. A single password that restricts access to the web remote: the browser asks for it. The web remote stays locked until one is set. <sub>VLC option: `http-password`</sub>
- **Web remote address**: text field. Default: empty, which listens on every local IP address. Give an IP address (for example `127.0.0.1` or `::1`) or a host name (`localhost`) to accept connections from one network interface only; `localhost` keeps the remote on this computer. <sub>VLC option: `http-host`</sub>
- **Ignore the volume keys of the keyboard**: toggle. Default: on. On, the volume up, volume down and mute keys of your keyboard always change the volume of the system. Off, they change VLC's volume while VLC is the active window, and the system volume otherwise. *(Windows only.)* <sub>VLC option: `qt-disable-volume-keys`</sub>

### Options of VLC behind these settings

| Row | VLC option |
|---|---|
| Edit Keyboard Shortcuts, Reset all shortcuts | the `key-…` and `global-key-…` options |
| Very short skip | `extrashort-jump-size` |
| Short skip | `short-jump-size` |
| Medium skip | `medium-jump-size` |
| Long skip | `long-jump-size` |
| Single click on the video | `mouse-pause-policy` |
| Double click on the video | `redesign/double-click-action` (interface setting) |
| Scroll wheel | `hotkeys-y-wheel-mode` |
| Horizontal scroll | `hotkeys-x-wheel-mode` |
| Reverse scroll direction | `hotkeys-y-wheel-mode`, `hotkeys-x-wheel-mode` (value 3) |
| Web remote control | `extraintf` (`http`) |
| Port | `http-port` |
| Handle mouse clicks in the video | `mouse-events` |
| Web remote password | `http-password` |
| Web remote address | `http-host` |
| Ignore the volume keys of the keyboard | `qt-disable-volume-keys` |

---

← [Privacy](Settings-Privacy) · [All settings](Settings) · [Playback](Settings-Playback) →
