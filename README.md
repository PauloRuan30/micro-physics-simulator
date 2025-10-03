Dependencies:
 - xmake
 - SDL2 dev libs (apt: libsdl2-dev, brew: sdl2, Windows: vcpkg/choco)

Build & run:
  xmake
  xmake run 

Controls:
 - Left-drag: paint
 - 1 = Sand, 2 = Water, 3 = Wall
 - V = cycle view mode (All / Sand-only / Water-only / Wall-only)
 - Space = pause/resume
 - . = single-step (when paused)
 - C = clear
 - Up/Down = increase/decrease brush
 - [ / ] = decrease / increase inclusion density (black dots in settled sand)
 - Esc = exit
