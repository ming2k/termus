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

- C compiler with C11 atomics support or newer
- autoconf, automake, libtool
- pkg-config
- ncursesw
- pthreads

Use `autoreconf`, not `autoconf`.

## Build Toolkit

This project uses a classic Unix build stack centered on Autotools. The layers
are intentionally separated so contributors know where to change build logic.

- `configure.ac`: feature detection, compiler policy, optional dependencies,
  install paths, and generated `Makefile`s
- top-level `Makefile.am`: subdirectory order, exported docs, and convenience
  targets such as `make run`
- `src/Makefile.am`: product assembly for `termus`, `termus-remote`, and
  `libcommon.la`
- `src/plugins/*/Makefile.am`: optional plugin modules built through Libtool
- `tests/Makefile.am`: `make check` targets
- `pkg-config`: external dependency discovery for ncurses and optional backends

Practical rule:

- change `configure.ac` when availability or policy changes
- change `Makefile.am` files when target composition changes
- rerun `autoreconf` only when Autotools inputs changed

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
../configure --disable-alsa
../configure --help
```

Developer-oriented switches:

- `--enable-debug=N`: compiled-in debug level from 0 to 2
- `--enable-warnings-as-errors`: enable `-Werror` when supported
- `--enable-sanitizers[=LIST]`: runtime sanitizers, with `yes` meaning
  `address,undefined`

Artifacts stay under `build/`:

- `build/src/termus`
- `build/src/termus-remote`
- `build/src/plugins/input/.libs/`
- `build/src/plugins/output/.libs/`

## Test And Run

Run from the build tree without installing:

```sh
make run
```

Run the test suite:

```sh
make check
```

`make check` currently uses a small low-level regression test set under
`tests/`. The goal is to keep a stable place for incremental coverage as
subsystems are cleaned up.

## Development Environment

For development and debugging, do not use your daily config directory. Keep a
separate termus environment so autosave, cache, history, playlists, and socket
state do not interfere with normal usage.

For the runtime meaning of `autosave`, `rc`, and the control socket, see
`docs/runtime.md`.

Recommended local setup:

```sh
mkdir -p /tmp/termus-dev
TERMUS_HOME=/tmp/termus-dev \
TERMUS_SOCKET=/tmp/termus-dev/socket \
build/src/termus
```

Why this is the preferred workflow:

- Keeps your real `~/.config/termus` untouched
- Makes test runs reproducible
- Lets you reset state with `rm -rf /tmp/termus-dev`
- Avoids socket collisions when another termus instance is already running

Suggested practices:

- Start with an empty dev config dir unless you are debugging user-specific
  behavior
- Add only the minimum files under the dev `TERMUS_HOME` such as `rc` or
  `*.theme`
- Treat `autosave`, `cache`, `command-history`, `search-history`, and `resume`
  as disposable debug state
- Use a different `TERMUS_SOCKET` for each concurrent instance

If you specifically want to test default XDG path behavior, isolate the XDG
variables instead of touching your real account config:

```sh
mkdir -p /tmp/termus-xdg/config /tmp/termus-xdg/state /tmp/termus-xdg/cache /tmp/termus-xdg/data /tmp/termus-xdg/runtime
XDG_CONFIG_HOME=/tmp/termus-xdg/config \
XDG_STATE_HOME=/tmp/termus-xdg/state \
XDG_CACHE_HOME=/tmp/termus-xdg/cache \
XDG_DATA_HOME=/tmp/termus-xdg/data \
XDG_RUNTIME_DIR=/tmp/termus-xdg/runtime \
build/src/termus
```

Use that only when validating path resolution or startup behavior. For ordinary
feature work, `TERMUS_HOME=/tmp/termus-dev` is simpler.

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
