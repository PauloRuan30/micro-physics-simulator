add_rules("mode.debug", "mode.release")

-- require SDL2 (xmake package system)
add_requires("sdl2 >=2.0.0")

target("sand")
set_kind("binary")
set_languages("c++17")
add_files("src/*.cpp")
add_packages("sdl2")