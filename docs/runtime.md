# Runtime Guide

See also:

- `docs/README.md` for the documentation index
- `docs/development.md` for isolated development environment setup

## Overview

termus has two different kinds of state:

- Static configuration you intentionally maintain, mainly `rc` and optional theme files.
- Runtime-managed state such as `autosave`, cache files, history, resume data, and the control socket.
- User data such as the on-disk library and playlists.

Keeping those separate is useful both for users and for development.

## Config Directory

The config directory is resolved as follows:

- `$TERMUS_HOME/config`, if `TERMUS_HOME` is set.
- Otherwise `$XDG_CONFIG_HOME/termus`.
- Otherwise `~/.config/termus`.

## State Directory

- `$TERMUS_HOME/state`, if `TERMUS_HOME` is set.
- `$XDG_STATE_HOME/termus`, if `XDG_STATE_HOME` is set.
- Otherwise `~/.local/state/termus`.

This directory is for restart-persistent state that termus manages itself rather
than for hand-edited user configuration.

## Cache Directory

- `$TERMUS_HOME/cache`, if `TERMUS_HOME` is set.
- `$XDG_CACHE_HOME/termus`, if `XDG_CACHE_HOME` is set.
- Otherwise `~/.cache/termus`.

This directory is for derived metadata that can be regenerated.

## Data Directory

- `$TERMUS_HOME/data`, if `TERMUS_HOME` is set.
- `$XDG_DATA_HOME/termus`, if `XDG_DATA_HOME` is set.
- Otherwise `~/.local/share/termus`.

This directory is for user library and playlist data rather than hand-edited
configuration.

## Runtime Files

Typical files by directory:

- Config directory:
  `rc`, optional `*.theme`
- State directory:
  `autosave`, `command-history`, `search-history`, `resume`, `queue.pl`
- Cache directory:
  `cache`
- Data directory:
  `lib.pl`, `playlists/`

## autosave

`autosave` is not a hand-written config file. It is the program's own saved
runtime state.

It stores:

- Current option values (including `resume` state).
- Current key bindings.
- Current filters and active filter state.
- Playlist selection state.

Load order:

1. If `autosave` exists in the state directory, termus loads it first.
2. Otherwise it loads the system default `rc`.
3. Then it loads the user `rc` as an override layer.

Practical rule:

- Put intentional long-term settings in `rc`.
- Treat `autosave` as disposable runtime state.

## Socket and IPC

termus exposes a local control socket used by `termusc` and similar tools.

Default socket resolution:

- `TERMUS_SOCKET`, if set.
- Otherwise `$XDG_RUNTIME_DIR/termus-socket`.
- Otherwise `<config-dir>/socket`.

This is a Unix domain socket for local IPC. That is still a good fit for termus:

- It is local-only by default.
- It works well for one long-running player instance plus external control.
- It fits desktop automation and headless or SSH-driven workflows equally well.
- It keeps automation separate from the ncursesw UI itself.

In short, the socket is the control channel, not part of the user-facing
configuration.

## Usage Scenarios

### Desktop Terminal

- Run termus directly in a terminal emulator.
- Keep your normal `rc` and optional theme files under the config directory.
- Use the full ncursesw interface for browsing, queueing, and playback control.

### Desktop Automation

- Keep one running termus instance.
- Use `termusc` through the socket from media keys, hotkeys, bars, launchers, notifications, or other desktop helpers.
- Use MPRIS integration where available.

### Remote or Headless Systems

- Run termus in tmux, screen, or a plain SSH session.
- Control it later with `termusc` without attaching to the full UI.
- This is one reason the local socket model remains useful.

### Development and Testing

- Run with an isolated `TERMUS_HOME`.
- Keep `autosave`, cache, and history disposable.
- Use a dedicated `TERMUS_SOCKET` per instance to avoid collisions.

Example:

```sh
mkdir -p /tmp/termus-dev
TERMUS_HOME=/tmp/termus-dev \
TERMUS_SOCKET=/tmp/termus-dev/socket \
build/src/termus
```

If you specifically want to test the split XDG layout instead of the
single-root override, set all four XDG roots explicitly:

```sh
mkdir -p /tmp/termus-xdg/config /tmp/termus-xdg/state /tmp/termus-xdg/cache /tmp/termus-xdg/data /tmp/termus-xdg/runtime
XDG_CONFIG_HOME=/tmp/termus-xdg/config \
XDG_STATE_HOME=/tmp/termus-xdg/state \
XDG_CACHE_HOME=/tmp/termus-xdg/cache \
XDG_DATA_HOME=/tmp/termus-xdg/data \
XDG_RUNTIME_DIR=/tmp/termus-xdg/runtime \
build/src/termus
```
