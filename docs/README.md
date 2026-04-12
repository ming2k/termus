# Documentation Guide

This directory is the entry point for contributor-facing project docs.

## Read This First

Pick the shortest path based on what you need:

- New contributor: read `../README.md`, then `development.md`, then `contributing.md`
- Trying to understand the codebase shape: read `project-structure.md`, then `architecture.md`
- Working on runtime/config behavior: read `runtime.md`
- Working on the ncurses interface: read `tui.md`
- Working on playback, plugins, or thread interactions: read `architecture.md`

## Document Map

- `project-structure.md`: top-level repository layout, source directory ownership,
  where to place new code, and what counts as generated or scratch state
- `contributing.md`: repository rules, change placement, dependency direction,
  and contribution checklists
- `architecture.md`: runtime model, threads, pipeline, plugin ABI, IPC, and
  current dependency direction
- `development.md`: build, test, local development workflow, and cleanup
- `runtime.md`: config directory, `autosave`, socket placement, and runtime
  state semantics
- `tui.md`: screen layout, numbered views, input modes, and where UI behavior
  lives in `src/ui/`

## Current Gaps This Index Fixes

Before this index, the same project facts were spread across `README.md`,
`development.md`, and `architecture.md`, which made the repo feel more disorganized
than it actually is. The intended split is now:

- `README.md`: quick project and build overview
- `docs/project-structure.md`: repository navigation and structure
- `docs/contributing.md`: contribution expectations and repository policy
- `docs/*.md`: deeper topic-specific references

## When Adding New Docs

- Prefer linking from this index instead of repeating long explanations in
  multiple files.
- Put build workflow in `development.md`, not in architecture docs.
- Put contribution policy and checklists in `contributing.md`.
- Put directory ownership and layering rules in `project-structure.md`.
- Put runtime file behavior in `runtime.md`.
- Put thread, plugin, and subsystem design in `architecture.md`.
- Put ncurses screen layout and interaction details in `tui.md`.
