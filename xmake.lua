add_rules("mode.debug", "mode.release")

target("sand")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("sdl2")
    add_rules("plugin.vsxmake.autoupdate")
