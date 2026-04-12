# Commit Guide

This repo does not need a ceremony-heavy workflow.

If you changed one thing, make one commit. If you changed three unrelated things, make three commits.

## Naming

Standard casing in this codebase is **snake_case** (for example: functions, variables, and file names).

## Before You Commit

Do these if they apply:

- build the project: `xmake build`
- make sure the diff is focused
- do not mix refactors, feature work, and random cleanup in the same commit
- if a UI change is visible in the showcase, make sure it still looks right there

## Commit Message Format

Use:

```text
type: short summary
```

Good types for this repo:

- `feat` for new behavior
- `fix` for bug fixes
- `refactor` for internal cleanup without behavior changes
- `docs` for README, guides, and comments
- `build` for xmake or backend setup changes
- `style` for visual polish
- `test` if test/demo coverage is added or adjusted
- `chore` for small maintenance work

Examples:

```text
fix: tighten text atlas downsampling on Win32
feat: add filterable multi-select widget
style: refine checkbox mark proportions
docs: add license and commit guide
build: enable optional d3d11 backend flag
```

## What Makes A Good Commit Here

- The summary should say what changed, not why you are tired.
- Keep the first line short enough to scan in `git log`.
- If the change needs extra context, add a short body under the first line.
- If a commit changes behavior, mention the user-visible effect.

Example with body:

```text
fix: restore stable Win32 font atlas generation

Reverts the stb experiment and goes back to the supersampled GDI path.
This fixes broken spacing and restores the last acceptable text rendering state.
```

## What To Avoid

- `misc fixes`
- `update stuff`
- `more work`
- giant mixed commits
- formatting-only cleanup bundled with functional changes unless the file already had to be touched

## Practical Rule

If somebody opens the commit six months from now, they should be able to answer two questions quickly:

1. What changed?
2. Was it supposed to change behavior or not?

If the message answers that, it is good enough.