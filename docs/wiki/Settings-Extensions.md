# Settings: Extensions

**⋯ → Settings → Extensions** (<kbd>Ctrl</kbd>+<kbd>P</kbd>; the header reads *Add-ons & Scripts*). VLC's add-ons
(extensions, playlist parsers, service discovery scripts, skins…) and the list of VLC's plugins. No row of this page
is reserved to Developer mode. See [how the Settings window works](Settings#how-the-window-works).

## Add-ons

The section is the list of the design, with a search field and three buttons under it, then one setting.

| Control | What it shows / default | What it does |
|---|---|---|
| **Add-on list** | Every add-on VLC knows, of every kind (extensions, playlist parsers, service discovery scripts, skins, interfaces, metadata scripts…), each with its name, its summary (or its author) and its state on the right: *Installed*, *Installing…*, *Removing…* or *Broken*. | Click a row to select it. The list loads the first time the page shows (a spinner turns while it loads) and holds the add-ons known on this computer until *Find more extensions* asks online. An add-on installed by hand (copied into VLC's folders) cannot be managed here: hovering it says *This add-on was installed manually and cannot be managed here.* |
| **Search** | Text field. | Filters the list as you type. |
| **Find more extensions** | Button. | Asks VLC's online add-ons repository for what it offers and adds it to the list. |
| **Add** | Button. | Installs the selected add-on. It works on an add-on that is not installed; otherwise hovering it says *Select an add-on that is not installed*. |
| **Remove** | Button. | Uninstalls the selected add-on. It works on an installed add-on; otherwise hovering it says *Select an installed add-on*. |

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Load add-ons on startup** | Toggle. Default: on. | Loads VLC's Lua extensions when they are first needed, so the extensions menu lists them. Off, that menu lists none until the *Plugins and extensions* window (or the VLsub subtitle search) loads them. | `qt-autoload-extensions` |

## Advanced Options

| Control | Default | What it does |
|---|---|---|
| **Show installed add-ons only** | Toggle. Default: off. | Shows only the installed add-ons in the list above. It only filters the list of the page and is not saved as a setting. |
| **Plugins and extensions** | Button **Open**. | Opens the *Plugins and extensions* dialog, drawn in the redesign's look since 0.4.0. **Plugins** lists the modules of this VLC, searchable by name or capability and sorted by name, capability or score (a module's description shows on hover); **Extensions** lists the Lua extensions with their version, author and link, with **Reload extensions**. |

## Good to know

- **After copying an extension (`.lua`) into VLC's `lua\extensions` folder by hand,** open *Plugins and extensions*
  and click **Reload extensions** (or restart VLC).
- **After adding or removing plugin files (`.dll`) by hand,** use *Rescan plugins* in
  [Performance](Settings-Performance) so that VLC finds them.
- **Find more extensions** needs an internet connection; the add-ons it lists come from VLC's repository, not from
  VLC Redesign.

**Related pages:** [Settings: Performance](Settings-Performance) (Rescan plugins) · [Settings: Interface](Settings-Interface) (VLC's classic skins)

---

← [Recording](Settings-Recording) · [All settings](Settings)
