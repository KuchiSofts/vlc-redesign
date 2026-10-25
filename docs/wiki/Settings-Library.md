# Settings: Library

**⋯ → Settings → Library** (or <kbd>Ctrl</kbd>+<kbd>P</kbd>). How files open and what VLC reads about them (metadata and artwork). The page is titled *Playlists, Folders & Files*.

Rows under **Advanced Options** are in the collapsed bar at the bottom of the page. This page has no rows reserved to Developer mode. See [how the Settings window works](Settings).

### Opening Files
- **Only allow one VLC window**: toggle. Default: off. A file you open while VLC is running goes to the VLC already open instead of starting a second copy of VLC. Useful when VLC opens your media files from the file manager. <sub>VLC option: `one-instance`</sub>
- **Apply to files opened from the file manager**: toggle. Default: on. Sends the files opened from the file manager (the file associations start VLC with `--started-from-file`) to the VLC already running. It works on its own: while it is on, files opened from the file manager go to the running VLC even with *Only allow one VLC window* off. <sub>VLC option: `one-instance-when-started-from-file`</sub>
- **Add to the playlist instead of replacing it**: toggle. Default: off. When a file goes to the VLC already running, it is added to the play queue and the current item keeps playing, instead of replacing the queue. The *Enqueue* command of the file associations always adds, whatever this row says. It applies when either *Only allow one VLC window* or *Apply to files opened from the file manager* is on. *(Dimmed while both are off.)* <sub>VLC option: `playlist-enqueue`</sub>
- **When opening a folder**: dropdown: Only this folder / Include subfolders / Include and expand subfolders. Default: Include subfolders. What happens to the subfolders of a folder you open. *Only this folder*: the subfolders do not appear in the play queue. *Include subfolders*: they appear, and are expanded the first time they play. *Include and expand subfolders*: all of them are expanded at once. <sub>VLC option: `recursive` (`none` / `collapse` / `expand`)</sub>
- **Show hidden files**: toggle. Default: off. Off, files whose name starts with a dot are left out when VLC opens or browses a folder. Turn it on to list them too. <sub>VLC option: `show-hiddenfiles`</sub>

### Metadata & Artwork
- **Fetch titles and artwork**: dropdown: From the file only / From the file and the internet. Default: From the file only. Where VLC looks for the titles, artists and cover art of your media. *From the file and the internet* lets VLC look them up online when the file does not have them. The same setting as **Fetch titles and cover art from the internet** in [Privacy](Settings-Privacy). <sub>VLC option: `metadata-network-access` (From the file and the internet = on)</sub>
- **Use the media library**: toggle. Default: on. *Applies after a restart.* Starts VLC's media library, which indexes your media folders for the Videos and Music views. Turn it off to browse without indexing your media. After a change, the note "Applies the next time VLC starts" appears. <sub>VLC option: `media-library-enabled`</sub>

### Advanced Options
- **Read media information automatically**: toggle. Default: on. Reads the information of each item added to the play queue (its duration, title and other metadata) as soon as it is added, rather than when it plays. <sub>VLC option: `auto-preparse`</sub>
- **Ignored file extensions**: text field, a comma-separated list of extensions. Default: `m3u,db,nfo,ini,jpg,jpeg,ljpg,gif,png,pgm,pgmyuv,pbm,pam,tga,bmp,pnm,xpm,xcf,pcx,tif,tiff,lbm,sfv,txt,sub,idx,srt,cue,ssa`. Files with these extensions are not added to the play queue when you open a folder. Useful for folders that hold playlists, pictures or subtitle files next to the media. <sub>VLC option: `ignore-filetypes`</sub>

### Options of VLC behind these settings

| Row | VLC option |
|---|---|
| Only allow one VLC window | `one-instance` |
| Apply to files opened from the file manager | `one-instance-when-started-from-file` |
| Add to the playlist instead of replacing it | `playlist-enqueue` |
| When opening a folder | `recursive` |
| Show hidden files | `show-hiddenfiles` |
| Fetch titles and artwork | `metadata-network-access` |
| Use the media library | `media-library-enabled` |
| Read media information automatically | `auto-preparse` |
| Ignored file extensions | `ignore-filetypes` |

---

← [Subtitles](Settings-Subtitles) · [All settings](Settings) · [Network](Settings-Network) →
