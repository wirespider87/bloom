# Bloom

Bloom is an immediate-mode GUI library written in C.

The short version: this project is for the kind of software people usually end up building with [ImGui](https://github.com/ocornut/imgui) anyway. Overlays. Debug panels. Internal editors. Tool windows. Trainers. Reverse-engineering frontends. Memory tools. Config UIs that sit next to a game or attach to one.

Bloom is meant to live in that space on purpose.

It is not trying to be a general desktop UI toolkit. It is not trying to look like a clone of [ImGui](https://github.com/ocornut/imgui) either. The goal is simpler than that: make a practical immediate-mode library that can replace [ImGui](https://github.com/ocornut/imgui) for a lot of native tool work, especially the game-hacking and game-tooling side of it.

It is not source-compatible with [ImGui](https://github.com/ocornut/imgui). It is a replacement in the "same job, different library" sense.

## Quick Jump

- [What Bloom Is For](#what-bloom-is-for)
- [Build](#build)
- [Add To Your Project](#add-to-your-project)
- [Xmake Package Setup](#xmake-package-setup)
- [CMake Source Integration](#cmake-source-integration)
- [Visual Studio Solution Integration](#visual-studio-solution-integration)
- [License](#license)
- [Start Here](#start-here)

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

[ImGui](https://github.com/ocornut/imgui) is good, but a lot of people use it for the exact same class of projects over and over: game-adjacent tools, overlays, live editors, and utility UIs. Bloom is built for that same category, with a different taste and a different set of defaults.

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

Bloom uses [xmake](https://xmake.io/).

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

## Add To Your Project

Bloom works in both C and C++ projects. The library itself is C, but `bloom.h` can be included from either language.

The integration paths below were smoke-tested on Windows x64 in both C and C++ where applicable.

### Xmake Package Setup

<details>
<summary><strong>Xmake Package Setup</strong> - use <code>bloom-packages</code> and <code>add_requires("bloom 1.0.2")</code></summary>

Bloom is distributed through its own [xmake](https://xmake.io/) package repository.

Add this to your project's `xmake.lua`:

```lua
add_rules("mode.debug", "mode.release")

add_repositories("bloom-packages https://github.com/wirespider87/bloom-packages.git")
add_requires("bloom 1.0.2")

target("your_app")
	set_kind("binary")
	set_languages("c11")
	add_files("src/*.c")
	add_packages("bloom")
```

For a C++ target, keep the same package setup and use a C++ target instead:

```lua
target("your_cpp_app")
	set_kind("binary")
	set_languages("cxx17")
	add_files("src/*.cpp")
	add_packages("bloom")
```

Then include:

```c
#include "bloom.h"
```

If you want the D3D11 backend enabled from the package:

```lua
add_requires("bloom 1.0.2", {configs = {d3d11 = true}})
```

If you add the repository and `xmake` does not see `bloom` immediately, run this once:

```bash
xmake repo -u
```

The self-hosted package repo is here:

- [bloom-packages](https://github.com/wirespider87/bloom-packages)

</details>

### CMake Source Integration

<details>
<summary><strong>CMake Source Integration</strong> - vendor Bloom and build it as a static library from source</summary>

If you are using CMake, the simplest path is to vendor Bloom into your source tree and build it as a static library from source.

Example `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(MyApp C CXX)

set(BLOOM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/bloom")

file(GLOB_RECURSE BLOOM_CORE_SOURCES CONFIGURE_DEPENDS
	"${BLOOM_ROOT}/core/*.c")
file(GLOB_RECURSE BLOOM_WIDGET_SOURCES CONFIGURE_DEPENDS
	"${BLOOM_ROOT}/widgets/*.c")
file(GLOB BLOOM_PLATFORM_SOURCES CONFIGURE_DEPENDS
	"${BLOOM_ROOT}/platform/win32/*.c")
file(GLOB BLOOM_OPENGL_SOURCES CONFIGURE_DEPENDS
	"${BLOOM_ROOT}/rendering/opengl/*.c")

add_library(bloom STATIC
	${BLOOM_CORE_SOURCES}
	${BLOOM_WIDGET_SOURCES}
	${BLOOM_PLATFORM_SOURCES}
	${BLOOM_OPENGL_SOURCES})

target_include_directories(bloom PUBLIC "${BLOOM_ROOT}")
target_compile_features(bloom PUBLIC c_std_11)
target_compile_definitions(bloom PUBLIC BLOOM_OPENGL_BACKEND)
target_link_libraries(bloom PUBLIC opengl32 user32 gdi32 dwmapi shell32)

add_executable(your_app src/main.cpp)
target_link_libraries(your_app PRIVATE bloom)
```

Use `src/main.c` for a C target or `src/main.cpp` for a C++ target.

If you want D3D11 too, also add the sources under `rendering/d3d11`, define `BLOOM_D3D11_BACKEND`, and link `d3d11`, `dxgi`, and `d3dcompiler`.

Keep `opengl32` linked even in that setup. The current Win32 platform layer still uses WGL during platform/context setup.

</details>

### Visual Studio Solution Integration

<details>
<summary><strong>Visual Studio Solution Integration</strong> - add Bloom source directly to a <code>.sln</code> and build for <code>Release|x64</code></summary>

If you are working directly in a Visual Studio `.sln` instead of CMake or [xmake](https://xmake.io/), add Bloom as source.

- Use `x64`. The validated smoke solution was built as `Release|x64`.
- Put the Bloom repo somewhere inside your solution tree, for example `external\bloom`.
- Create a `Static Library` project for Bloom, or add Bloom's `.c` files directly to your existing app project.
- Add the `.c` files under `core`, `widgets`, `platform\win32`, and the rendering backend folder you want.
- Add the Bloom root folder to `Additional Include Directories`.
- Add `BLOOM_OPENGL_BACKEND` to `Preprocessor Definitions` for the default OpenGL path.
- If you also want D3D11, add the `rendering\d3d11` sources and define `BLOOM_D3D11_BACKEND` too.
- Link `opengl32.lib`, `user32.lib`, `gdi32.lib`, `dwmapi.lib`, and `shell32.lib`.
- If D3D11 is enabled, also link `d3d11.lib`, `dxgi.lib`, and `d3dcompiler.lib`.
- Include `bloom.h` from either C or C++ source files.

For Visual Studio C++ projects, keep Bloom's own files as `.c` sources, keep your application files as `.cpp`, and include `bloom.h` normally from your C++ code.

</details>

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

It is not trying to compete with Qt, WPF, or web UI frameworks. It is also not pretending to be a drop-in [ImGui](https://github.com/ocornut/imgui) fork. It is a native immediate-mode GUI library aimed at tools, overlays, and game-facing utility software.

If that is the kind of software you build, Bloom should make sense immediately.