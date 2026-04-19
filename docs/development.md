# Development Guide

See also:

- `docs/contributing.md` for repository policy and contribution expectations
- `docs/project-structure.md` for repository navigation and directory ownership
- `docs/runtime.md` for config and runtime-state behavior

## Audience

This document is for people who need to use the repository as developers rather
than just end users. That includes:

- contributors building and testing local changes
- maintainers debugging runtime behavior in isolated environments
- distribution packagers integrating termus into downstream build systems

It focuses on how to build, run, test, and stage this project. It does not try
to restate repository policy or architecture details.

## Toolchain

Required:

- C23-capable C compiler with atomics support
- autoconf, automake, libtool
- pkg-config
- ncursesw
- pthreads

Primary format libraries (required by default):

- libFLAC (`libflac-dev`)
- opusfile (`libopusfile-dev`)
- faad2 (`libfaad-dev`)
- mp4v2 + faad2 (`libmp4v2-dev libfaad-dev`)
- libvorbisfile (`libvorbis-dev`) — auto-detected

Use `autoreconf`, not `autoconf`.

## C23 Baseline

The project now builds only in C23 mode. In practice `configure` will select
the first working compiler flag among the supported C23 spellings (currently
`-std=gnu2x`, `-std=c2x`, `-std=gnu23`, or `-std=c23`) and stop if none work.

Practical implications for contributors:

- treat C23 as the baseline, not as an optional newer mode
- prefer standard `static_assert` and other standard language facilities over
  local compatibility shims
- fix const-correctness at the API boundary instead of casting away `const`
- keep compiler-specific attributes or builtins centralized in
  `src/common/compiler.h` rather than spreading ad hoc compiler branches

## Build Toolkit

This project uses a classic Unix build stack centered on Autotools. The layers
are intentionally separated so contributors know where to change build logic.

- `configure.ac`: feature detection, compiler policy, optional dependencies,
  install paths, and generated `Makefile`s
- top-level `Makefile.am`: subdirectory order, exported docs, and convenience
  targets such as `make run`
- `src/Makefile.am`: product assembly for `termus`, `termusc`, and
  `libcommon.la`
- `src/plugins/*/Makefile.am`: optional plugin modules built through Libtool
- `tests/Makefile.am`: `make check` targets
- `pkg-config`: external dependency discovery for ncursesw and optional backends

Practical rule:

- change `configure.ac` when availability or policy changes
- change `Makefile.am` files when target composition changes
- rerun `autoreconf` only when Autotools inputs changed

For ncursesw UI code, include `src/ui/curses_compat.h` instead of open-coding
new `<curses.h>` / `<ncurses.h>` platform branches in individual source files.

## Contributor Workflow

Bootstrap once after clone:

```sh
autoreconf --force --install --verbose
```

Recommended out-of-tree build:

```sh
mkdir -p build
cd build
../configure
make
```

After the first configure, ordinary source edits are incremental:

```sh
cd build
make
```

When to rerun which step:

- Changed only `src/`, `data/`, `man/`, `docs/`, or `tests/`: run `make`
- Changed `configure.ac` or any `Makefile.am`: rerun
  `autoreconf --force --install --verbose`, then rerun `../configure`, then
  `make`
- Changed configure options, install prefix, or dependency environment: rerun
  `../configure`, then `make`
- Delete and recreate `build/` only when you want a clean rebuild or the build
  tree is inconsistent

Useful configure examples:

```sh
../configure --prefix=/usr
../configure --enable-debug=2
../configure --enable-warnings-as-errors
../configure --enable-sanitizers
../configure --enable-sanitizers=address
../configure --disable-alsa          # skip ALSA, use PipeWire only
../configure --disable-flac          # not recommended
../configure --disable-mpg123        # skip legacy MP3 support
../configure --help
```

Developer-oriented switches:

- `--enable-debug=N`: compiled-in debug level from 0 to 2
- `--enable-warnings-as-errors`: enable `-Werror` when supported
- `--enable-sanitizers[=LIST]`: runtime sanitizers, with `yes` meaning
  `address,undefined`

Format support flags (see `configure.ac` for the full list):

- `--disable-flac / --disable-opus / --disable-aac / --disable-mp4`: disable
  a primary format (not recommended; these are required by default)
- `--disable-vorbis`: skip Ogg Vorbis auto-detection
- `--disable-mpg123`: skip legacy MP3 support
- `--disable-cue`: skip CUE sheet playlist support

Artifacts stay under `build/`:

- `build/src/termus`
- `build/src/termusc`
- `build/src/plugins/input/.libs/`
- `build/src/plugins/output/.libs/`

## Test And Run

Run the test suite:

```sh
make check
```

`make check` uses a low-level regression test set under `tests/`. The goal is to
keep a stable place for incremental coverage as subsystems are cleaned up.

## Development Environment

For development and debugging, do not use your daily config directory. Keep a
separate termus environment so autosave, cache, history, playlists, and socket
state do not interfere with normal usage.

The build system provides built-in targets to handle this isolation and correctly
stage locally built plugins (which otherwise wouldn't be found without a full `make install`):

- **`make run`**: Launches termus against a persistent sandbox at `build/dev-home/`.
  Configuration, history, and cache survive across invocations. This is the recommended
  workflow for normal feature work.
- **`make run-clean`**: Launches termus against a completely empty, disposable
  temporary directory. Great for testing first-run behavior or reproducing issues
  from a blank slate.
- **`make run-user`**: Launches termus against your real `~/.config/termus` account
  config. Use this *only* if you specifically need to test your daily setup.

If you specifically want to test default XDG path resolution manually, you can
isolate the XDG variables against the raw binary (make sure to run `make stage` first
so plugins are available):

```sh
make stage
mkdir -p /tmp/termus-xdg/config /tmp/termus-xdg/state /tmp/termus-xdg/cache
XDG_CONFIG_HOME=/tmp/termus-xdg/config \
XDG_STATE_HOME=/tmp/termus-xdg/state \
XDG_CACHE_HOME=/tmp/termus-xdg/cache \
TERMUS_LIB_DIR=$PWD/stage \
src/termus
```

Use that only when validating path resolution or startup behavior. For ordinary
feature work, simply use `make run`.

## Packager Notes

For downstream packaging, the important behavior is:

- the project expects an out-of-tree build
- optional plugins and features are detected at `../configure` time
- install staging works through standard `make install DESTDIR=...`
- plugin artifacts are built as shared modules and installed under
  `$(libdir)/termus/input/` and `$(libdir)/termus/output/`

Typical packaging flow:

```sh
autoreconf --force --install --verbose
mkdir -p build
cd build
../configure --prefix=/usr
make
make check
make install DESTDIR=/tmp/termus-pkg
```

Things to keep in mind:

- changing available system libraries changes which optional backends are built
- `../configure --help` is the authoritative place to inspect supported
  feature toggles
- generated Autotools files are not the source of truth; maintain
  `configure.ac` and `Makefile.am`
- when shipping patches that alter dependencies or install layout, update the
  relevant docs alongside the build logic

## Cleanup

Disposable generated files:

- `build/`
- `autom4te.cache/`
- `configure`
- `configure~`
- `aclocal.m4`
- `Makefile.in`
- `config.h.in`
- generated helper files in `build-aux/` and `m4/`

Quick cleanup:

```sh
rm -rf build autom4te.cache
rm -f configure configure~ aclocal.m4 Makefile.in config.h.in
find build-aux -maxdepth 1 -type f ! -name '.gitkeep' -delete
find m4 -maxdepth 1 -type f ! -name '.gitkeep' -delete
```

## Related Docs

- `docs/contributing.md`: repository rules, code placement, and contribution
  checklists
- `docs/project-structure.md`: repository layout and source tree ownership
- `docs/architecture.md`: runtime and subsystem architecture
- `docs/runtime.md`: config paths, autosave, socket, and runtime state
