# TUI Interface Guide

See also:

- `docs/README.md` for the documentation index
- `man/termus.txt` for the full user reference

## Scope

This document describes the current ncursesw-based interface as implemented in
`src/ui/`. It is intended for contributors who need to understand how the
screen is divided, how the three views and popup overlays behave, and where to
make UI changes.

It is not a full command reference. Keybindings, colors, and visible formats
are configurable, so treat the layout and interaction model here as the stable
part and the exact text rendering as user-defined.

For new UI source files, include `src/ui/curses_compat.h` as the single entry
point for ncursesw headers and the small Sun/Cygwin `termios.h` portability
hook. Do not add new ad hoc `<curses.h>` or `<ncurses.h>` conditionals in
individual files.

## Screen Layout

termus reserves the last three terminal rows for playback and input state. The
rest of the screen is the active view.

```text
row 0            current view title bar or pane headers
rows 1..LINES-4  view body
row LINES-4      optional live filter badge at right edge in library views
row LINES-3      title line for current track / stream
row LINES-2      status line with playback state and progress indication
row LINES-1      command line, search prompt, messages, and yes/no queries
```

Important details from `src/ui/ui_curses.c`:

- Row `LINES-3` is rendered by `do_update_titleline()`.
- Row `LINES-2` is rendered by `do_update_statusline()`.
- Row `LINES-1` is rendered by `do_update_commandline()`.
- The optional `filtered: ...` badge is drawn by `update_filterline()` only in
  library view.

### Stable Part Names

Use these names when discussing the layout:

- `library_header`: the top bar of the library view.
- `library_list`: the flat track list in library view.
- `library_filter_badge`: the optional `filtered: ...` badge.
- `playlist_sidebar`: the optional left playlist-name pane.
- `playlist_tracks`: the right track pane in playlist view.
- `queue_header`: the top bar of the play queue view.
- `queue_list`: the flat play queue list.
- `now_playing_line`: row `LINES-3`.
- `status_line`: row `LINES-2`.
- `command_line`: row `LINES-1`.

### Bottom Chrome

The bottom three rows are shared across all views:

- Title line: the currently playing track or stream title. By default it uses
  just artist and title, and it also updates the terminal title when
  `set_term_title` is enabled.
- Status line: playback status, position or duration, speed, play source,
  volume, and toggle flags such as continue, follow, repeat, and shuffle.
- Command line: empty in normal mode; reused for `:` commands, `/` or `?`
  searches, transient info and error messages, and confirmation prompts.

The status line can also render a progress indicator according to the
`progress_bar` option (`disabled`, `line`, `shuttle`, `color`, or
`color-shuttle`).

## Views

termus has three views. `src/app/options_ui_state.h` defines the stable
view order and `src/ui/ui_curses.c:set_view()` switches the active searchable
model for each one.

### 1. Library View

This is the main view displaying all tracks in the library as one flat list. The heading shows
track count, total duration, mark count, and current sort expression. The row
content is configurable through `library_columns` or the lower-level
`format_playlist`.

```text
+------------------------------------------------------------------+
| Library (123 tracks, 08:45:12) - sorted by: artist album track   |
+------------------------------------------------------------------+
| Artist 1                 Track A                          03:45   |
| Artist 1                 Track B                          04:11   |
| Artist 1                 Track C                          05:02   |
| Artist 2                 Track D                          03:18   |
| ...                                                              |
|                                                                  |
|                                         (Row LINES-4)  filtered: |
+------------------------------------------------------------------+
```

### 2. Playlist View

The playlist view can be either split or single-pane:

- Optional left panel: playlist names.
- Right panel: tracks in the visible playlist.
- If the playlist panel is hidden, the track list expands to full width.

```text
+-----------------------+------------------------------------------+
| Playlists             | Playlist: Rock Classics                  |
+-----------------------+------------------------------------------+
| Favorites             |   1. Led Zeppelin - Stairway to Heaven   |
| Rock Classics         |   2. Pink Floyd - Time                   |
| Jazz Mix              |   3. Queen - Bohemian Rhapsody           |
| Workout               |                                          |
|                       |                                          |
|                       |                                          |
+-----------------------+------------------------------------------+
```

The visible header is formatted with `format_heading_playlist`.

### 3. Play Queue View

This is a flat editable list of upcoming tracks. Queue items take priority over
library or playlist playback until the queue is drained.

```text
+------------------------------------------------------------------+
| Queue                                                            |
+------------------------------------------------------------------+
|   1. Artist X - Upcoming Track 1                                 |
|   2. Artist Y - Upcoming Track 2                                 |
|   3. Artist Z - Upcoming Track 3                                 |
|                                                                  |
|                                                                  |
|                                                                  |
+------------------------------------------------------------------+
```

## Popups

Popups are floating modal overlays drawn on top of the current view using a
dedicated `newwin()` ncurses window. They are managed in `src/ui/popup.c`
and rendered via `popup_draw()`, which is called from `post_update()` after
every `refresh()` so the popup always appears on top.

`src/ui/popup.h` defines `enum popup_type` (POPUP_NONE, POPUP_FILTERS,
POPUP_TRACK_INFO) and the public API: `popup_open`, `popup_close`,
`popup_draw`, `popup_resize`, `popup_is_open`, `popup_get_type`.

### Filters Popup (key F)

This is a modal text input for the library filter expression. It reuses the
existing live-filter machinery, so the library list updates while the user
types. `Enter` keeps the new filter, `Esc` cancels and restores the previous
value, and an empty input clears the filter back to the unfiltered list.

```text
+----------------------------------------+
| Filter                                 |
| Enter a library filter. Empty input    |
| restores all items.                    |
| The list updates while you type.       |
| > artist="Massive Attack"&genre="Trip" |
| Enter apply  Esc cancel                |
+----------------------------------------+
```

### Track Info Popup (key I)

Shows metadata for the currently playing track read from `player_info.ti`.
It has two tabs switchable with `Left`/`Right`, `Tab`, or `1`/`2`:

- **1: Metadata** — title, artist, album, genre, year, track number, duration,
  bitrate, and codec.
- **2: File** — path, codec, profile, bitrate, sample rate, bit depth,
  channels, file size, and last-modified date.

```text
+----------------------------------------------+
| Track Info                                   |
|  1:Metadata   2:File                         |
| Title:        Echoes                         |
| Artist:       Pink Floyd                     |
| Album:        Meddle                         |
| Duration:     23:31                          |
| Bitrate:      320 kbps                       |
| Codec:        mp3                            |
| Left/Right or 1/2 to switch tabs, Esc close  |
+----------------------------------------------+
```

`Esc`, `q`, or `Enter` close the popup.

### Input Routing

When a popup is open, `normal_mode_ch`, `normal_mode_key`, and
`handle_escape()` in `src/ui/keys.c` and `src/ui/ui_curses.c` hand input to
`src/ui/popup.c` first. The filter popup edits its own text buffer and applies
changes directly through `filters_set_live()`.

### Terminal Resize

`popup_resize()` is called from `update_window_size()`. It destroys and
recreates the popup WINDOW at the new centered position.

## Input Modes

The TUI has three input modes defined in `src/ui/ui_curses.h`:

- `NORMAL_MODE`: the default mode for navigation and bound commands.
- `COMMAND_MODE`: entered with `:`, using the command line for command editing
  and tab completion.
- `SEARCH_MODE`: entered with `/` for forward search or `?` for backward
  search.

The active mode changes what the last row means:

- In normal mode it is usually blank unless there is a message.
- In command mode it shows `:` plus the editable command buffer.
- In search mode it shows `/` or `?` plus the search buffer.

Search uses a per-view `searchable` backend selected by `set_view()`. The same
`n` and `N` follow-up behavior works across all views.

## Core Interaction Model

Most visible actions are the same command system exposed through different
entry points:

- keybindings dispatch commands through `src/ui/keys.c`
- `:` commands are parsed in `src/ui/command_mode.c`
- search editing lives in `src/ui/search_mode.c`
- remote control uses the same command handlers through IPC via `termusc`

That shared model explains why UI actions often mirror a command exactly. A few
important examples:

- `1` to `3`: switch views; `F` opens the filters popup; `I` opens track info
- `Enter`: activate the selected item for the current view, or apply the
  current filter in the popup
- `Space`: expand, mark, toggle, or select depending on the view
- `a`, `y`, `e`, `E`: copy the selected or marked item to another collection
- `:`: open command mode
- `/` and `?`: open search mode
- `Ctrl+n` and `Ctrl+p`: find next/previous search result
- `n` and `p`: next/previous track
- `s`: stop playback
- `c`: pause/unpause playback

The exact default bindings are documented in `man/termus.txt`.

## Implementation Map

For UI work, these files are the main entry points:

- `src/ui/ui_curses.c`: screen layout, resize handling, redraw scheduling,
  view switching, title/status/command lines, and the main event loop
- `src/ui/popup.c` / `src/ui/popup.h`: floating modal popup system
- `src/ui/window.c`: generic scrolling window abstraction
- `src/ui/editable_view.c`: adapter for editable list views
- `src/ui/keys.c`: key contexts, bindings, and dispatch
- `src/ui/command_mode.c`: command-line parser and handlers
- `src/ui/search_mode.c`: interactive search
- `src/ui/options_display.c`: default UI format strings and color option names

If you need to change what the user sees, start in `ui_curses.c`. If you need
to change what a row represents or how it scrolls, the next place is usually
`window.c`, `editable_view.c`, or the view-specific module. Popup content and
layout lives in `popup.c`; adding a new popup type means adding an entry to
`enum popup_type` and a `draw_*` function there.
