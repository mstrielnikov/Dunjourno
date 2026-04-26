#include <iostream>
#include <memory>
#include "include/pipeline.hpp"


int main(){
    auto grid = std::make_unique<Grid>(128, 128);

    std::cout << "--- Composing Pipeline (Lazy Initialization) ---\n";
    auto pipeline = Pipeline(
        LandscapeGenerator{12.0f},
        ForestMaskGenerator{0.6f},
        ForestPlacementGenerator{0.5f}
    );

    std::cout << "\n--- Evaluating layer composition... ---\n\n";
    pipeline.execute(*grid);

    // Visual feedback
    for (size_t y = 0; y < grid->get_height(); ++y) {
        for (size_t x = 0; x < grid->get_width(); ++x) {
            uint32_t id = (*grid)(x, y).textureId;
            if (id == 2) std::cout << "Y";      // Tree
            else if (id == 1) std::cout << "^"; // Mountain
            else std::cout << ".";             // Grass
        }
        std::cout << "\n";
    }

    return 0;
}