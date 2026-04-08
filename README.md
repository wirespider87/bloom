# Bloom

Bloom is an immediate-mode GUI library written in C.

The short version: this project is for the kind of software people usually end up building with ImGui anyway. Overlays. Debug panels. Internal editors. Tool windows. Trainers. Reverse-engineering frontends. Memory tools. Config UIs that sit next to a game or attach to one.

Bloom is meant to live in that space on purpose.

It is not trying to be a general desktop UI toolkit. It is not trying to look like a clone of ImGui either. The goal is simpler than that: make a practical immediate-mode library that can replace ImGui for a lot of native tool work, especially the game-hacking and game-tooling side of it.

It is not source-compatible with ImGui. It is a replacement in the "same job, different library" sense.

## What Bloom Is For

- in-game overlays
- external tool windows
- debug UIs
- editors and inspectors
- reverse-engineering tools
- game hacking utilities
- launchers, config tools, and other native helper apps

If your UI mostly exists to expose values, toggle behavior, inspect state, or drive a renderer-backed tool, Bloom is the target.

## Why It Exists

ImGui is good, but a lot of people use it for the exact same class of projects over and over: game-adjacent tools, overlays, live editors, and utility UIs. Bloom is built for that same category, with a different taste and a different set of defaults.

What Bloom already gives you:

- plain C API through `bloom.h`
- immediate-mode workflow
- built-in Win32 platform layer
- OpenGL backend
- optional D3D11 backend
- widgets that make sense for actual tool UIs instead of toy demos

## What Is In The Repo Right Now

Bloom already has:

- windows, child windows, layout, draw lists, input, styling, animation, memory helpers, and hashing
- text, buttons, ghost buttons, directional buttons, checkboxes, toggles, radio buttons, hyperlinks, spinners, and progress bars
- sliders, bar sliders, tall sliders, scrub fields, span editors, and general numeric value editors
- text inputs and multiline text areas
- combo boxes, filterable selects, multi-selects, tables, trees, tooltips, splitters, and selectable text
- color editors, color pickers, and swatches
- image widgets
- a showcase app in `examples/widgets_showcase/main.c`

## Build

Bloom uses xmake.

Default build:

```bash
xmake f -c -y
xmake build
```

OpenGL is on by default.

If you want D3D11 too:

```bash
xmake f --d3d11=y -c -y
xmake build
```

## License

Bloom is released under `0BSD`.

That means people can use it, modify it, ship it, and fold it into other projects without attribution requirements.

## Start Here

If you want to see how the library is supposed to be used, open `examples/widgets_showcase/main.c`.

If you want to integrate it, include:

```c
#include "bloom.h"
```

There is also a short [COMMIT_GUIDE.md](COMMIT_GUIDE.md) in the repo if you want the house rules for commits.

## What Bloom Is Not

Bloom is currently Windows-first.

It is not trying to compete with Qt, WPF, or web UI frameworks. It is also not pretending to be a drop-in ImGui fork. It is a native immediate-mode GUI library aimed at tools, overlays, and game-facing utility software.

If that is the kind of software you build, Bloom should make sense immediately.