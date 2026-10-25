# Settings: Extensions

**⋯ → Settings → Extensions** (or <kbd>Ctrl</kbd>+<kbd>P</kbd>). Add-ons (extensions, playlists parsers, skins) and the list of VLC's plugins. The page is titled *Add-ons & Scripts*.

Rows under **Advanced Options** are in the collapsed bar at the bottom of the page. This page has no rows reserved to Developer mode. See [how the Settings window works](Settings).

### Add-ons
- **Add-on list**: a list of VLC's add-ons of every kind (extensions, playlist parsers, service discovery scripts, skins, interfaces, metadata scripts…), each with its name, its summary (or its author) and its state on the right: *Installed*, *Installing…*, *Removing…* or *Broken*. Click a row to select it. The list loads the first time the page shows (a spinner turns while it loads); at first it holds the add-ons known on this computer. An add-on installed by hand (copied into VLC's folders) cannot be managed here: hovering it says "This add-on was installed manually and cannot be managed here."
  - **Search** field: filters the list as you type.
  - **Find more extensions**: button. Asks VLC's online add-ons repository for the add-ons it offers and adds them to the list.
  - **Add**: button. Installs the selected add-on. *(Works on an add-on that is not installed; otherwise hovering the button says "Select an add-on that is not installed".)*
  - **Remove**: button. Uninstalls the selected add-on. *(Works on an installed add-on; otherwise hovering the button says "Select an installed add-on".)*
- **Load add-ons on startup**: toggle. Default: on. Loads VLC's extensions automatically, so they are listed in the extensions menu. Off, the extensions are not loaded until something else loads them. <sub>VLC option: `qt-autoload-extensions`</sub>

### Advanced Options
- **Show installed add-ons only**: toggle. Default: off. Shows only the installed add-ons in the list above. It only filters the list of the page: it is not saved as a setting.
- **Plugins and extensions**: button **Open**. Opens VLC's classic *Plugins and extensions* window, which lists the plugins (modules) of this VLC and the extensions.

### Options of VLC behind these settings

| Row | VLC option |
|---|---|
| Load add-ons on startup | `qt-autoload-extensions` |
| Add-on list, Show installed add-ons only, Plugins and extensions | none (they act on the add-ons and plugins directly) |

---

← [Recording](Settings-Recording) · [All settings](Settings)
