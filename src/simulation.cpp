// simulation.cpp
#include "simulation.h"
#include "materials.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>

Simulation::Simulation(int w, int h) : W(w), H(h) {
    grid.assign(W * H, EMPTY);
    next_grid.assign(W * H, EMPTY);
    inclusion.assign(W * H, 0);
    std::srand((unsigned)std::time(nullptr));
}

void Simulation::paint_at(int gx, int gy) {
    for (int dy = -brush_radius; dy <= brush_radius; ++dy) {
        for (int dx = -brush_radius; dx <= brush_radius; ++dx) {
            int x = gx + dx;
            int y = gy + dy;
            if (x < 0 || x >= W || y < 0 || y >= H) continue;
            if (dx*dx + dy*dy <= brush_radius * brush_radius) {
                uint8_t mat = static_cast<uint8_t>(paint_material);
                grid[idx(x,y)] = mat;
                // keep next_grid consistent to avoid one-frame flicker
                if (!next_grid.empty()) next_grid[idx(x,y)] = mat;
                // clear inclusion overlay when erasing
                if (mat == EMPTY && !inclusion.empty()) inclusion[idx(x,y)] = 0;
            }
        }
    }
}

void Simulation::step_once() {
    // initialize next grid to EMPTY (or copy WALLs if you want to preserve them)
    std::fill(next_grid.begin(), next_grid.end(), EMPTY);

    for (int y = 0; y < H; ++y) {
        // iterate left-to-right or right-to-left randomly to reduce bias
        bool lr = (std::rand() & 1);
        for (int xi = 0; xi < W; ++xi) {
            int x = lr ? xi : (W - 1 - xi);
            uint8_t cell = grid[idx(x,y)];
            // If next_grid already occupied from earlier moves, skip (we preserve the occupant)
            if (next_grid[idx(x,y)] != EMPTY) continue;

            // WALL: stays in place
            if (cell == WALL) {
                next_grid[idx(x,y)] = WALL;
                continue;
            }

            // SAND behavior: fall into empty or swap with water (so sand sinks through water)
            if (cell == SAND) {
                bool moved = false;
                // try straight down
                if (y + 1 < H) {
                    uint8_t below = grid[idx(x, y+1)];
                    if (below == EMPTY && next_grid[idx(x, y+1)] == EMPTY) {
                        next_grid[idx(x, y+1)] = SAND;
                        moved = true;
                    } else if (below == WATER && next_grid[idx(x, y+1)] == EMPTY) {
                        // swap with water so sand sinks
                        next_grid[idx(x, y+1)] = SAND;
                        if (next_grid[idx(x,y)] == EMPTY) next_grid[idx(x,y)] = WATER;
                        moved = true;
                    }
                }
                if (moved) continue;

                // try diagonals (random left/right order)
                int dir = (std::rand() & 1) ? -1 : 1;
                for (int i = 0; i < 2 && !moved; ++i) {
                    int dx = (i == 0) ? dir : -dir;
                    int nx = x + dx, ny = y + 1;
                    if (nx < 0 || nx >= W || ny >= H) continue;
                    uint8_t target = grid[idx(nx, ny)];
                    if (target == EMPTY && next_grid[idx(nx, ny)] == EMPTY) {
                        next_grid[idx(nx, ny)] = SAND;
                        moved = true;
                        break;
                    } else if (target == WATER && next_grid[idx(nx, ny)] == EMPTY) {
                        // diagonal swap with water
                        next_grid[idx(nx, ny)] = SAND;
                        if (next_grid[idx(x, y)] == EMPTY) next_grid[idx(x,y)] = WATER;
                        moved = true;
                        break;
                    }
                }
                if (moved) continue;

                // else stays in place
                if (next_grid[idx(x,y)] == EMPTY) next_grid[idx(x,y)] = SAND;
                continue;
            }

            // WATER behavior: fall, then sideways; allow swapping with sand sideways (optional)
            if (cell == WATER) {
                bool moved = false;
                // fall straight down
                if (y + 1 < H) {
                    uint8_t below = grid[idx(x, y+1)];
                    if (below == EMPTY && next_grid[idx(x, y+1)] == EMPTY) {
                        next_grid[idx(x, y+1)] = WATER;
                        moved = true;
                    }
                }
                if (moved) continue;

                // try sideways (randomize L/R)
                int dirs[2] = { -1, 1 };
                if (std::rand() & 1) { dirs[0] = 1; dirs[1] = -1; }
                for (int d = 0; d < 2 && !moved; ++d) {
                    int nx = x + dirs[d];
                    if (nx < 0 || nx >= W) continue;
                    uint8_t target = grid[idx(nx, y)];
                    if (target == EMPTY && next_grid[idx(nx, y)] == EMPTY) {
                        next_grid[idx(nx, y)] = WATER;
                        moved = true;
                        break;
                    }
                    // optional: allow water to swap with SAND sideways so water flows around/under sand
                    if (target == SAND && next_grid[idx(nx,y)] == EMPTY) {
                        next_grid[idx(nx,y)] = WATER;
                        if (next_grid[idx(x,y)] == EMPTY) next_grid[idx(x,y)] = SAND;
                        moved = true;
                        break;
                    }
                }
                if (moved) continue;

                // else stays in place
                if (next_grid[idx(x,y)] == EMPTY) next_grid[idx(x,y)] = WATER;
                continue;
            }

            // EMPTY: nothing, remains empty unless something moved into it earlier (we already check next_grid)
            // Already handled by initialization to EMPTY
        }
    }

    // swap buffers
    grid.swap(next_grid);
    // optional: clear next_grid for next frame (will be filled at beginning)
}
