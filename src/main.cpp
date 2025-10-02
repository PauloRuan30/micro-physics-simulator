#include <SDL.h>
#include <vector>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <random>
#include <iostream>
#include <algorithm>

static const int GRID_W = 320;   // adjust for performance/fidelity
static const int GRID_H = 200;
static const int SCALE  = 2;     // pixel scale

enum Cell : uint8_t { EMPTY = 0, SAND = 1 };

inline int idx(int x, int y) { return y * GRID_W + x; }

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    const int win_w = GRID_W * SCALE;
    const int win_h = GRID_H * SCALE;

    SDL_Window* window = SDL_CreateWindow("Falling Sand (xmake + SDL2)",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          win_w, win_h,
                                          SDL_WINDOW_SHOWN);
    if (!window) { std::cerr << "CreateWindow: " << SDL_GetError() << "\n"; SDL_Quit(); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { std::cerr << "CreateRenderer: " << SDL_GetError() << "\n"; SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    SDL_Texture* tex = SDL_CreateTexture(renderer,
                                         SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         GRID_W, GRID_H);
    if (!tex) { std::cerr << "CreateTexture: " << SDL_GetError() << "\n"; SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    std::vector<uint8_t> grid(GRID_W * GRID_H, EMPTY);
    std::vector<uint8_t> next_grid(GRID_W * GRID_H, EMPTY);
    std::vector<uint32_t> pixels(GRID_W * GRID_H, 0xFF000000);

    bool running = true;
    bool paused = false;

    int brush = 3;
    bool mouse_down = false;

    // RNG for tie-breaking and small randomness in movement
    std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> r01(0, 1);

    const uint32_t sand_color = 0xFFFFCC66; // ARGB
    const uint32_t empty_color = 0xFF000000; // black

    Uint32 last_tick = SDL_GetTicks();
    const Uint32 target_ms = 1000 / 60; // ~60 fps

    int frame_count = 0; // used to alternate scan direction

    while (running) {
        Uint32 t0 = SDL_GetTicks();

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            else if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) running = false;
                else if (ev.key.keysym.sym == SDLK_SPACE) paused = !paused;
                else if (ev.key.keysym.sym == SDLK_c) {
                    std::fill(grid.begin(), grid.end(), EMPTY);
                } else if (ev.key.keysym.sym == SDLK_UP) {
                    brush = std::min(50, brush + 1);
                } else if (ev.key.keysym.sym == SDLK_DOWN) {
                    brush = std::max(1, brush - 1);
                }
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                if (ev.button.button == SDL_BUTTON_LEFT) mouse_down = true;
            } else if (ev.type == SDL_MOUSEBUTTONUP) {
                if (ev.button.button == SDL_BUTTON_LEFT) mouse_down = false;
            }
        }

        // Paint sand under mouse if dragging
        if (mouse_down) {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            int gx = mx / SCALE;
            int gy = my / SCALE;
            for (int dy = -brush; dy <= brush; ++dy) {
                for (int dx = -brush; dx <= brush; ++dx) {
                    int x = gx + dx;
                    int y = gy + dy;
                    if (x < 0 || x >= GRID_W || y < 0 || y >= GRID_H) continue;
                    if (dx*dx + dy*dy <= brush*brush) {
                        grid[idx(x,y)] = SAND;
                    }
                }
            }
        }

        // -----------------------------
        // Updated simulation block:
        // substeps, sideways sliding, alternating scan order
        // -----------------------------
        if (!paused) {
            const int substeps = 2; // tune: 1..5 (higher = more cascading but more CPU)
            for (int s = 0; s < substeps; ++s) {
                std::fill(next_grid.begin(), next_grid.end(), EMPTY);

                bool left_to_right = (((frame_count + s) & 1) == 0);

                for (int y = GRID_H - 1; y >= 0; --y) {
                    if (left_to_right) {
                        for (int x = 0; x < GRID_W; ++x) {
                            int i = idx(x, y);
                            if (grid[i] != SAND) continue;

                            // try move down
                            if (y + 1 < GRID_H && grid[idx(x, y + 1)] == EMPTY && next_grid[idx(x, y + 1)] == EMPTY) {
                                next_grid[idx(x, y + 1)] = SAND;
                                continue;
                            }

                            // try down-left / down-right (random tie-break)
                            bool moved = false;
                            int dir = r01(rng) ? -1 : 1;
                            for (int k = 0; k < 2; ++k) {
                                int dx = (k == 0) ? dir : -dir;
                                int nx = x + dx;
                                int ny = y + 1;
                                if (nx >= 0 && nx < GRID_W && ny < GRID_H) {
                                    if (grid[idx(nx, ny)] == EMPTY && next_grid[idx(nx, ny)] == EMPTY) {
                                        next_grid[idx(nx, ny)] = SAND;
                                        moved = true;
                                        break;
                                    }
                                }
                            }

                            // sideways sliding fallback
                            if (!moved) {
                                int sdir = r01(rng) ? -1 : 1;
                                if (x + sdir >= 0 && x + sdir < GRID_W
                                    && grid[idx(x + sdir, y)] == EMPTY && next_grid[idx(x + sdir, y)] == EMPTY) {
                                    next_grid[idx(x + sdir, y)] = SAND;
                                    moved = true;
                                } else if (x - sdir >= 0 && x - sdir < GRID_W
                                           && grid[idx(x - sdir, y)] == EMPTY && next_grid[idx(x - sdir, y)] == EMPTY) {
                                    next_grid[idx(x - sdir, y)] = SAND;
                                    moved = true;
                                }
                            }

                            if (!moved) {
                                if (next_grid[i] == EMPTY) next_grid[i] = SAND;
                            }
                        }
                    } else {
                        // right-to-left scan
                        for (int x = GRID_W - 1; x >= 0; --x) {
                            int i = idx(x, y);
                            if (grid[i] != SAND) continue;

                            // try move down
                            if (y + 1 < GRID_H && grid[idx(x, y + 1)] == EMPTY && next_grid[idx(x, y + 1)] == EMPTY) {
                                next_grid[idx(x, y + 1)] = SAND;
                                continue;
                            }

                            // try down-left / down-right (random tie-break)
                            bool moved = false;
                            int dir = r01(rng) ? -1 : 1;
                            for (int k = 0; k < 2; ++k) {
                                int dx = (k == 0) ? dir : -dir;
                                int nx = x + dx;
                                int ny = y + 1;
                                if (nx >= 0 && nx < GRID_W && ny < GRID_H) {
                                    if (grid[idx(nx, ny)] == EMPTY && next_grid[idx(nx, ny)] == EMPTY) {
                                        next_grid[idx(nx, ny)] = SAND;
                                        moved = true;
                                        break;
                                    }
                                }
                            }

                            // sideways sliding fallback
                            if (!moved) {
                                int sdir = r01(rng) ? -1 : 1;
                                if (x + sdir >= 0 && x + sdir < GRID_W
                                    && grid[idx(x + sdir, y)] == EMPTY && next_grid[idx(x + sdir, y)] == EMPTY) {
                                    next_grid[idx(x + sdir, y)] = SAND;
                                    moved = true;
                                } else if (x - sdir >= 0 && x - sdir < GRID_W
                                           && grid[idx(x - sdir, y)] == EMPTY && next_grid[idx(x - sdir, y)] == EMPTY) {
                                    next_grid[idx(x - sdir, y)] = SAND;
                                    moved = true;
                                }
                            }

                            if (!moved) {
                                if (next_grid[i] == EMPTY) next_grid[i] = SAND;
                            }
                        }
                    }
                } // end y-loop

                grid.swap(next_grid);
            } // end substeps

            ++frame_count;
        } // end paused check

        // update pixel buffer
        for (int y = 0; y < GRID_H; ++y) {
            for (int x = 0; x < GRID_W; ++x) {
                pixels[idx(x,y)] = (grid[idx(x,y)] == SAND) ? sand_color : empty_color;
            }
        }

        // upload pixels to texture
        SDL_UpdateTexture(tex, nullptr, pixels.data(), GRID_W * sizeof(uint32_t));

        // render scaled texture
        SDL_RenderClear(renderer);
        SDL_Rect dst{0,0, GRID_W * SCALE, GRID_H * SCALE};
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_RenderPresent(renderer);

        // frame cap
        Uint32 t1 = SDL_GetTicks();
        Uint32 elapsed = t1 - t0;
        if (elapsed < target_ms) SDL_Delay(target_ms - elapsed);
    } // end main loop

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
