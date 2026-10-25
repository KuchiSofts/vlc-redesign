# Settings: Library

**⋯ → Settings → Library** (<kbd>Ctrl</kbd>+<kbd>P</kbd>; the header reads *Playlists, Folders & Files*). What
happens when you open a file while VLC is already running, how folders are opened, and where VLC gets the titles and
the artwork of your media. The folders the media library indexes are not chosen here but in the library itself: see
[Library: Media folders](Library#media-folders). No row of this page is reserved to Developer mode. See
[how the Settings window works](Settings#how-the-window-works).

## Opening Files

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Only allow one VLC window** | Toggle. Default: off. | A file you open while VLC is running goes to the VLC already open, instead of starting a second copy of VLC, however VLC is started. | `one-instance` |
| **Apply to files opened from the file manager** | Toggle. Default: on. | Sends the files opened from File Explorer to the VLC already running: a double click on an associated file, its *Play* and *Add to queue* commands, and *Play with VLC Redesign* / *Add to VLC Redesign queue* of the right-click menu (they all start VLC with `--started-from-file`). It works on its own: while it is on, those files go to the running VLC even with *Only allow one VLC window* off; a VLC started another way (its shortcut, a command line) still opens its own window. | `one-instance-when-started-from-file` |
| **Add to the playlist instead of replacing it** | Toggle. Default: off. Dimmed while both rows above are off. | When a file goes to the VLC already running, it is added to the end of the play queue and the current item keeps playing, instead of replacing the queue. It decides for a double click and the *Play* command of an associated file; the right-click commands decide for themselves: *Add to queue* and *Add to VLC Redesign queue* always add, *Play with VLC Redesign* always replaces. | `playlist-enqueue` |
| **When opening a folder** | Dropdown: *Only this folder* / *Include subfolders* / *Include and expand subfolders*. Default: *Include subfolders*. | What happens to the subfolders of a folder you open or drop. *Only this folder*: the subfolders do not appear in the play queue. *Include subfolders*: they appear as folders and are expanded the first time they play. *Include and expand subfolders*: all of them are expanded at once, so every file of the tree is in the queue. | `recursive` (`none` / `collapse` / `expand`) |
| **Show hidden files** | Toggle. Default: off. | Off, files and folders whose name starts with a dot (`.thumbnails`) are left out when VLC opens or browses a folder; on, they are listed too. Files only marked hidden in Windows (the hidden attribute) are not concerned. | `show-hiddenfiles` |

## Metadata & Artwork

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Fetch titles and artwork** | Dropdown: *From the file only* / *From the file and the internet*. Default: *From the file only*. | Where VLC looks for the titles, artists, albums and cover art of your media. *From the file and the internet* lets VLC look them up online when the file does not have them. The same setting as **Fetch titles and cover art from the internet** in [Privacy](Settings-Privacy): changing one changes the other. | `metadata-network-access` (From the file and the internet = on) |
| **Use the media library** | Toggle. Default: on. | Starts VLC's media library, which indexes your media folders for the views of the library (Videos, Music…). Off, you browse folders without anything being indexed. The media library starts with VLC, so a change shows *Applies the next time VLC starts* under the row. | `media-library-enabled` |

## Advanced Options

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Read media information automatically** | Toggle. Default: on. | Reads the information of each item added to the play queue (its duration, title and other metadata) as soon as it is added, rather than when it plays. | `auto-preparse` |
| **Ignored file extensions** | Text field: extensions without dots, separated by commas. Default: `m3u,db,nfo,ini,jpg,jpeg,ljpg,gif,png,pgm,pgmyuv,pbm,pam,tga,bmp,pnm,xpm,xcf,pcx,tif,tiff,lbm,sfv,txt,sub,idx,srt,cue,ssa`. | Files with these extensions are not added to the play queue when you open a folder: the playlists, pictures, subtitle and text files that sit next to the media. Subtitle files left out here are still loaded with their video (*Load subtitle files automatically* in [Subtitles](Settings-Subtitles)). | `ignore-filetypes` |

## Good to know

- **Double-clicking files in Explorer starts a new VLC each time?** Check that *Apply to files opened from the file
  manager* is on. Turn on *Only allow one VLC window* as well if you also start VLC from its shortcut and want one
  window in every case.
- **To add files to the queue without changing this page**, right-click them in Explorer and choose *Add to VLC
  Redesign queue*.
- **Images and text files appear in the queue when you open a folder?** Add their extensions to *Ignored file
  extensions*.
- **Titles and cover art from the internet** are off by default; see [Privacy](Settings-Privacy) for what VLC
  fetches online.

**Related pages:** [Library](Library) · [Settings: Privacy](Settings-Privacy) · [Install and update](Install-and-Update) (the file associations and the right-click menu)

---

← [Subtitles](Settings-Subtitles) · [All settings](Settings) · [Network](Settings-Network) →
