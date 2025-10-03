#include <SDL2/SDL.h>
#include <iostream>
#include <string>
#include <cmath>
#include "simulation.h"
#include "materials.h"

const int GRID_W = 320;
const int GRID_H = 180;
const int SCALE = 3;

static void upload_texture(SDL_Texture* tex, Simulation &sim) {
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

static void draw_filled_circle(SDL_Renderer* ren, int cx, int cy, int radius, 
                                uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, r, g, b, a);
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx*dx + dy*dy <= radius * radius) {
                SDL_RenderDrawPoint(ren, cx + dx, cy + dy);
            }
        }
    }
}

static void draw_circle_outline(SDL_Renderer* ren, int cx, int cy, int radius,
                                 uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, r, g, b, a);
    for (int a = 0; a < 360; a += 2) {
        float rad = a * 3.14159265358979323846f / 180.0f;
        int px = cx + static_cast<int>(radius * std::cos(rad));
        int py = cy + static_cast<int>(radius * std::sin(rad));
        SDL_RenderDrawPoint(ren, px, py);
    }
}

static void draw_digit(SDL_Renderer* ren, int x, int y, int digit, int size) {
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    
    bool segs[10][7] = {
        {1,1,1,1,1,1,0}, {0,1,1,0,0,0,0}, {1,1,0,1,1,0,1}, {1,1,1,1,0,0,1}, {0,1,1,0,0,1,1},
        {1,0,1,1,0,1,1}, {1,0,1,1,1,1,1}, {1,1,1,0,0,0,0}, {1,1,1,1,1,1,1}, {1,1,1,1,0,1,1}
    };
    
    if (digit < 0 || digit > 9) return;
    
    int w = size * 2;
    int h = size * 3;
    SDL_Rect rects[7] = {
        {x, y, w, size/2}, {x+w-size/2, y, size/2, h/2}, {x+w-size/2, y+h/2, size/2, h/2},
        {x, y+h-size/2, w, size/2}, {x, y+h/2, size/2, h/2}, {x, y, size/2, h/2},
        {x, y+h/2-size/4, w, size/2}
    };
    
    for (int i = 0; i < 7; ++i) {
        if (segs[digit][i]) SDL_RenderFillRect(ren, &rects[i]);
    }
}

static void draw_number(SDL_Renderer* ren, int x, int y, int num, int size) {
    if (num > 99) num = 99;
    if (num < 0) num = 0;
    if (num >= 10) {
        draw_digit(ren, x, y, num / 10, size);
        draw_digit(ren, x + size * 3, y, num % 10, size);
    } else {
        draw_digit(ren, x, y, num, size);
    }
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    int window_w = GRID_W * SCALE;
    int window_h = GRID_H * SCALE;
    SDL_Window* win = SDL_CreateWindow("Physics Sim [1-Sand 2-Water 3-Wall 4-Erase C-Clear]",
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

    sim.set_paint_material(SAND);

    // Initial ground
    for (int x = 0; x < sim.W; ++x) {
        sim.grid[sim.W * (sim.H-1) + x] = WALL;
    }

    Uint32 last = SDL_GetTicks();
    const int TARGET_FPS = 60;
    const int STEP_PER_FRAME = 2;
    
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
            } else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_ESCAPE) running = false;
                else if (k == SDLK_1) sim.set_paint_material(SAND);
                else if (k == SDLK_2) sim.set_paint_material(WATER);
                else if (k == SDLK_3) sim.set_paint_material(WALL);
                else if (k == SDLK_4) sim.set_paint_material(EMPTY);
                else if (k == SDLK_UP || k == SDLK_EQUALS || k == SDLK_PLUS) {
                    sim.set_brush_radius(std::min(50, sim.brush_radius + 1));
                }
                else if (k == SDLK_DOWN || k == SDLK_MINUS) {
                    sim.set_brush_radius(std::max(1, sim.brush_radius - 1));
                }
                else if (k == SDLK_c) {
                    std::fill(sim.grid.begin(), sim.grid.end(), EMPTY);
                }
            }
        }

        int paint_grid_x = mouse_x / SCALE;
        int paint_grid_y = mouse_y / SCALE;

        if (mouse_down) {
            sim.paint_at(paint_grid_x, paint_grid_y);
        } else if (right_down) {
            uint8_t prev = sim.paint_material;
            sim.set_paint_material(EMPTY);
            sim.paint_at(paint_grid_x, paint_grid_y);
            sim.set_paint_material(prev);
        }

        for (int i = 0; i < STEP_PER_FRAME; ++i) {
            sim.step_once();
        }

        upload_texture(tex, sim);

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_Rect dst = { 0, 0, sim.W * SCALE, sim.H * SCALE };
        SDL_RenderCopy(ren, tex, nullptr, &dst);

        // Brush preview
        int brush_px = sim.brush_radius * SCALE;
        uint32_t mat_color = material_color(sim.paint_material);
        uint8_t r = (mat_color >> 16) & 0xFF;
        uint8_t g = (mat_color >> 8) & 0xFF;
        uint8_t b = mat_color & 0xFF;
        
        draw_filled_circle(ren, mouse_x, mouse_y, brush_px, r, g, b, 60);
        draw_circle_outline(ren, mouse_x, mouse_y, brush_px, 255, 255, 255, 200);
        draw_circle_outline(ren, mouse_x, mouse_y, brush_px + 1, 0, 0, 0, 200);

        // UI: Brush size indicator
        SDL_Rect bg_rect = {10, 10, 60, 40};
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
        SDL_RenderFillRect(ren, &bg_rect);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderDrawRect(ren, &bg_rect);
        draw_number(ren, 20, 18, sim.brush_radius, 3);

        // Material color indicator
        SDL_Rect mat_rect = {10, 60, 80, 25};
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
        SDL_RenderFillRect(ren, &mat_rect);
        SDL_SetRenderDrawColor(ren, r, g, b, 255);
        SDL_Rect color_rect = {15, 65, 15, 15};
        SDL_RenderFillRect(ren, &color_rect);

        SDL_RenderPresent(ren);

        Uint32 now = SDL_GetTicks();
        Uint32 elapsed = now - last;
        if (elapsed < 1000 / TARGET_FPS) {
            SDL_Delay((1000 / TARGET_FPS) - elapsed);
        }
        last = SDL_GetTicks();
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}