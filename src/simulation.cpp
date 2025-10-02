// simulation.cpp
#include "simulation.h"
#include "materials.h"
#include <random>
#include <algorithm>
#include <cstring>
#include <cassert>
#include <chrono>

// helper
inline int idx(int x, int y, int W) { return y * W + x; }

Simulation::Simulation(int w, int h, unsigned seed)
    : W(w), H(h),
      grid(W * H, EMPTY),
      next_grid(W * H, EMPTY),
      inclusion(W * H, 0),
      inclusion_chance(0.005),
      paint_material(SAND),
      view_mode(0),
      rng_seed(seed)
{
    if (rng_seed == 0) rng_seed = (unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

int Simulation::width() const { return W; }
int Simulation::height() const { return H; }

void Simulation::set_paint_material(Cell m) { paint_material = m; }
Cell Simulation::get_paint_material() const { return paint_material; }
void Simulation::set_view_mode(int vm) { view_mode = vm % 5; }
int Simulation::get_view_mode() const { return view_mode; }

void Simulation::set_inclusion_chance(double p) {
    inclusion_chance = std::max(0.0, std::min(1.0, p));
}
double Simulation::get_inclusion_chance() const { return inclusion_chance; }

void Simulation::clear() {
    std::fill(grid.begin(), grid.end(), EMPTY);
    std::fill(inclusion.begin(), inclusion.end(), 0);
}

// painting (circle brush) writes material into current grid directly
void Simulation::paint_at(int gx, int gy, int brush_radius) {
    for (int dy = -brush_radius; dy <= brush_radius; ++dy) {
        for (int dx = -brush_radius; dx <= brush_radius; ++dx) {
            int x = gx + dx, y = gy + dy;
            if (x < 0 || x >= W || y < 0 || y >= H) continue;
            if (dx*dx + dy*dy <= brush_radius*brush_radius) {
                grid[idx(x,y,W)] = static_cast<uint8_t>(paint_material);
            }
        }
    }
}

// step: performs CA updates. frame_count is incremented by caller (alternation)
void Simulation::step(int substeps, int &frame_count) {
    std::mt19937 rng(rng_seed + frame_count);
    std::uniform_int_distribution<int> r01(0,1);
    std::uniform_real_distribution<double> r01f(0.0, 1.0);

    for (int s = 0; s < substeps; ++s) {
        std::fill(next_grid.begin(), next_grid.end(), EMPTY);
        bool left_to_right = (((frame_count + s) & 1) == 0);

        // bottom-up scan
        for (int y = H - 1; y >= 0; --y) {
            if (left_to_right) {
                for (int x = 0; x < W; ++x) {
                    int i = idx(x,y,W);
                    uint8_t mat = grid[i];
                    if (mat == EMPTY) continue;

                    // WALL is static
                    if (mat == WALL) {
                        next_grid[i] = WALL;
                        continue;
                    }

                    // --- SAND behavior (heavy) ---
                    if (mat == SAND) {
                        // try move down; allow sinking into WATER or AR by swapping
                        if (y + 1 < H) {
                            uint8_t below = grid[idx(x,y+1,W)];
                            if (below == EMPTY && next_grid[idx(x,y+1,W)] == EMPTY) {
                                next_grid[idx(x,y+1,W)] = SAND;
                                continue;
                            }
                            // if below is WATER or AR and next_grid below empty -> swap (sand sinks)
                            if ((below == WATER || below == AR) && next_grid[idx(x,y+1,W)] == EMPTY) {
                                next_grid[idx(x,y+1,W)] = SAND;
                                next_grid[i] = below; // place water/AR where sand was
                                continue;
                            }
                        }

                        // diagonals
                        bool moved = false;
                        int dir = r01(rng) ? -1 : 1;
                        for (int k = 0; k < 2; ++k) {
                            int dx = (k==0) ? dir : -dir;
                            int nx = x + dx, ny = y + 1;
                            if (nx >= 0 && nx < W && ny < H) {
                                uint8_t target = grid[idx(nx,ny,W)];
                                if (target == EMPTY && next_grid[idx(nx,ny,W)] == EMPTY) {
                                    next_grid[idx(nx,ny,W)] = SAND; moved = true; break;
                                }
                                // if diagonal contains WATER or AR, allow sand to move into it (sink)
                                if ((target == WATER || target == AR) && next_grid[idx(nx,ny,W)] == EMPTY) {
                                    next_grid[idx(nx,ny,W)] = SAND;
                                    next_grid[i] = target;
                                    moved = true; break;
                                }
                            }
                        }
                        if (!moved) {
                            // sideways creep
                            int sdir = r01(rng) ? -1 : 1;
                            if (x + sdir >= 0 && x + sdir < W
                                && grid[idx(x+sdir,y,W)] == EMPTY && next_grid[idx(x+sdir,y,W)] == EMPTY) {
                                next_grid[idx(x+sdir,y,W)] = SAND; moved = true;
                            } else if (x - sdir >= 0 && x - sdir < W
                                       && grid[idx(x-sdir,y,W)] == EMPTY && next_grid[idx(x-sdir,y,W)] == EMPTY) {
                                next_grid[idx(x-sdir,y,W)] = SAND; moved = true;
                            }
                        }
                        if (!moved && next_grid[i] == EMPTY) {
                            next_grid[i] = SAND;
                            // mark inclusion overlay when settled on floor/wall/settled sand below
                            bool on_floor = (y == H - 1)
                                || (grid[idx(x,y+1,W)] == WALL)
                                || (grid[idx(x,y+1,W)] == SAND && next_grid[idx(x,y+1,W)] != EMPTY);
                            if (on_floor && inclusion[i] == 0 && r01f(rng) < inclusion_chance) {
                                inclusion[i] = 1;
                            }
                        }
                        continue;
                    }

                    // --- WATER behavior (light liquid) ---
                    if (mat == WATER) {
                        // drop down if empty or AR (let AR replace)
                        if (y + 1 < H) {
                            uint8_t below = grid[idx(x,y+1,W)];
                            if (below == EMPTY && next_grid[idx(x,y+1,W)] == EMPTY) {
                                next_grid[idx(x,y+1,W)] = WATER; continue;
                            }
                            // if below is AR, water will be replaced by AR in physical sense; we avoid moving into AR
                        }
                        // spread sideways first
                        int dir = r01(rng) ? -1 : 1;
                        int lx = x + dir, rx = x - dir;
                        if (lx >= 0 && lx < W && grid[idx(lx,y,W)] == EMPTY && next_grid[idx(lx,y,W)] == EMPTY) {
                            next_grid[idx(lx,y,W)] = WATER; continue;
                        }
                        if (rx >= 0 && rx < W && grid[idx(rx,y,W)] == EMPTY && next_grid[idx(rx,y,W)] == EMPTY) {
                            next_grid[idx(rx,y,W)] = WATER; continue;
                        }
                        // try diagonals
                        for (int k=0;k<2;++k) {
                            int dx = (k==0)? -1: 1;
                            int nx = x + dx, ny = y+1;
                            if (nx >=0 && nx < W && ny < H) {
                                if (grid[idx(nx,ny,W)] == EMPTY && next_grid[idx(nx,ny,W)] == EMPTY) {
                                    next_grid[idx(nx,ny,W)] = WATER; break;
                                }
                            }
                        }
                        if (next_grid[i] == EMPTY) next_grid[i] = WATER;
                        continue;
                    }

                    // --- AR behavior (reactive/acid) ---
                    if (mat == AR) {
                        // AR flows similar to water, but replaces things it moves into (consumes)
                        if (y + 1 < H) {
                            uint8_t below = grid[idx(x,y+1,W)];
                            if ((below == EMPTY || below == SAND || below == WATER) && next_grid[idx(x,y+1,W)] == EMPTY) {
                                // AR moves down, replacing sand/water below
                                next_grid[idx(x,y+1,W)] = AR;
                                // current becomes EMPTY (consumed) or leave as AR? We'll leave current empty so AR flows down.
                                continue;
                            }
                        }
                        // spread sideways (consume)
                        int dir = r01(rng) ? -1 : 1;
                        int lx = x + dir, rx = x - dir;
                        if (lx >= 0 && lx < W && (grid[idx(lx,y,W)] == EMPTY || grid[idx(lx,y,W)] == SAND || grid[idx(lx,y,W)] == WATER)
                            && next_grid[idx(lx,y,W)] == EMPTY) {
                            next_grid[idx(lx,y,W)] = AR; continue;
                        }
                        if (rx >= 0 && rx < W && (grid[idx(rx,y,W)] == EMPTY || grid[idx(rx,y,W)] == SAND || grid[idx(rx,y,W)] == WATER)
                            && next_grid[idx(rx,y,W)] == EMPTY) {
                            next_grid[idx(rx,y,W)] = AR; continue;
                        }
                        // fallback keep AR in place
                        if (next_grid[i] == EMPTY) next_grid[i] = AR;
                        continue;
                    }
                }
            } else {
                // right-to-left (same logic mirrored)
                for (int x = W - 1; x >= 0; --x) {
                    int i = idx(x,y,W);
                    uint8_t mat = grid[i];
                    if (mat == EMPTY) continue;

                    if (mat == WALL) {
                        next_grid[i] = WALL;
                        continue;
                    }

                    if (mat == SAND) {
                        if (y + 1 < H) {
                            uint8_t below = grid[idx(x,y+1,W)];
                            if (below == EMPTY && next_grid[idx(x,y+1,W)] == EMPTY) {
                                next_grid[idx(x,y+1,W)] = SAND; continue;
                            }
                            if ((below == WATER || below == AR) && next_grid[idx(x,y+1,W)] == EMPTY) {
                                next_grid[idx(x,y+1,W)] = SAND;
                                next_grid[i] = below;
                                continue;
                            }
                        }
                        bool moved = false;
                        int dir = r01(rng) ? -1 : 1;
                        for (int k = 0; k < 2; ++k) {
                            int dx = (k==0) ? dir : -dir;
                            int nx = x + dx, ny = y + 1;
                            if (nx >= 0 && nx < W && ny < H) {
                                uint8_t target = grid[idx(nx,ny,W)];
                                if (target == EMPTY && next_grid[idx(nx,ny,W)] == EMPTY) {
                                    next_grid[idx(nx,ny,W)] = SAND; moved = true; break;
                                }
                                if ((target == WATER || target == AR) && next_grid[idx(nx,ny,W)] == EMPTY) {
                                    next_grid[idx(nx,ny,W)] = SAND;
                                    next_grid[i] = target;
                                    moved = true; break;
                                }
                            }
                        }
                        if (!moved) {
                            int sdir = r01(rng) ? -1 : 1;
                            if (x + sdir >= 0 && x + sdir < W
                                && grid[idx(x+sdir,y,W)] == EMPTY && next_grid[idx(x+sdir,y,W)] == EMPTY) {
                                next_grid[idx(x+sdir,y,W)] = SAND; moved = true;
                            } else if (x - sdir >= 0 && x - sdir < W
                                       && grid[idx(x-sdir,y,W)] == EMPTY && next_grid[idx(x-sdir,y,W)] == EMPTY) {
                                next_grid[idx(x-sdir,y,W)] = SAND; moved = true;
                            }
                        }
                        if (!moved && next_grid[i] == EMPTY) {
                            next_grid[i] = SAND;
                            bool on_floor = (y == H - 1)
                                || (grid[idx(x,y+1,W)] == WALL)
                                || (grid[idx(x,y+1,W)] == SAND && next_grid[idx(x,y+1,W)] != EMPTY);
                            if (on_floor && inclusion[i] == 0 && r01f(rng) < inclusion_chance) {
                                inclusion[i] = 1;
                            }
                        }
                        continue;
                    }

                    if (mat == WATER) {
                        if (y + 1 < H) {
                            uint8_t below = grid[idx(x,y+1,W)];
                            if (below == EMPTY && next_grid[idx(x,y+1,W)] == EMPTY) {
                                next_grid[idx(x,y+1,W)] = WATER; continue;
                            }
                        }
                        int dir = r01(rng) ? -1 : 1;
                        int lx = x + dir, rx = x - dir;
                        if (lx >= 0 && lx < W && grid[idx(lx,y,W)] == EMPTY && next_grid[idx(lx,y,W)] == EMPTY) { next_grid[idx(lx,y,W)] = WATER; continue; }
                        if (rx >= 0 && rx < W && grid[idx(rx,y,W)] == EMPTY && next_grid[idx(rx,y,W)] == EMPTY) { next_grid[idx(rx,y,W)] = WATER; continue; }
                        for (int k=0;k<2;++k) {
                            int dx = (k==0)? -1: 1;
                            int nx = x + dx, ny = y+1;
                            if (nx >=0 && nx < W && ny < H) {
                                if (grid[idx(nx,ny,W)] == EMPTY && next_grid[idx(nx,ny,W)] == EMPTY) {
                                    next_grid[idx(nx,ny,W)] = WATER; break;
                                }
                            }
                        }
                        if (next_grid[i] == EMPTY) next_grid[i] = WATER;
                        continue;
                    }

                    if (mat == AR) {
                        if (y + 1 < H) {
                            uint8_t below = grid[idx(x,y+1,W)];
                            if ((below == EMPTY || below == SAND || below == WATER) && next_grid[idx(x,y+1,W)] == EMPTY) {
                                next_grid[idx(x,y+1,W)] = AR; continue;
                            }
                        }
                        int dir = r01(rng) ? -1 : 1;
                        int lx = x + dir, rx = x - dir;
                        if (lx >= 0 && lx < W && (grid[idx(lx,y,W)] == EMPTY || grid[idx(lx,y,W)] == SAND || grid[idx(lx,y,W)] == WATER)
                            && next_grid[idx(lx,y,W)] == EMPTY) { next_grid[idx(lx,y,W)] = AR; continue; }
                        if (rx >= 0 && rx < W && (grid[idx(rx,y,W)] == EMPTY || grid[idx(rx,y,W)] == SAND || grid[idx(rx,y,W)] == WATER)
                            && next_grid[idx(rx,y,W)] == EMPTY) { next_grid[idx(rx,y,W)] = AR; continue; }
                        if (next_grid[i] == EMPTY) next_grid[i] = AR;
                        continue;
                    }
                } // end x loop
            }
        } // end y loop

        // swap buffers for this substep then continue
        grid.swap(next_grid);
    } // end substeps

    ++frame_count;
}

// pixel color for rendering (includes inclusion overlay and view mode)
uint32_t Simulation::pixel_color_at(int x, int y) const {
    assert(x >= 0 && x < W && y >= 0 && y < H);
    uint8_t m = grid[idx(x,y,W)];
    bool incl = (inclusion[idx(x,y,W)] != 0);
    return color_for(static_cast<Cell>(m), incl, view_mode);
}
