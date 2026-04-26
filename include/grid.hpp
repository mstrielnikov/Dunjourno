#pragma once
#include <vector>
#include <cstdint>

struct Cell {
    float height = 0.0f;        // 0.0 (sea level) to 1.0 (peak)
    float forest_mask = 0.0f;   // 0.0 (no trees) to 1.0 (dense)
    uint32_t textureId = 0;     // 0: Grass, 1: Mountain, 2: Tree, 3: Water
};

struct Grid {
private:
    size_t width;
    size_t length;
    std::vector<Cell> cells;
public:
    Grid(size_t width, size_t length) : width(width), length(length), cells(width * length) {}

    size_t get_width() const { return width; }

    size_t get_height() const { return length; }   

    Cell& operator()(size_t x, size_t y) {
        return cells[y * width + x];
    }

    const Cell& operator()(size_t x, size_t y) const {
        return cells[y * width + x];
    }

    std::vector<Cell>& get_all_cells(){
        return cells;
    }

    const std::vector<Cell>& get_all_cells() const {
        return cells;
    }
    
    
};
