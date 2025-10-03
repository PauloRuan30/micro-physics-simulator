// simulation.h
#ifndef SIMULATION_H
#define SIMULATION_H

#include <vector>
#include <cstdint>

class Simulation {
public:
    Simulation(int w, int h);

    void step_once();                 // perform one CA update
    void paint_at(int gx, int gy);    // paint at grid cell (gx,gy) using current brush radius & material

    void set_brush_radius(int r) { brush_radius = r; }
    void set_paint_material(uint8_t m) { paint_material = m; }

    // public for simple access from main
    int W, H;
    std::vector<uint8_t> grid;       // current grid material ids
    std::vector<uint8_t> next_grid;  // next step
    std::vector<uint8_t> inclusion;  // optional overlay/selection flags

    int brush_radius = 4;
    uint8_t paint_material = 1; // default = SAND

private:
    inline int idx(int x, int y) const { return y * W + x; }
};

#endif // SIMULATION_H