// main.cpp
#include <SDL2/SDL.h>
#include <iostream>
#include <string>
#include "simulation.h"
#include "materials.h"

// Simulation & rendering parameters
const int GRID_W = 320;
const int GRID_H = 180;
const int SCALE = 3; // pixels per grid cell

// helper: create SDL texture from grid (ARGB32)
static void upload_texture(SDL_Texture* tex, Simulation &sim) {
    // texture format expects 32-bit pixels in format SDL_PIXELFORMAT_ARGB8888
    void* pixels;
    int pitch;
    if (SDL_LockTexture(tex, nullptr, &pixels, &pitch) != 0) {
        std::cerr << "SDL_LockTexture error: " << SDL_GetError() << "\n";
        return;
    }
    uint32_t* dst = static_cast<uint32_t*>(pixels);
    for (int y = 0; y < sim.H; ++y) {
        for (int x = 0; x < sim.W; ++x) {
            uint8_t m = sim.grid[y * sim.W + x];
            dst[y * (pitch / 4) + x] = material_color(m);
        }
    }
    SDL_UnlockTexture(tex);
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    int window_w = GRID_W * SCALE;
    int window_h = GRID_H * SCALE;
    SDL_Window* win = SDL_CreateWindow("Micro-Physics Simulator",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       window_w, window_h, SDL_WINDOW_SHOWN);
    if (!win) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    Simulation sim(GRID_W, GRID_H);

    // create texture to upload grid (ARGB)
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING, sim.W, sim.H);
    if (!tex) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);

    bool running = true;
    bool mouse_down = false;
    bool right_down = false;
    int mouse_x = 0, mouse_y = 0;
    int paint_grid_x = 0, paint_grid_y = 0;

    // initial paint material = SAND
    sim.set_paint_material(SAND);

    // Simple initial scene (optional)
    for (int x = sim.W/4; x < sim.W*3/4; ++x) {
        sim.grid[sim.idx(x, sim.H-1)] = WALL;
    }

    Uint32 last = SDL_GetTicks();
    const int TARGET_FPS = 60;
    const int STEP_PER_FRAME = 1; // simulation steps per frame (tune)
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) mouse_down = true;
                if (e.button.button == SDL_BUTTON_RIGHT) right_down = true;
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                if (e.button.button == SDL_BUTTON_LEFT) mouse_down = false;
                if (e.button.button == SDL_BUTTON_RIGHT) right_down = false;
            } else if (e.type == SDL_MOUSEMOTION) {
                mouse_x = e.motion.x;
                mouse_y = e.motion.y;
                // convert to grid coords
                paint_grid_x = mouse_x / SCALE;
                paint_grid_y = mouse_y / SCALE;
            } else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_ESCAPE) running = false;
                else if (k == SDLK_1) sim.set_paint_material(SAND);
                else if (k == SDLK_2) sim.set_paint_material(WATER);
                else if (k == SDLK_3) sim.set_paint_material(WALL);
                else if (k == SDLK_4) sim.set_paint_material(EMPTY); // air / eraser
                else if (k == SDLK_UP) sim.set_brush_radius(std::min(50, sim.brush_radius + 1));
                else if (k == SDLK_DOWN) sim.set_brush_radius(std::max(1, sim.brush_radius - 1));
            }
        }

        // painting while holding mouse
        if (mouse_down) {
            sim.paint_at(paint_grid_x, paint_grid_y);
        } else if (right_down) {
            // right mouse: erase
            uint8_t prev = sim.paint_material;
            sim.set_paint_material(EMPTY);
            sim.paint_at(paint_grid_x, paint_grid_y);
            sim.set_paint_material(prev);
        }

        // do a few simulation steps per frame
        for (int i = 0; i < STEP_PER_FRAME; ++i) sim.step_once();

        // upload sim to texture
        upload_texture(tex, sim);

        // render scaled texture
        SDL_RenderClear(ren);
        SDL_Rect dst = { 0, 0, sim.W * SCALE, sim.H * SCALE };
        SDL_RenderCopy(ren, tex, nullptr, &dst);

        // draw brush preview (circle) in screen/pixel coords (before present)
        int brush_px = sim.brush_radius * SCALE;
        int cx = mouse_x;
        int cy = mouse_y;
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        // translucent white fill
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 80);
        for (int dy = -brush_px; dy <= brush_px; ++dy) {
            for (int dx = -brush_px; dx <= brush_px; ++dx) {
                if (dx*dx + dy*dy <= brush_px * brush_px) {
                    SDL_RenderDrawPoint(ren, cx + dx, cy + dy);
                }
            }
        }
        // outline to make it readable
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 200);
        for (int a = 0; a < 360; ++a) {
            float rad = a * 3.14159265358979323846f / 180.0f;
            int px = cx + int(brush_px * cos(rad));
            int py = cy + int(brush_px * sin(rad));
            SDL_RenderDrawPoint(ren, px, py);
        }

        // draw brush size as number (simple rectangle + number using SDL_RenderDraw* because we don't have text)
        // For now draw a small box with the number using rectangle + ascii art is not trivial,
        // so we just draw circles; if you want text install SDL_ttf and draw the number.
        // (This comment tells you how to extend.)

        SDL_RenderPresent(ren);

        // simple framerate cap
        Uint32 now = SDL_GetTicks();
        Uint32 elapsed = now - last;
        if (elapsed < 1000 / TARGET_FPS) SDL_Delay( (1000 / TARGET_FPS) - elapsed );
        last = SDL_GetTicks();
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
