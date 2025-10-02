#pragma once
// Simulation class interface for falling-sand multi-material CA

#include "materials.h"
#include <vector>
#include <cstdint>

class Simulation {
public:
    Simulation(int w, int h, unsigned rng_seed = 0);

    int width() const;
    int height() const;

    // Update the simulation: do 'substeps' internal CA updates.
    // 'frame_count' is used for scan-order alternation.
    void step(int substeps, int &frame_count);

    // paint material at grid coord (x,y) with current brush radius
    void paint_at(int gx, int gy, int brush_radius);

    // clear grid
    void clear();

    // set/get paint material and view mode
    void set_paint_material(Cell m);
    Cell get_paint_material() const;
    void set_view_mode(int vm);
    int get_view_mode() const;

    // inclusion (black dots inside sand) control
    void set_inclusion_chance(double p); // 0..1
    double get_inclusion_chance() const;

    // read pixel color for render
    uint32_t pixel_color_at(int x, int y) const;

private:
    int W, H;
    std::vector<uint8_t> grid;       // cell material enum values
    std::vector<uint8_t> next_grid;
    std::vector<uint8_t> inclusion;  // overlay: 0/1 marking inclusion dot inside sand (visual-only)
    double inclusion_chance;
    Cell paint_material;
    int view_mode;
    unsigned rng_seed;
    // RNG functions hidden in cpp
};
