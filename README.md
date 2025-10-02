Dependencies:
- xmake (https://xmake.io)
- SDL2 development libraries installed for your platform
- Debian/Ubuntu: sudo apt install libsdl2-dev
- macOS (homebrew): brew install sdl2
- Windows: install SDL2 dev headers/libs or use vcpkg/win package manager


Build & Run:
xmake
xmake run sand


Controls while running:
- Left mouse drag: spawn sand
- Space: pause/resume
- C: clear grid
- Up / Down: increase / decrease brush size
- Esc: exit


Notes:
- Default grid: 320 x 200 (choose lower/higher to trade off speed vs fidelity)
- Window scales grid by a per-pixel scale factor so each cell is visible.