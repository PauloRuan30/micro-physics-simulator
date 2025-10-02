
#include <SDL.h>
#include <vector>
#include <random>
#include <iostream>

// Grid constants
const int GRID_WIDTH = 320;
const int GRID_HEIGHT = 200;
const int PIXEL_SCALE = 2; // Scale up for visibility

// SDL Window and Renderer
SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
SDL_Texture* gTexture = nullptr;

// Grid data (0 = empty, 1 = sand)
std::vector<uint8_t> gGrid(GRID_WIDTH * GRID_HEIGHT, 0);
std::vector<uint8_t> gNextGrid(GRID_WIDTH * GRID_HEIGHT, 0);

// Random number generator for tie-breaking
std::mt19937 gRNG(std::random_device{}());

// Simulation state
bool gPaused = false;
int gBrushSize = 5;

// Function to initialize SDL
bool init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    gWindow = SDL_CreateWindow("Falling Sand Sandbox", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, GRID_WIDTH * PIXEL_SCALE, GRID_HEIGHT * PIXEL_SCALE, SDL_WINDOW_SHOWN);
    if (gWindow == nullptr)
    {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (gRenderer == nullptr)
    {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);

    gTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GRID_WIDTH, GRID_HEIGHT);
    if (gTexture == nullptr)
    {
        std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

// Function to close SDL
void close()
{
    SDL_DestroyTexture(gTexture);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gTexture = nullptr;
    gRenderer = nullptr;
    gWindow = nullptr;

    SDL_Quit();
}

// Get cell value safely
uint8_t getCell(int x, int y)
{
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT)
        return 0; // Treat out of bounds as empty
    return gGrid[y * GRID_WIDTH + x];
}

// Set cell value safely for the next grid
void setNextCell(int x, int y, uint8_t value)
{
    if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT)
        gNextGrid[y * GRID_WIDTH + x] = value;
}

// Update simulation state (bottom-up)
void updateSimulation()
{
    if (gPaused) return;

    gNextGrid = gGrid; // Start next grid as a copy of current

    for (int y = GRID_HEIGHT - 1; y >= 0; --y) // Bottom-up scan
    {
        for (int x = 0; x < GRID_WIDTH; ++x)
        {
            if (getCell(x, y) == 1) // If it's sand
            {
                // Try to move down
                if (getCell(x, y + 1) == 0)
                {
                    setNextCell(x, y + 1, 1);
                    setNextCell(x, y, 0);
                }
                else // Try to move down-left or down-right
                {
                    bool canMoveLeft = (getCell(x - 1, y + 1) == 0);
                    bool canMoveRight = (getCell(x + 1, y + 1) == 0);

                    if (canMoveLeft && canMoveRight)
                    {
                        // Random tie-break
                        if (std::uniform_int_distribution<>(0, 1)(gRNG) == 0)
                        {
                            setNextCell(x - 1, y + 1, 1);
                            setNextCell(x, y, 0);
                        }
                        else
                        {
                            setNextCell(x + 1, y + 1, 1);
                            setNextCell(x, y, 0);
                        }
                    }
                    else if (canMoveLeft)
                    {
                        setNextCell(x - 1, y + 1, 1);
                        setNextCell(x, y, 0);
                    }
                    else if (canMoveRight)
                    {
                        setNextCell(x + 1, y + 1, 1);
                        setNextCell(x, y, 0);
                    }
                }
            }
        }
    }
    gGrid = gNextGrid; // Swap buffers
}

// Render the grid to the SDL texture
void renderGrid()
{
    uint32_t pixels[GRID_WIDTH * GRID_HEIGHT];
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; ++i)
    {
        if (gGrid[i] == 1)
        {
            pixels[i] = 0xFFC2B280; // Sand color (light brown/yellowish)
        }
        else
        {
            pixels[i] = 0xFF000000; // Empty (black)
        }
    }
    SDL_UpdateTexture(gTexture, nullptr, pixels, GRID_WIDTH * sizeof(uint32_t));
    SDL_RenderCopy(gRenderer, gTexture, nullptr, nullptr);
    SDL_RenderPresent(gRenderer);
}

// Add sand at a specific grid coordinate
void addSand(int gridX, int gridY)
{
    for (int y = -gBrushSize / 2; y <= gBrushSize / 2; ++y)
    {
        for (int x = -gBrushSize / 2; x <= gBrushSize / 2; ++x)
        {
            int targetX = gridX + x;
            int targetY = gridY + y;
            if (targetX >= 0 && targetX < GRID_WIDTH && targetY >= 0 && targetY < GRID_HEIGHT)
            {
                gGrid[targetY * GRID_WIDTH + targetX] = 1;
            }
        }
    }
}

int main(int argc, char* args[])
{
    if (!init())
    {
        std::cerr << "Failed to initialize!" << std::endl;
        return 1;
    }

    bool quit = false;
    SDL_Event e;

    Uint32 lastTick = SDL_GetTicks();
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;

    while (!quit)
    {
        Uint32 currentTick = SDL_GetTicks();
                Uint32 deltaTick = currentTick - lastTick;

        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN)
            {
                switch (e.key.keysym.sym)
                {
                    case SDLK_SPACE:
                        gPaused = !gPaused;
                        std::cout << "Simulation " << (gPaused ? "paused" : "resumed") << std::endl;
                        break;
                    case SDLK_c:
                        std::fill(gGrid.begin(), gGrid.end(), 0);
                        std::fill(gNextGrid.begin(), gNextGrid.end(), 0);
                        std::cout << "Grid cleared" << std::endl;
                        break;
                    case SDLK_UP:
                        gBrushSize = std::min(gBrushSize + 1, 20);
                        std::cout << "Brush size: " << gBrushSize << std::endl;
                        break;
                    case SDLK_DOWN:
                        gBrushSize = std::max(gBrushSize - 1, 1);
                        std::cout << "Brush size: " << gBrushSize << std::endl;
                        break;
                    case SDLK_ESCAPE:
                        quit = true;
                        break;
                }
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEMOTION)
            {
                if (e.button.button == SDL_BUTTON_LEFT && (e.type == SDL_MOUSEBUTTONDOWN || (e.type == SDL_MOUSEMOTION && SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK)))
                {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    addSand(mouseX / PIXEL_SCALE, mouseY / PIXEL_SCALE);
                }
            }
        }

        // Update and render only if enough time has passed for a new frame
        if (deltaTick >= frameDelay)
        {
            updateSimulation();
            renderGrid();
            lastTick = currentTick;
        }
    }

    close();

    return 0;
}

