# Library

The library is the window VLC Redesign opens on: your videos and music, the media you played last, your playlists
and the network. It is VLC's media library: it reads the folders you add to it and keeps track of what you played,
how far, and how often.

## The sidebar

| Entry | What it shows |
|---|---|
| **Home** | Continue Watching, Listen Again, Frequently Visited Folders, Favorites, New Videos and New Music. |
| **Videos** | The videos of the library. |
| **Music** | The music of the library. Opened, it lists its pages under it: **Artists**, **Albums**, **Tracks**, **Genres**, **Playlists**. |
| **Network** | The folders, this computer, the devices and the network shares. |
| **Playlists** | Every playlist, of videos and of songs. |
| **Recent** | The media played last, videos and songs. |
| **Add item** (+) | Opens files to play (as <kbd>Ctrl</kbd>+<kbd>O</kbd>). |

While the library reads its folders, a progress bar shows under the entries (*Scanning …*, *Indexing Medias (40%)*).
The sidebar can be made wider or narrower by dragging its edge; in a small window, its icons scroll in their column.

## Home

Home lists what is recent, the newest first. Each section shows one row of cards (or five rows in the list view);
its arrow (**›**) opens all of it. A section with nothing in it is not shown.

| Section | What it holds |
|---|---|
| **Continue Watching** | Videos you started, the last played first, with how far you got (an accent line under the card). |
| **Listen Again** | The songs you played, the last played first. |
| **Frequently Visited Folders** | The folders of the media you play, the most played first. A folder opens the folders view of the library. |
| **Favorites** | The media you marked as favorite (the **Favorite** button of the player, or *Favorite* in a right-click menu). |
| **New Videos** | The videos added to the library last. |
| **New Music** | The songs added to the library last. |

The header shows how many media the library has, and **Sort** (*Alphabetic*, *Duration*). An empty library shows
two buttons instead, **Open File** and **Open Disc**.

## Videos

All the videos of the library, as cards or a list. A video's card shows its picture, its title (the file name
without its extension when the video has no title of its own) and its length; a video watched to the end is
dimmed, one stopped partway stays bright with an accent line for how far it went.

- **Group**: in the **Sort** menu, after the orders: *Do not group videos*, *Group by name*, *Group by folder*.
- **Sort**: *Title A-Z* / *Z-A*, *Duration*, *Newest First* / *Oldest First*, *Watched First* / *Unwatched First*,
  and more, by page.
- **Playlists** of videos are a tab of the page too.

## Music

| Page | What it lists |
|---|---|
| **Artists** | The artists, then an artist's albums and songs. |
| **Albums** | The albums; one opens its tracks. *Play as Album* plays it in order. |
| **Tracks** | Every song, as a table: title, album, artist, duration... Sort by *Title*, *Album*, *Artist*, *Track Number*, *Disc Number*, *Duration*. |
| **Genres** | The genres, then the albums and songs of one. |
| **Playlists** | The playlists of songs. |

A folder of songs lists *title - artist*, with **Add**.

## Network

The network page lists **Folders**, **Computer** (the drives of this PC), **Devices** and **Network** (the shares
found on the local network: *No network shares found* when there are none). A place opens as a folder with a path
of breadcrumbs. The header of a folder has **Add to medialibrary** (or **Remove from medialibrary** when it is in it).

To play a stream: **⋯ → Open Network Stream** (<kbd>Ctrl</kbd>+<kbd>N</kbd>): one link per line, several at once,
optionally saved as a playlist. Only links and paths are taken; the dialog says which word is not one.

## Playlists

Every playlist, of videos and of songs.

- **New playlist**: a *Playlist title* (*e.g., Favorite Videos*), a thumbnail of your own (*Choose a thumbnail*),
  and files (*Drag files into the box or click here to select files.*). A playlist can be saved with its title
  alone and filled later.
- A playlist's right-click menu: **Play**, **Shuffle**, **Add to Queue**, **Favorite** / *Remove from Favorites*,
  **Rename**, **Export Playlist** (to a file), **Properties & Metadata** (its name, its media, its length and its
  file), **Add Subtitle Tracks**, **Add Audio Tracks**, **Delete**.
- Opened, a playlist lists its media: **Add files**, **Move up** / **Move down**, **Remove Selected**, and
  *Subtitles…* for a media.
- Removing media and deleting a playlist are confirmed first, while *Show confirmation and error dialogs* is on
  ([Settings: General](Settings-General)).

## Recent

The media played last, videos and songs together, the last played first. What is remembered, and what is never
remembered, is in [Settings: Privacy](Settings-Privacy).

## The right-click menu of a media

A right click on a video, a song, a folder, a file or a playlist opens the menu card of the design. Depending on
what it is:

| Row | What it does |
|---|---|
| **Play**, **Play as Audio**, **Play as Album** | Plays it (a video without its picture, an album in order). |
| **Add to Queue**, **Add Album to Queue** | Adds it to the end of the play queue. |
| **Add to Playlist** | Adds it to a playlist (also *Add to a Video-only Playlist*, *Add to an Audio-only Playlist*). |
| **Favorite** / **Remove from Favorites** | Marks it as a favorite (Home › Favorites). |
| **Mark as Watched** / **Mark as Unwatched** | Changes what Continue Watching and the dimmed cards show. |
| **Show in File Explorer** | Opens its folder, the file selected. |
| **Go to Source** | Opens the folder it was found in. |
| **Information**, **Properties & Metadata** | Its details, and the media information window. |
| **Add Subtitle Tracks**, **Add Audio Tracks** | Attaches subtitle or audio files to it (see [Subtitles](Subtitles#the-subtitles-of-a-language)). |
| **Convert**, **Stream** | VLC's Convert / Save and Stream windows. |
| **Delete File From System** | Deletes the file from the disk, after *Are you sure you want to delete this file?* |

## The header: search, sort and views

Every page has the same header:

- **Back** and **Next** arrows, then the address of the page as breadcrumbs (each part opens its place).
- The count of items (*42 items*).
- **Sort**: the orders of the page, by the names of the design (*Title A-Z*, *Title Z-A*, *Newest First*,
  *Oldest First*, *Album A-Z*, *Artist A-Z*, *Track Number Ascending*, *Disc Number Ascending*, *URI Ascending*,
  *Watched First*, *Unwatched First*...). The library starts **A-Z**; Home's sections keep the newest first. An
  order you pick stays.
- **List/Grid**: *Grid View* or *List View*.
- **Search**: looks through the page as you type (<kbd>/</kbd> or <kbd>Ctrl</kbd>+<kbd>K</kbd> opens it).
  *No media matches the search* when nothing does.
- **⋯** (more): *Open media* — *File…*, *Folder…*, *Disc…*, *Network stream…*, *Capture device…*, *Convert…*,
  *Stream…* — and **Add folder to library…**. In a folder, *Add File* and *Add Folder* come first.

In a narrow window, the count, Sort, the view, Search and ⋯ move under the address.

## Media folders

The library holds the media of the folders you add to it:

- the **media folder** chosen in the first-run setup (*Choose a Media Folder to get started with*);
- **⋯ → Add folder to library…** in the header of any page (*Add folder to media library*);
- **Add to medialibrary** in the header of a folder of the Network page, which **Remove from medialibrary** takes out
  again.

The library reads new files of its folders by itself; **Rescan library** (in the ⋯ menu of the library next to the
video) reads them again. How files open and what VLC reads about them (metadata, artwork) is in
[Settings: Library](Settings-Library). With the media library turned off, Home only offers **Open File** and
**Open Disc**, and the library pages, bookmarks, favorites and playlists are not available.

## The library mini player

While something plays and the library shows, it stays at hand:

- **A video** goes on in a small player over the library (picture in picture): drag it anywhere, click to pause,
  double-click to go back to the player.
- **The mini player card** shows the picture or the cover, a seek bar, the title and the controls; **Show queue**
  opens the queue on the card (drag a row by its handle to move it, **−** removes the selected row, **+** adds files).
- **The player bar** at the bottom: the cover and title (a click opens the player), Shuffle, Previous, Play, Next,
  Repeat, the volume and *Switch Player*. With *Always show the player bar* ([Settings: Interface](Settings-Interface))
  it stays while nothing plays.

See [The player](The-Player#the-mini-player).

## The library next to the video

In the player, **Library** (top bar) or <kbd>L</kbd> opens a compact library over the left of the video: Home with the
sections of Home (*See all* lists a whole section), *Videos* and its folders, *Music* (Songs, Albums, Artists,
Genres) and *Network* (*Add Network Stream*, the Internet services). Each place has its search, **Sort**, the grid or
list **View**, *Play all*, *Add all to queue*, *Up one folder* in a folder, and a **More** menu (*Rescan library*,
*Add folder to library…*, *Add network stream…*, *Create playlist*). **Open in the library** leaves the player for
the same place in the library window.

---

[The player](The-Player) · [Subtitles](Subtitles) · [Settings: Library](Settings-Library) · [Home](Home)
