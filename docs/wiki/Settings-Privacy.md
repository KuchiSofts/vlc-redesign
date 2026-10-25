# Settings: Privacy

**⋯ → Settings → Privacy** (or <kbd>Ctrl</kbd>+<kbd>P</kbd>). What VLC remembers (recent media, resume points), what it fetches from the network, and Last.fm.

This is the only page without an **Advanced Options** bar: every row of the page is listed below. See [how the Settings window works](Settings).

### History
- **Save recently played items**: toggle. Default: on. Keeps the media you play in VLC's history (the recently played items of Home and of the media library). Off, nothing new is added to the history. <sub>VLC option: `save-recentplay`</sub>
- **Remember the playback position**: toggle. Default: on. Lets VLC continue a media where you left off. It is a shortcut for **Continue where you left off** on the [Playback](Settings-Playback) page: off sets it to Never; on sets it to Ask, or keeps Always if that was already chosen. The two rows always agree. <sub>VLC option: `restore-playback-pos` (off = Never, 0)</sub>
- **Never remember items matching**: text field. Default: empty (nothing is left out). A regular expression, matched without regard to case: the items whose address or name matches it are not saved in the history, and their playback position is not kept either. Use it to keep a folder, a website or a kind of file out of the history, for example `private|\.webm$`. <sub>Interface setting: `redesign/recentplay-filter`</sub>
- **Clear history now**: button **Clear**. Asks "Clear playback history?" ("Recently played items will be removed from the media library."), then removes the recently played items of the media library. The settings above stay as they are. *(Disabled, with the tooltip "Media library unavailable", when VLC runs without its media library.)*

### Network
- **Fetch titles and cover art from the internet**: toggle. Default: off. Lets VLC look up titles and cover art of your media on the internet. Off, VLC only uses what it finds in the files themselves. The same setting is **Fetch titles and artwork** on the [Library](Settings-Library) page. <sub>VLC option: `metadata-network-access`</sub>
- **Submit played tracks to last.fm**: toggle. Default: off. Sends the titles of the tracks you play to your Last.fm profile ("scrobbling"). On adds VLC's `audioscrobbler` interface to the additional interfaces; off removes it again (from the additional and from the control interfaces). It starts or stops at once. *(The Last.fm rows appear only in builds that include the Audioscrobbler module.)* <sub>VLC options: `extraintf` (and `control`)</sub>
- **Last.fm username**: text field. Default: empty. The user name of your Last.fm account. <sub>VLC option: `lastfm-username`</sub>
- **Last.fm password**: password field. Default: empty. The password of your Last.fm account. <sub>VLC option: `lastfm-password`</sub>
- **Last.fm submission URL**: text field. Default: `post.audioscrobbler.com`. The address the played tracks are sent to. Change it only to use another service that accepts the Audioscrobbler protocol. <sub>VLC option: `scrobbler-url`</sub>

The three Last.fm fields stay empty while **Submit played tracks to last.fm** is off (a field shows its value while you edit it). Changing the account while submitting is on restarts the Last.fm interface with the new values.

### Options of VLC behind these settings

| Row | VLC option |
|---|---|
| Save recently played items | `save-recentplay` |
| Remember the playback position | `restore-playback-pos` |
| Never remember items matching | `redesign/recentplay-filter` (interface setting) |
| Clear history now | *(an action: no option)* |
| Fetch titles and cover art from the internet | `metadata-network-access` |
| Submit played tracks to last.fm | `extraintf` / `control` (the `audioscrobbler` interface) |
| Last.fm username | `lastfm-username` |
| Last.fm password | `lastfm-password` |
| Last.fm submission URL | `scrobbler-url` |

---

← [Interface](Settings-Interface) · [All settings](Settings) · [Controls](Settings-Controls) →
