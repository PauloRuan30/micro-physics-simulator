#include <SDL.h>
#include <vector>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <random>
#include <iostream>

// GRID configuration (tweak for performance/visibility)
static const int GRID_W = 320; // cells horizontally
static const int GRID_H = 200; // cells vertically
static const int SCALE = 2;    // render scale per cell (pixel size)

enum Cell : uint8_t
{
    EMPTY = 0,
    SAND = 1
};

inline int idx(int x, int y) { return y * GRID_W + x; }

int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    const int win_w = GRID_W * SCALE;
    const int win_h = GRID_H * SCALE;

    SDL_Window *window = SDL_CreateWindow("Falling Sand (xmake + SDL2)",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          win_w, win_h,
                                          SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::cerr << "CreateWindow: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cerr << "CreateRenderer: " << SDL_GetError() << "\n";
        return 1;
    }

    // We'll use an ARGB8888 streaming texture sized to GRID_W x GRID_H
    SDL_Texture *tex = SDL_CreateTexture(renderer,
                                         SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         GRID_W, GRID_H);
    if (!tex)
    {
        std::cerr << "CreateTexture: " << SDL_GetError() << "\n";
        return 1;
    }

    // Simulation buffers
    std::vector<uint8_t> grid(GRID_W * GRID_H, EMPTY);
    std::vector<uint8_t> next_grid(GRID_W * GRID_H, EMPTY);

    // Pixel buffer for the texture (ARGB8888)
    std::vector<uint32_t> pixels(GRID_W * GRID_H, 0xFF000000);

    bool running = true;
    bool paused = false;

    int brush = 3;
    bool mouse_down = false;

    std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> r01(0, 1);

    const uint32_t sand_color = 0xFFFFCC66;  // ARGB
    const uint32_t empty_color = 0xFF000000; // black

    Uint32 last_tick = SDL_GetTicks();
    const Uint32 target_ms = 1000 / 60; // ~60 fps

    while (running)
    {
        Uint32 t0 = SDL_GetTicks();

        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
                running = false;
            else if (ev.type == SDL_KEYDOWN)
            {
                if (ev.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
                else if (ev.key.keysym.sym == SDLK_SPACE)
                    paused = !paused;
            }