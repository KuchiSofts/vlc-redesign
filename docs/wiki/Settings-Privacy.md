# Settings: Privacy

**⋯ → Settings → Privacy** (<kbd>Ctrl</kbd>+<kbd>P</kbd>; the header reads *Privacy Settings*). What VLC remembers
about what you play, what it looks up on the internet, and Last.fm. This is the only page without an **Advanced
Options** bar: every row is listed below. See [how the Settings window works](Settings#how-the-window-works).

## History

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Save recently played items** | Toggle. Default: on. | Adds what you play to VLC's history: the recently played items of Home and of the media library, and the playback position of each media. Off: nothing new is recorded, and no new resume point is saved either. | `save-recentplay` |
| **Remember the playback position** | Toggle. Default: on. | Lets VLC continue a media where you left off. It is a shortcut for **Continue where you left off** on [Playback](Settings-Playback#startup): off sets it to *Never*; on sets it to *Ask*, or keeps *Always* if that was chosen. The two rows always agree. | `restore-playback-pos` (off = 0) |
| **Never remember items matching** | Text field. Default: empty (nothing is left out). | A regular expression, matched without regard to case against each item's address and name. Matching items are not saved in the history, and their position is not kept. | `redesign/recentplay-filter` (interface setting) |
| **Clear history now** | Button **Clear**. | Asks *Clear playback history?* (*Recently played items will be removed from the media library.*), then removes them. The settings above stay as they are. Disabled, with the tooltip *Media library unavailable*, when VLC runs without its media library. | |

### Examples for *Never remember items matching*

| Pattern | Leaves out |
|---|---|
| `private` | Every item whose path or name contains *private* (any case). |
| `\.webm$` | WebM files. |
| `^https?://` | Everything played from a website. |
| `D:/Private/` | Everything in the folder `D:\Private` (addresses use `/`, and a space is written `%20`). |
| `private\|\.webm$` | Both of the first two. |

The address of a local file is its `file:///` URL. An invalid expression is ignored, so everything is recorded as
usual.

## Network

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Fetch titles and cover art from the internet** | Toggle. Default: off. | Lets VLC look up titles and cover art of your media on the internet. Off: VLC only uses what is in the files and folders themselves. The same setting is **Fetch titles and artwork** on [Library](Settings-Library) (*From the file only* / *From the file and the internet*). | `metadata-network-access` |
| **Submit played tracks to last.fm** | Toggle. Default: off. | Sends the title of each track you play to your Last.fm profile ("scrobbling"). On adds VLC's `audioscrobbler` interface to the additional interfaces; off removes it (from the control interfaces too). It starts or stops when the window closes. | `extraintf` / `control` |
| **Last.fm username** | Text field. Default: empty. | The user name of your Last.fm account. | `lastfm-username` |
| **Last.fm password** | Password field. Default: empty. | The password of your Last.fm account. | `lastfm-password` |
| **Last.fm submission URL** | Text field. Default: `post.audioscrobbler.com`. | The server the played tracks are sent to. Change it only for another service that speaks the Audioscrobbler protocol. | `scrobbler-url` |

The four Last.fm rows appear only in builds that include the Audioscrobbler module. The three fields stay empty
while **Submit played tracks to last.fm** is off; a field shows its value while you edit it. Changing the account
while submitting is on restarts the Last.fm connection with the new values.

## Good to know

- **Resume points need the history.** The position of a media is stored with its history entry in the media
  library. With **Save recently played items** off, or with the media library off
  ([Library](Settings-Library)), no new position is saved, whatever **Remember the playback position** says.
- **Clear history now** empties the recently played list; it does not change any setting. To stop recording,
  turn **Save recently played items** off.
- **The filter works per item when it is added** to the play queue: changing it does not remove items already in
  the history (use **Clear history now** for those).

---

← [Interface](Settings-Interface) · [All settings](Settings) · [Controls](Settings-Controls) →
