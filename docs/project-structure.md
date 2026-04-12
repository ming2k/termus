# Project Structure

## Summary

The source code organization is mostly reasonable already. The repository feels
messy for three different reasons that are easy to conflate:

1. The documentation entry points were fragmented.
2. The top level mixes hand-maintained files with generated Autotools output.
3. Refactor scratch files and extra build directories can sit next to the real
   source tree.

This document separates those concerns so it is clearer what is actual project
structure and what is temporary workspace noise.

## Top-Level Directories

```text
.github/       CI workflows and repository automation
build-aux/     Autotools helper scripts generated or refreshed by autoreconf
contrib/       extra scripts and user-contributed utilities
data/          default rc and themes installed with termus
docs/          contributor-facing documentation
man/           manpage sources and generated man output
m4/            Autoconf macro files
scripts/       development helpers
src/           main source tree
tests/         low-level regression tests used by make check
```

## Top-Level Files

Hand-maintained:

- `configure.ac`
- `Makefile.am`
- `README.md`
- `CHANGELOG.md`
- `.gitignore`

Usually generated or refreshed by Autotools:

- `configure`
- `aclocal.m4`
- `config.h.in`
- `Makefile.in`
- helper files under `build-aux/`
- macro copies under `m4/`

Build outputs and staging:

- `build/`
- any extra build tree such as `build-sanitize/`
- generated binaries, `.libs/`, and staged plugin trees under build dirs

Scratch or refactor residue:

- one-off patch files
- `*.rej` merge/reject leftovers
- temporary notes created during subsystem cleanup

Those last two categories are valid during active work, but they should not be
mistaken for the intended repository layout.

## Source Tree Ownership

```text
src/app        process setup, startup flow, option state, composition layer
src/common     generic low-level helpers with no UI policy
src/core       playback engine, metadata, codecs, transport, PCM flow
src/ipc        remote control server and desktop integration
src/library    library, playlists, search, filters, collection state
src/plugins    loadable input and output plugins
src/ui         ncurses UI, rendering, key handling, presentation state
```

## Dependency Direction

The intended direction for new code is:

```text
app -> common/core/library/ipc/ui
ui  -> library/core/common
ipc -> core/common
library -> core/common
core -> common
common -> (nothing higher-level)
plugins -> core/common ABI
```

Practical rules:

- `common` should stay reusable and avoid `ui` dependencies.
- `core` should not call into curses or presentation code.
- `library` should own collection state, not window rendering details.
- `app` is where subsystem wiring belongs.

Some historical cross-layer edges still exist, but this is the standard new
code should follow.

## Where New Files Should Go

Add code by responsibility, not by who is currently editing it:

- new config or option state: `src/app/`
- reusable helpers with no termus UI policy: `src/common/`
- playback or decoding pipeline logic: `src/core/`
- socket or remote-control behavior: `src/ipc/`
- playlist/library/search logic: `src/library/`
- ncurses rendering or interaction behavior: `src/ui/`
- plugin implementations: `src/plugins/input/` or `src/plugins/output/`
- regression tests: `tests/`
- contributor docs: `docs/`

Avoid using the repository root as a holding area for ad hoc patches, notes, or
alternate build outputs longer than necessary.

## Why The Repo Still Feels Noisy

These are the main clarity problems visible today:

- `README.md`, `docs/development.md`, and `docs/architecture.md` previously repeated
  overlapping context instead of forming a clear hierarchy.
- The root directory contains both canonical project files and generated
  Autotools files, which visually hides the true source layout.
- Extra build trees and scratch refactor artifacts sit alongside the canonical
  directories and make `git status` harder to read.

The first problem is addressed by the docs index and tighter document roles.
The second and third are workflow hygiene issues rather than a source tree
design failure.

## Hygiene Guidelines

To keep the repository understandable during ongoing cleanup:

- Prefer one named out-of-tree build directory unless you genuinely need
  multiple build variants.
- Keep temporary refactor artifacts under a dedicated scratch location instead
  of the repository root when possible.
- Remove `*.rej` leftovers immediately after resolving them.
- Treat generated Autotools files as disposable implementation of the build
  system, not as documentation of project structure.
- Update this document when adding a new top-level directory or a new
  subsystem split.
