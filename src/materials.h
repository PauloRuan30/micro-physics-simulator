#pragma once
// materials.h
// Material enumeration / color helper

#include <cstdint>

enum Cell : uint8_t {
    EMPTY = 0,
    SAND  = 1,
    WATER = 2,
    WALL  = 3,
    AR    = 4   // "AR" material (acid / reactive) - replaces/erodes certain materials
};

// view mode: 0 = all, 1 = sand-only, 2 = water-only, 3 = wall-only, 4 = AR-only
inline uint32_t color_for(Cell m, bool inclusion, int view_mode) {
    // ARGB8888
    const uint32_t sand_color = 0xFFFFCC66u;     // yellow
    const uint32_t water_color = 0xFF66CCFFu;    // blue
    const uint32_t wall_color  = 0xFF888888u;    // gray
    const uint32_t ar_color    = 0xFF66FF66u;    // greenish (acid)
    const uint32_t empty_color = 0xFF000000u;    // black background
    const uint32_t inclusion_color = 0xFF000000u; // black dot

    if (view_mode == 1) return (m == SAND ? sand_color : empty_color);
    if (view_mode == 2) return (m == WATER ? water_color : empty_color);
    if (view_mode == 3) return (m == WALL ? wall_color : empty_color);
    if (view_mode == 4) return (m == AR ? ar_color : empty_color);

    // composite view
    if (m == SAND) {
        return inclusion ? inclusion_color : sand_color;
    } else if (m == WATER) {
        return water_color;
    } else if (m == WALL) {
        return wall_color;
    } else if (m == AR) {
        return ar_color;
    }
    return empty_color;
}
