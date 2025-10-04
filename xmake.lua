add_rules("mode.debug", "mode.release")

add_requires("libsdl2")

target("micro-physics-simulator")
set_kind("binary")
set_languages("c++17")
add_files("src/*.cpp")
add_packages("libsdl2")
