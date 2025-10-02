add_rules("mode.debug", "mode.release")

add_requires("libsdl2")

target("micrio-physics-simulator")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("libsdl2")