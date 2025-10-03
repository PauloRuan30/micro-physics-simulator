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
                // FIX: Don't modify next_grid here - it was causing the flicker bug!
            }
        }
    }
}

void Simulation::step_once() {
    // Copy grid to next_grid as starting point
    std::copy(grid.begin(), grid.end(), next_grid.begin());

    // Process bottom-up for proper falling behavior
    for (int y = H - 1; y >= 0; --y) {
        // Randomize left-right iteration to reduce bias
        bool lr = (std::rand() & 1);
        for (int xi = 0; xi < W; ++xi) {
            int x = lr ? xi : (W - 1 - xi);
            uint8_t cell = grid[idx(x,y)];
            
            if (cell == EMPTY || cell == WALL) continue;

            // SAND behavior: falls and displaces water
            if (cell == SAND) {
                // Try straight down
                if (y + 1 < H) {
                    uint8_t below = next_grid[idx(x, y+1)];
                    if (below == EMPTY) {
                        next_grid[idx(x, y+1)] = SAND;
                        next_grid[idx(x, y)] = EMPTY;
                        continue;
                    } else if (below == WATER) {
                        // Sand sinks through water
                        next_grid[idx(x, y+1)] = SAND;
                        next_grid[idx(x, y)] = WATER;
                        continue;
                    }
                }

                // Try diagonal down
                int dir = (std::rand() & 1) ? -1 : 1;
                for (int i = 0; i < 2; ++i) {
                    int dx = (i == 0) ? dir : -dir;
                    int nx = x + dx, ny = y + 1;
                    if (nx >= 0 && nx < W && ny < H) {
                        uint8_t target = next_grid[idx(nx, ny)];
                        if (target == EMPTY) {
                            next_grid[idx(nx, ny)] = SAND;
                            next_grid[idx(x, y)] = EMPTY;
                            break;
                        } else if (target == WATER) {
                            next_grid[idx(nx, ny)] = SAND;
                            next_grid[idx(x, y)] = WATER;
                            break;
                        }
                    }
                }
            }

            // WATER behavior: flows more like liquid
            else if (cell == WATER) {
                // Try straight down
                if (y + 1 < H && next_grid[idx(x, y+1)] == EMPTY) {
                    next_grid[idx(x, y+1)] = WATER;
                    next_grid[idx(x, y)] = EMPTY;
                    continue;
                }

                // Try diagonal down
                int dir = (std::rand() & 1) ? -1 : 1;
                bool moved = false;
                for (int i = 0; i < 2; ++i) {
                    int dx = (i == 0) ? dir : -dir;
                    int nx = x + dx, ny = y + 1;
                    if (nx >= 0 && nx < W && ny < H && next_grid[idx(nx, ny)] == EMPTY) {
                        next_grid[idx(nx, ny)] = WATER;
                        next_grid[idx(x, y)] = EMPTY;
                        moved = true;
                        break;
                    }
                }
                if (moved) continue;

                // Horizontal spread (makes water flow more liquid-like)
                int dirs[2] = { -1, 1 };
                if (std::rand() & 1) { dirs[0] = 1; dirs[1] = -1; }
                
                // Try spreading up to 2 cells away
                for (int range = 1; range <= 2; ++range) {
                    for (int d = 0; d < 2; ++d) {
                        int nx = x + dirs[d] * range;
                        if (nx >= 0 && nx < W && next_grid[idx(nx, y)] == EMPTY) {
                            // Check if path is clear
                            bool clear = true;
                            for (int r = 1; r < range; ++r) {
                                int check_x = x + dirs[d] * r;
                                if (next_grid[idx(check_x, y)] != EMPTY && 
                                    next_grid[idx(check_x, y)] != WATER) {
                                    clear = false;
                                    break;
                                }
                            }
                            if (clear) {
                                next_grid[idx(nx, y)] = WATER;
                                next_grid[idx(x, y)] = EMPTY;
                                moved = true;
                                break;
                            }
                        }
                    }
                    if (moved) break;
                }
            }
        }
    }

    grid.swap(next_grid);
}