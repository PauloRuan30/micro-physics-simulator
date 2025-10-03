// materials.h
#ifndef MATERIALS_H
#define MATERIALS_H

#include <cstdint>

// Material IDs
enum Material : uint8_t {
    EMPTY = 0,
    SAND  = 1,
    WATER = 2,
    WALL  = 3
};

// Colors in 0xAARRGGBB (alpha first). Make alpha 0xFF (opaque).
static const uint32_t COLOR_EMPTY = 0xFF000000u; // black background (opaque)
static const uint32_t COLOR_SAND  = 0xFFFFCC66u; // sand
static const uint32_t COLOR_WATER = 0xFF66CCFFu; // water (light blue)
static const uint32_t COLOR_WALL  = 0xFF888888u; // wall (grey)

static inline uint32_t material_color(uint8_t m) {
    switch(m) {
        case SAND:  return COLOR_SAND;
        case WATER: return COLOR_WATER;
        case WALL:  return COLOR_WALL;
        case EMPTY:
        default:    return COLOR_EMPTY;
    }
}

#endif // MATERIALS_H