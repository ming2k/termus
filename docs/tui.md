# TUI Interface Guide

See also:

- `docs/README.md` for the documentation index
- `man/termus.txt` for the full user reference
- `man/termus-tutorial.txt` for the end-user walkthrough

## Scope

This document describes the current ncurses interface as implemented in
`src/ui/`. It is intended for contributors who need to understand how the
screen is divided, how the seven views behave, and where to make UI changes.

It is not a full command reference. Keybindings, colors, and visible formats
are configurable, so treat the layout and interaction model here as the stable
part and the exact text rendering as user-defined.

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
  library views 1 and 2.

### Bottom Chrome

The bottom three rows are shared across all views:

- Title line: the currently playing track or stream title. It also updates the
  terminal title when `set_term_title` is enabled.
- Status line: playback status, position or duration, speed, play source,
  volume, and toggle flags such as continue, follow, repeat, and shuffle.
- Command line: empty in normal mode; reused for `:` commands, `/` or `?`
  searches, transient info and error messages, and confirmation prompts.

The status line can also render a progress indicator according to the
`progress_bar` option (`disabled`, `line`, `shuttle`, `color`, or
`color-shuttle`).

## Views

termus has seven numbered views. `src/app/options_ui_state.h` defines the stable
view order and `src/ui/ui_curses.c:set_view()` switches the active searchable
model for each one.

### 1. Library Tree View

This is a split-pane view:

- Left pane: artist and album tree.
- Right pane: tracks for the currently selected artist or album.
- `Tab` switches the active pane.

The split width is controlled by `tree_width_percent` and capped by
`tree_width_max`. The vertical divider is drawn explicitly by
`draw_separator()`.

### 2. Sorted Library View

This is the same library content rendered as one flat list. The heading shows
track count, total duration, mark count, and current sort expression.

This view is backed by the generic editable list path rather than the tree
widgets used by view 1.

### 3. Playlist View

The playlist view can be either split or single-pane:

- Optional left panel: playlist names.
- Right panel: tracks in the visible playlist.
- If the playlist panel is hidden, the track list expands to full width.

The visible header is formatted with `format_heading_playlist`.

### 4. Play Queue View

This is a flat editable list of upcoming tracks. Queue items take priority over
library or playlist playback until the queue is drained.

### 5. Browser View

This is the filesystem browser. The title shows the current directory as
`Browser - <path>`. From here the selected file or directory can be added to
the library, the active playlist, or the queue.

### 6. Filters View

This lists saved library filters. Toggling filters here affects the visible
contents of the library views.

### 7. Settings View

The visible title is `Settings`, but internally this view is still named
`HELP_VIEW`. It is a combined inspector/editor for:

- bound keybindings
- unbound commands
- options

Pressing `Enter` on a row drops the corresponding `bind` or `set` command into
the command line for editing. Pressing `Space` toggles an option when that
option exposes a toggle handler.

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
`n` and `N` follow-up search behavior works across views 1-7, but the matched
fields differ by view.

## Core Interaction Model

Most visible actions are the same command system exposed through different
entry points:

- keybindings dispatch commands through `src/ui/keys.c`
- `:` commands are parsed in `src/ui/command_mode.c`
- search editing lives in `src/ui/search_mode.c`
- remote control uses the same command handlers through IPC

That shared model explains why UI actions often mirror a command exactly. A few
important examples:

- `1` to `7`: switch views
- `Enter`: activate the selected item for the current view
- `Space`: expand, mark, toggle, or select depending on the view
- `a`, `y`, `e`, `E`: copy the selected or marked item to another collection
- `:`: open command mode
- `/` and `?`: open search mode

The exact default bindings are documented in `man/termus.txt`, but the important
contributor detail is that the UI is command-driven rather than hardcoding
special-case behavior for each key.

## Rendering and Customization

Visible strings are mostly produced from format options registered in
`src/ui/options_display.c`. The most important ones for UI work are:

- `format_title` and `altformat_title`
- `format_statusline`
- `format_heading_album`
- `format_heading_artist`
- `format_heading_playlist`
- `format_trackwin*`
- `format_playlist*`

Colors and text attributes are also option-backed. The curses color pair lookup
used by the main renderer lives in `src/ui/ui_curses.c`.

This means many interface changes can be made in formatting and option code
without changing the underlying window model.

## Implementation Map

For UI work, these files are the main entry points:

- `src/ui/ui_curses.c`: screen layout, resize handling, redraw scheduling,
  view switching, title/status/command lines, and the main event loop
- `src/ui/window.c`: generic scrolling window abstraction
- `src/ui/editable_view.c`: adapter for editable list views
- `src/ui/browser.c`: browser view model and actions
- `src/ui/help.c`: settings view content and editing behavior
- `src/ui/keys.c`: key contexts, bindings, and dispatch
- `src/ui/command_mode.c`: command-line parser and handlers
- `src/ui/search_mode.c`: interactive search
- `src/ui/options_display.c`: default UI format strings and color option names

If you need to change what the user sees, start in `ui_curses.c`. If you need
to change what a row represents or how it scrolls, the next place is usually
`window.c`, `editable_view.c`, or the view-specific module.
