# Micro-Physics Simulator

A simple falling sand simulation with multiple materials built using C++ and SDL2.

## Features

- **Multiple Materials**: Sand, Water, Wall, and Air/Eraser
- **Realistic Physics**: Sand falls and piles up, water flows like liquid
- **Interactive Painting**: Draw materials with adjustable brush size
- **Real-time Simulation**: 60 FPS with smooth rendering

## Controls

### Material Selection
- `1` - Sand (falls and piles up)
- `2` - Water (flows and spreads)
- `3` - Wall (static obstacle)
- `4` - Air/Eraser (remove materials)

### Brush Controls
- `UP` or `+` - Increase brush size
- `DOWN` or `-` - Decrease brush size

### Mouse Controls
- `Left Click + Drag` - Paint selected material
- `Right Click + Drag` - Erase

### Other
- `C` - Clear screen
- `ESC` - Exit

## Building

### Requirements
- C++17 compiler
- SDL2 library
- xmake build system

### Build Instructions

```bash
# Install xmake
curl -fsSL https://xmake.io/shget.text | bash

# Build project
xmake

# Run
xmake run 
```

## Project Structure

```
.
├── src/
│   ├── main.cpp           # Main application and rendering
│   ├── simulation.cpp     # Physics simulation logic
│   ├── simulation.h       # Simulation class declaration
│   └── materials.h        # Material definitions and colors
└── xmake.lua             # Build configuration
```

## How It Works

The simulation uses a cellular automaton approach:
- **Sand**: Falls downward, can displace water, piles up when blocked
- **Water**: Falls downward, spreads horizontally to simulate liquid flow
- **Wall**: Static material that blocks other materials
- **Empty**: Represents air/empty space

Each frame updates the grid based on simple rules, creating emergent behavior.
