#include <iostream>
#include <memory>
#include <pipeline.hpp>
#include <landscape.hpp>
#include <forest.hpp>

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
    auto grid = std::make_shared<Grid>(128, 128);

    std::cout << "--- Composing Pipeline (Lazy Initialization) ---\n";
    
    auto pipeline = Pipeline(
        RippleTerrainGenerator{5.0f},                       // Base landmass
        ForestMaskGenerator{0.6f, 2.0f},                    // Forest mask
        GaussianForestSeeder{ {20, 100, 15.0, 15.0, 1.8} }, // Specific groves
        ForestPlacementGenerator{0.5f}                      // Final texture assignment
    );

    std::cout << "\n--- Evaluating layer composition... ---\n\n";
    pipeline.execute(std::move(grid));

    // Visual feedback
    

    return 0;
}