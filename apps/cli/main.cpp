#include <iostream>
#include <memory>

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>
#undef STB_PERLIN_IMPLEMENTATION

#include "generation/pipeline.hpp"
#include "generation/forest.hpp"
#include "generation/mountain.hpp"
#include "generation/thermal.hpp"
#include "generation/texture.hpp"

using namespace generation;

int main(){
    auto grid_res = Grid::create(128, 128);
    if (!grid_res.has_value()) {
        std::cerr << "Failed to allocate Grid\n";
        return 1;
    }
    // We use shared_ptr because the pipeline can accept it
    auto grid = std::make_shared<Grid>(std::move(grid_res.value()));

    std::cout << "--- Composing Pipeline (Lazy Initialization) ---\n";
    
    auto pipeline = Pipeline(
        MountainPlacementGenerator{3, 12.0f},
        MountainRidgeGenerator{3.0f, 10.0f, 0.1f, 2.5f},
        ThermalErosionGenerator{0.15f, 0.1f, 8},
        ForestMaskGenerator{0.6f, 2.0f},
        ForestPlacementGenerator{0.4f},
        TerrainTextureGenerator{}
    );

    std::cout << "\n--- Evaluating layer composition... ---\n\n";
    pipeline.execute(grid);

    // Visual feedback
    GridView view = grid->view();
    float max_h = 0;
    int mt_count = 0;
    for (size_t y = 0; y < view.height(); ++y) {
        for (size_t x = 0; x < view.width(); ++x) {
            TerrainType terrain = view(x, y).terrain;
            if (view(x,y).height > max_h) max_h = view(x,y).height;
            if (terrain == TerrainType::Tree) std::cout << "Y";
            else if (terrain == TerrainType::Mountain) { std::cout << "^"; mt_count++; }
            else std::cout << ".";
        }
        std::cout << "\n";
    }

    std::cout << "Max height: " << max_h << " | Mountain cells: " << mt_count << "\n";

    return 0;
}