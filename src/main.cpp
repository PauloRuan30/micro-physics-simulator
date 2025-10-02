// main.cpp
#include "simulation.h"
#include "materials.h"
#include <SDL.h>
#include <vector>
#include <iostream>
#include <cmath>

static int GRID_W = 320;
static int GRID_H = 200;
static int SCALE = 2;

// helper: draw filled circular brush preview (grid-cell aligned)
static void draw_brush_preview(SDL_Renderer* renderer, int mx, int my, int brush, int scale) {
    // convert mouse pixel -> grid coords
    int gx = mx / scale;
    int gy = my / scale;

    // semi-transparent color
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 96); // white translucent

    for (int dy = -brush; dy <= brush; ++dy) {
        for (int dx = -brush; dx <= brush; ++dx) {
            if (dx*dx + dy*dy <= brush*brush) {
                int x = gx + dx;
                int y = gy + dy;
                if (x < 0 || x >= GRID_W || y < 0 || y >= GRID_H) continue;
                SDL_Rect r{ x * scale, y * scale, scale, scale };
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }
    // reset blend
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Sand+Water+Wall+AR - multi-material",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        GRID_W * SCALE, GRID_H * SCALE, SDL_WINDOW_SHOWN);
    if (!window) { std::cerr << "CreateWindow: " << SDL_GetError() << "\n"; SDL_Quit(); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { std::cerr << "CreateRenderer: " << SDL_GetError() << "\n"; SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    SDL_Texture* tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        GRID_W, GRID_H);
    if (!tex) { std::cerr << "CreateTexture: " << SDL_GetError() << "\n"; SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    Simulation sim(GRID_W, GRID_H);
    std::vector<uint32_t> pixels(GRID_W * GRID_H);

    bool running = true;
    bool paused = false;
    bool mouse_down = false;
    int brush = 3;
    int frame_count = 0;
    const Uint32 target_ms = 1000 / 60;

    std::cout << "Controls:\n";
    std::cout << " Left-drag: paint\n 1=SAND 2=WATER 3=WALL 4=AR\n V=view-mode  Space=pause  .=step  C=clear\n Up/Down=brush  [/]=inclusion density down/up\n\n";

    while (running) {
        Uint32 t0 = SDL_GetTicks();
        SDL_Event ev;
        int mouse_x = 0, mouse_y = 0;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            else if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_SPACE: paused = !paused; break;
                    case SDLK_c: sim.clear(); break;
                    case SDLK_UP: brush = std::min(50, brush + 1); break;
                    case SDLK_DOWN: brush = std::max(1, brush - 1); break;
                    case SDLK_1: sim.set_paint_material(SAND); std::cout << "Paint=SAND\n"; break;
                    case SDLK_2: sim.set_paint_material(WATER); std::cout << "Paint=WATER\n"; break;
                    case SDLK_3: sim.set_paint_material(WALL); std::cout << "Paint=WALL\n"; break;
                    case SDLK_4: sim.set_paint_material(AR); std::cout << "Paint=AR\n"; break;
                    case SDLK_v: sim.set_view_mode((sim.get_view_mode()+1)%5); std::cout << "View=" << sim.get_view_mode() << "\n"; break;
                    case SDLK_PERIOD: { paused = true; sim.step(1, frame_count); } break; // single-step
                    case SDLK_LEFTBRACKET: sim.set_inclusion_chance(std::max(0.0, sim.get_inclusion_chance() - 0.001)); std::cout << "incl=" << sim.get_inclusion_chance() << "\n"; break;
                    case SDLK_RIGHTBRACKET: sim.set_inclusion_chance(std::min(0.1, sim.get_inclusion_chance() + 0.001)); std::cout << "incl=" << sim.get_inclusion_chance() << "\n"; break;
                    default: break;
                }
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                if (ev.button.button == SDL_BUTTON_LEFT) mouse_down = true;
            } else if (ev.type == SDL_MOUSEBUTTONUP) {
                if (ev.button.button == SDL_BUTTON_LEFT) mouse_down = false;
            }
        }

        SDL_GetMouseState(&mouse_x, &mouse_y);

        if (mouse_down) {
            int gx = mouse_x / SCALE;
            int gy = mouse_y / SCALE;
            sim.paint_at(gx, gy, brush);
        }

        if (!paused) {
            sim.step(2, frame_count);
        }

        // fill pixels by querying simulation
        for (int y = 0; y < GRID_H; ++y) {
            for (int x = 0; x < GRID_W; ++x) {
                pixels[y * GRID_W + x] = sim.pixel_color_at(x, y);
            }
        }

        SDL_UpdateTexture(tex, nullptr, pixels.data(), GRID_W * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_Rect dst{0,0, GRID_W * SCALE, GRID_H * SCALE};
        SDL_RenderCopy(renderer, tex, nullptr, &dst);

        // draw brush preview (if not clicking, or even while clicking)
        draw_brush_preview(renderer, mouse_x, mouse_y, brush, SCALE);

        // draw brush size numeric indicator (top-left)
        // simple textless numeric indicator: small rectangle per size (or you can print to console)
        SDL_SetRenderDrawColor(renderer, 255,255,255,200);
        SDL_Rect ui{8,8, 6 + brush, 10};
        SDL_RenderFillRect(renderer, &ui);

        SDL_RenderPresent(renderer);

        Uint32 t1 = SDL_GetTicks();
        Uint32 elapsed = t1 - t0;
        if (elapsed < target_ms) SDL_Delay(target_ms - elapsed);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
