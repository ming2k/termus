# termus Architecture

See also:

- `docs/README.md` for the documentation index
- `docs/project-structure.md` for repository layout and subsystem ownership
- `docs/development.md` for build and test workflow

## Overview

termus is a terminal music player built around a multi-threaded audio pipeline.
The UI, audio decoding, and audio output each run on separate threads and
communicate through a shared ring buffer and condition variables.

The repository layout is clearer than the historical upstream tree, but some
older cross-layer dependencies still exist in the implementation. This document
describes both the current structure and the intended direction for ongoing
cleanup.

```text
┌─────────────────────────────────────────────────────────┐
│                        termus process                     │
│                                                         │
│  Main thread          Producer thread   Consumer thread │
│  ┌───────────┐        ┌─────────────┐  ┌─────────────┐  │
│  │ UI/ncurses│        │Input plugin │  │Output plugin│  │
│  │  server   │──cmds─▶│  (decoder)  │─▶│  (playback) │  │
│  │  (select) │        │             │  │             │  │
│  └───────────┘        └──────┬──────┘  └──────▲──────┘  │
│                              │  ring buffer   │         │
│                              └────────────────┘         │
│                                                         │
│  Worker thread                                          │
│  ┌────────────────────────────────┐                     │
│  │ metadata scan / cache update   │                     │
│  └────────────────────────────────┘                     │
└─────────────────────────────────────────────────────────┘
```

## Threads

### Main thread (`src/ui/ui_curses.c`)

Runs the event loop (`select(2)`). Handles:

- ncurses input and display refresh
- Unix/TCP socket (`server_socket`) for `termus-remote`
- dispatching commands from both the TUI and remote clients via
  `src/ui/command_mode.c`

Entry point: `main()` in `src/ui/ui_curses.c`.

### Producer thread (`src/core/player.c` — `producer_loop`)

Reads decoded PCM samples from the active input plugin and writes them into the
shared ring buffer (`src/core/buffer.c`). Blocks on `producer_playing`
condition when paused or stopped.

### Consumer thread (`src/core/player.c` — `consumer_loop`)

Drains the ring buffer and writes PCM data to the active output plugin. Blocks
on `consumer_playing` condition when the buffer is empty or playback is paused.

### Worker thread (`src/common/worker.c`)

Processes background jobs off the main thread to keep the UI responsive. The
generic queue lives in `src/common/worker.c` and the job type definitions live
in `src/common/worker.h`. The UI result delivery lives in `src/ui/job.c`.

Jobs are typed with bit flags from `src/common/worker.h`:

| Flag                    | Description                            |
|-------------------------|----------------------------------------|
| `JOB_TYPE_ADD`          | Add files or directories to a playlist |
| `JOB_TYPE_UPDATE`       | Re-read metadata for existing tracks   |
| `JOB_TYPE_UPDATE_CACHE` | Sync the track cache to disk           |
| `JOB_TYPE_DELETE`       | Remove tracks from a playlist          |

Jobs target one or more playlist contexts with `JOB_TYPE_LIB`,
`JOB_TYPE_PL`, and `JOB_TYPE_QUEUE`.

## Audio Pipeline

```text
File
 │
 ▼
Input plugin  ←─ open/read/seek/close via struct input_plugin_ops (src/core/ip.h)
 │  PCM bytes
 ▼
Ring buffer   ←─ src/core/buffer.c  (lock-free SPSC, about 2 seconds of audio)
 │  PCM bytes
 ▼
Output plugin ←─ open/write/close via struct output_plugin_ops (src/core/op.h)
 │
 ▼
Sound hardware
```

**Sample format** (`src/core/sf.h`): a single `uint64_t` (`sample_format_t`)
encodes sample rate, bit depth, channel count, and byte order in bitfields.
Both plugins agree on one format; `src/core/pcm.c` and `src/core/convert.c`
handle format conversion when needed.

**Playback speed** (`src/core/player.c`): implemented as tape-speed. The
hardware sample rate sent to the output plugin is scaled by `playback_speed`.
A 2× speed doubles the rate, causing pitch-proportional speedup without
bringing in a time-stretch library.

## Plugin System

Plugins are shared libraries loaded at runtime with `dlopen`. They live in
`$(libdir)/termus/input/` and `$(libdir)/termus/output/`, overridable with
`$TERMUS_LIB_DIR`.

### Input plugins (`src/plugins/input/`)

Each plugin exports these symbols, declared in `src/core/ip.h`:

```c
const struct input_plugin_ops ip_ops;
const int ip_priority;
const char * const ip_extensions[];
const char * const ip_mime_types[];
const struct input_plugin_opt ip_options[];
const unsigned ip_abi_version;
```

`struct input_plugin_ops` provides:

- `open`
- `close`
- `read`
- `seek`
- `read_comments`
- `duration`
- `bitrate`
- `codec`
- `codec_profile`

Plugin selection happens in `src/core/input.c`: first by extension, then by
MIME type, with lower `ip_priority` preferred.

### Output plugins (`src/plugins/output/`)

Each plugin exports these symbols, declared in `src/core/op.h`:

```c
const struct output_plugin_ops op_pcm_ops;
const struct output_plugin_opt op_pcm_options[];
const int op_priority;
const unsigned op_abi_version;
```

`struct output_plugin_ops` provides:

- `init`
- `exit`
- `open`
- `close`
- `write`
- `buffer_space`
- `drop`
- `pause`
- `unpause`

## IPC — termus-remote

`src/ipc/server.c` opens a listening socket. On Linux and macOS it is usually a
Unix domain socket at `$XDG_RUNTIME_DIR/termus-socket` or the config-directory
fallback. A TCP fallback on port 3000 also exists.

`termus-remote` in `src/app/main.c` connects, sends newline-terminated command
strings, and reads back responses. Unix connections are trusted; TCP
connections are restricted to safe commands unless a password is set.

The main thread's `select()` loop includes `server_socket` and open client file
descriptors. Incoming lines are dispatched through the same command handler
used by the TUI.

## Library and Playlist State

### `struct track_info` (`src/core/track_info.h`)

The canonical record for a single track: file path, duration, codec, bitrate,
and arbitrary metadata tags stored as key/value pairs. Track records are
reference-counted and cached to disk through `src/library/cache.c`.

### Views (`src/library/lib.c`, `src/library/pl.c`, `src/library/play_queue.c`)

| Source | Description |
|--------|-------------|
| Library | All tracks organised in artist and album trees |
| Playlist | User-editable ordered lists |
| Play queue | Temporary FIFO override |

All three share the `struct editable` abstraction from
`src/library/editable.c`. Today that abstraction still carries some UI-facing
state; the modernization goal is to keep collection state in `library/` and
move curses window state into `ui/`.

## Commands and Options

### Commands (`src/ui/command_mode.c`)

The `commands[]` table maps command strings to `cmd_*` handlers. The same table
is used for `:command` input in the TUI, `termus-remote --raw`, and keybinding
dispatch.

### Options (`src/ui/options.c` plus `src/app/options_*.c`)

Option registration is split by area in `src/app/options_*.c` and wired through
the registry declared in `src/app/options_registry.h`. The UI-facing load path
still starts from `src/ui/options.c`, but the option definitions themselves are
already separated by subsystem.

### Format strings (`src/ui/format_print.c`)

The statusline and track format strings support `%{tag}` substitution and
`%{?tag?text}` conditional blocks. Format option values are populated per frame
by `get_global_fopts()` in `src/ui/ui_curses.c`.

## Directory Roles

```text
src/app        process setup, command-line entry points, option state/registry
src/common     generic helpers, locale state, and low-level utilities
src/core       playback pipeline, metadata, codecs, transport
src/ipc        remote control and desktop integration
src/library    library, playlists, filters, search
src/plugins    runtime-loaded input/output plugins
src/ui         curses UI, command mode, rendering, window state
tests/         low-level regression tests run by make check
```

This section is the architectural view of the codebase. For top-level
repository navigation and where new files should live, use
`docs/project-structure.md`.

## Dependency Policy

This is the intended direction for new work:

- `common` should not depend on `ui`.
- `core` should not depend on `ui`.
- `library` should model playlist and library state, not own curses windows.
- `ipc` should expose protocol-facing state without reusing TUI formatting
  internals.
- `app` is the composition layer that wires subsystems together.

**Current Status:**
The `common` and `core` layers are strictly decoupled from `ui`. They use a
notification system (`src/common/msg.h`) with function pointers
(`info_handler`, `error_handler`, `yes_no_query_handler`) to interact with
the user. These pointers are hooked up to the ncurses UI during `main()` in
`src/app/main.c` / `src/ui/ui_curses.c`.

The `library` and `ipc` layers still have some historical cross-layer
dependencies (e.g., `library` heavily depends on `ui/window.h`). Treat this
section as the policy to move toward when refactoring those remaining
subsystems or adding features.
