# Contributing Guide

This document covers contribution policy rather than local build mechanics.
For build, test, and development environment setup, see
`docs/development.md`.

## Scope

Use this guide when you are changing the repository, not just building it
locally. It covers:

- which files are authoritative
- where new code should go
- what dependency direction new work should follow
- what a contribution is expected to update alongside code

## Repository Rules

- Maintain `configure.ac` and `Makefile.am`.
- Do not hand-edit generated `configure`, `Makefile.in`, or `Makefile`.
- Prefer out-of-tree builds.
- Treat C23 as the language baseline for new code and refactors.
- Keep style consistent with surrounding C code.
- Use hard tabs.
- Add new tests under `tests/` and wire them into `make check`.
- Prefer standard facilities such as `static_assert` over project-local
  compatibility macros when the code already assumes C23.
- In `src/ui/`, include `ui/curses_compat.h` for ncursesw access instead of
  adding new per-file curses include branches.
- Do not cast away `const` just to satisfy a writable API; copy the buffer
  locally when mutation is actually required.

## Where Changes Should Go

For the full repository map, see `docs/project-structure.md`.

The short version is:

```text
src/app        process setup, startup flow, option state, composition layer
src/common     generic low-level helpers with no UI policy
src/core       playback engine, metadata, codecs, transport, PCM flow
src/ipc        remote control server and desktop integration
src/library    library, playlists, search, filters, collection state
src/plugins    loadable input and output plugins
src/ui         ncurses UI, rendering, key handling, presentation state
tests/         low-level regression tests
docs/          contributor-facing documentation
```

Use responsibility, not convenience, to decide placement. Avoid using the
repository root as a long-term holding area for scratch patches, reject files,
or alternate build outputs.

## Dependency Direction

New work should move the codebase toward this direction:

- `common` should not depend on `ui`.
- `core` should not depend on `ui`.
- `library` should model playlist and library state, not own curses windows.
- `ipc` should expose protocol-facing state, not reuse TUI formatting directly.
- `app` is the composition layer that wires subsystems together.

Current state:

- `common` and `core` are already decoupled from `ui`.
- `library` and `ipc` still carry some historical cross-layer dependencies.

Treat this as policy for new code and refactors. Do not add fresh cross-layer
globals when the existing direction is already clearer.

## What To Update With Code Changes

- API or layout changes: update `docs/project-structure.md` or
  `docs/architecture.md` when the mental model changes
- build graph or dependency changes: update `docs/development.md`
- runtime state or config path changes: update `docs/runtime.md`
- contributor workflow expectations: update this document

The project already felt more disorganized than it was because information was
spread across multiple docs. Prefer updating one canonical document over
copying the same explanation into several files.

## Plugin Contributions

Input plugin checklist:

1. Add source under `src/plugins/input/`.
2. Export `ip_ops`, `ip_extensions`, `ip_mime_types`, `ip_options`,
   `ip_priority`, `ip_abi_version`.
3. Register the module in `src/plugins/input/Makefile.am`.
4. Add dependency detection and `AM_CONDITIONAL` in `configure.ac`.

Output plugin checklist:

1. Add source under `src/plugins/output/`.
2. Export `op_pcm_ops`, optional mixer ops/options, priority, ABI version.
3. Register the module in `src/plugins/output/Makefile.am`.
4. Add dependency detection and `AM_CONDITIONAL` in `configure.ac`.
