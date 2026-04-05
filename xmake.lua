package("bloom")
    set_homepage("https://github.com/bloom-ui/bloom")
    set_description("Lightweight immediate-mode GUI library in C")
    set_license("GPL-2.0-or-later")

    add_configs("opengl",  { description = "Enable OpenGL rendering backend",    default = true,  type = "boolean" })
    add_configs("d3d11",   { description = "Enable Direct3D 11 rendering backend", default = false, type = "boolean" })

    on_install(function (package)
        local configs = {}
        configs.opengl = package:config("opengl")
        configs.d3d11  = package:config("d3d11")
        import("package.tools.xmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:has_cfuncs("bloom_init", { includes = "core/api.h" }))
    end)
package_end()
