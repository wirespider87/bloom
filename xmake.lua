set_project("bloom")
set_version("1.0.3")

add_rules("mode.debug", "mode.release")

option("opengl")
    set_default(true)
    set_showmenu(true)
    set_description("Build OpenGL rendering backend")
option_end()

option("d3d11")
    set_default(false)
    set_showmenu(true)
    set_description("Build Direct3D 11 backend")
option_end()

option("shared")
    set_default(false)
    set_showmenu(true)
    set_description("Build bloom as a shared library")
option_end()

option("examples")
    set_default(true)
    set_showmenu(true)
    set_description("Build showcase/example programs")
option_end()

target("bloom")
    set_kind(has_config("shared") and "shared" or "static")
    set_languages("c11")

    add_includedirs(".", {public = true})

    add_files("core/api.c")
    add_files("core/**/*.c")
    add_files("widgets/**/*.c")
    add_files("platform/win32/*.c")

    if has_config("opengl") then
        add_defines("BLOOM_OPENGL_BACKEND", {public = true})
        add_files("rendering/opengl/*.c")
    end

    if has_config("d3d11") then
        add_defines("BLOOM_D3D11_BACKEND", {public = true})
        add_files("rendering/d3d11/*.c")
        add_syslinks("d3d11", "dxgi", "d3dcompiler")
    end

    add_headerfiles("bloom.h")
    add_headerfiles("(core/**.h)")
    add_headerfiles("(widgets/**.h)")
    add_headerfiles("(rendering/**.h)")
    add_headerfiles("(platform/**.h)")

    add_syslinks("opengl32", "user32", "gdi32", "dwmapi", "shell32")

if has_config("examples") and has_config("opengl") then
    target("example_widgets")
        set_kind("binary")
        set_languages("c11")
        add_deps("bloom")
        add_includedirs(".")
        add_files("examples/widgets_showcase/main.c")
end
