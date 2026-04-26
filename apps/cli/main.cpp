#include <iostream>
#include <memory>
#include "generation/pipeline.hpp"
#include "generation/landscape.hpp"
#include "generation/forest.hpp"

using namespace generation;

// Center forest in mid walley mountains
// auto pipeline = Pipeline(
//     LandscapeGenerator{12.0f},
//     ForestMaskGenerator{0.6f},
//     ForestPlacementGenerator{0.5f}
// );

// Cluster mountain with forest in mid walley
// auto pipeline = Pipeline(
//     RippleTerrainGenerator{5.0f}, // Base landmass
//     ForestMaskGenerator{0.6f},
//     ForestPlacementGenerator{0.5f} // Final texture assignment
// );       


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
        RippleTerrainGenerator{5.0f},                       // Base landmass
        ForestMaskGenerator{0.6f, 2.0f},                    // Forest mask
        GaussianForestSeeder{ {20, 100, 15.0, 15.0, 1.8} }, // Specific groves
        ForestPlacementGenerator{0.5f}                      // Final texture assignment
    );

    std::cout << "\n--- Evaluating layer composition... ---\n\n";
    pipeline.execute(grid);

    // Visual feedback
    // Visual feedback using the read-only view
    GridView view = grid->view();
    for (size_t y = 0; y < view.height(); ++y) {
        for (size_t x = 0; x < view.width(); ++x) {
            TerrainType terrain = view(x, y).terrain;
            if (terrain == TerrainType::Tree) std::cout << "Y";                  // Tree
            else if (terrain == TerrainType::Mountain) std::cout << "^";         // Mountain
            else std::cout << ".";                                               // Grass
        }
        std::cout << "\n";
    }

    return 0;
}