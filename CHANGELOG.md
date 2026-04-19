# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.0] - 2026-04-19

### Added
- UI: New popup system with modal filter editor and track-info overlay (`F` for filters, `I` for track info).
- UI: Configurable library columns via `library_columns` option (e.g. `set library_columns=artist,title,duration`).
- UI: Configurable now-playing title fields via `now_playing_fields` option.
- UI: Left/right view navigation commands.
- Build: Added `GNUmakefile` at project root so `make` and `make run` work directly from the source directory without entering the build subdirectory.

### Changed
- Status line: Mode flags now show readable labels (`con`, `flw`, `rep`, `shuf`, `alb`) only when active; hidden when off, eliminating visual noise from inactive flags.
- Status line: Volume hidden when at 100%; shown as `vol:xx%` when below, stereo channels as `vol:L%/R%`.
- Status line: Playback speed moved to the right side alongside mode flags and hidden when at the default 1.0×.
- Status line: Removed `|` separator between volume and mode flags.
- Config: `show_remaining_time` is no longer bound to `t` by default; set it in the rc file with `toggle show_remaining_time` as needed.

## [1.1.0] - 2026-04-19

### Added
- Core: Integrated `-fanalyzer` and `-Werror` strict compilation modes to prevent memory leaks and pointer bugs.
- Build: Added automatic test integration for playback logic (`make check`).
- Build: Re-organized `libcommon.la` to provide better static analysis and testability.

### Changed
- Refactoring: The entire codebase is now strictly formatted using `clang-format` (LLVM style) for long-term maintainability.
- Input: Promoted `FLAC` 24-bit streams to 32-bit natively inside the decoder to solve historical hardware misalignment issues on Linux.

### Fixed
- Player: Fixed a critical loop condition where changing playback speeds (`:speed`) on unsupported hardware rates (especially Hi-Res files) would cause a 100% CPU lockup and severe continuous white noise. The player will now detect write failures and smoothly fallback to 1.0x playback speed.
- Memory: Fixed a hidden memory leak in the options string setter (`generic_set_str`) caught by the new static analysis suite.

## [1.0.4] - 2026-04-13

### Added
- Auto-resume support enabled by default: program remembers view, position, and track selection
- Persistent playlist selection support
- Improved startup behavior: automatically pauses playback if it was playing/paused previously

## [1.0.3] - 2026-04-12

### Fixed

- Fix SIGSEGV on exit: Reordered `exit_all()` to ensure state is saved
  before system services or plugins are closed.
- Fix library and command history persistence: Modified the exit sequence
  to complete pending background jobs before saving the library and history.

## [1.0.2] - 2026-04-12

### Fixed

- Fix SIGSEGV on startup when a resume state exists: `resume_load()` called
  `track_info_unref(ti)` twice but only held one ref (obtained from
  `cache_get_ti`). The extra unref stole a ref from one of the internal
  owners (hash table, views, or `cur_track_ti`), causing a premature free
  and subsequent use-after-free crash. Previously loaded songs appeared
  missing because termus crashed before `lib.pl` finished loading.

## [1.0.1] - 2026-04-12

### Fixed

- Fix `termus: loading ${prefix}/share/termus/rc: No such file or directory`
  on startup: `configure.ac` was only doing one `eval` pass on `$datadir`,
  leaving the two-level autoconf chain (`datadir` → `datarootdir` → `prefix`)
  unresolved so the literal string `${prefix}` was baked into the binary.
  Added `eval "datarootdir=\"$datarootdir\""` before the `datadir` eval so
  the full path is expanded at configure time
- Remove duplicate `man1_MANS` / `nodist_man1_MANS` variable causing `make
  install` to attempt installing generated man pages twice, resulting in an
  "install: will not overwrite just-created" error (exit code 2 in stage
  'staging')
- Drop `termus-tutorial.7` man page; only `termus.1` and `termusc.1`
  are installed

## [1.0.0] - 2026-04-12

### Added

- Multi-threaded audio pipeline with separate UI, decoder, and output threads
  communicating via a lock-free ring buffer
- ncurses TUI with semantic views, command mode, and remote-control socket
- Input plugin support: AAC, FLAC, MP3 (libmad/mpg123/nomad), Ogg Vorbis,
  Opus, WAV, WavPack, MP4/AAC, Musepack, MOD/MikMod/ModPlug, VTX, BASS, CDDA
- Output plugin support: ALSA, PipeWire, JACK, OSS, sndio, ao, RoarAudio,
  AAudio, CoreAudio, Sun audio, WaveOut
- MPRIS2 D-Bus integration for desktop media controls
- Unix and TCP socket IPC via `termusc`
- Library management with playlist, filter, and search support
- Metadata scanning with background worker thread and on-disk cache
- Autoconf/Automake build system with `--enable`/`--disable` plugin flags
- Runtime config directory, autosave, and persistent option store
- Comprehensive contributor documentation under `docs/`
