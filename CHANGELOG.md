# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
- Drop `termus-tutorial.7` man page; only `termus.1` and `termus-remote.1`
  are installed

## [1.0.0] - 2026-04-12

### Added

- Multi-threaded audio pipeline with separate UI, decoder, and output threads
  communicating via a lock-free ring buffer
- ncurses TUI with numbered views, command mode, and remote-control socket
- Input plugin support: AAC, FLAC, MP3 (libmad/mpg123/nomad), Ogg Vorbis,
  Opus, WAV, WavPack, MP4/AAC, Musepack, MOD/MikMod/ModPlug, VTX, BASS, CDDA
- Output plugin support: ALSA, PipeWire, JACK, OSS, sndio, ao, RoarAudio,
  AAudio, CoreAudio, Sun audio, WaveOut
- MPRIS2 D-Bus integration for desktop media controls
- Unix and TCP socket IPC via `termus-remote`
- Library management with playlist, filter, and search support
- Metadata scanning with background worker thread and on-disk cache
- Autoconf/Automake build system with `--enable`/`--disable` plugin flags
- Runtime config directory, autosave, and persistent option store
- Comprehensive contributor documentation under `docs/`
