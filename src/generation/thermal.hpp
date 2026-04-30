#pragma once
#include <generation/generation.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

namespace generation {

/**
 * @brief Thermal Erosion Generator
 * 
 * Simulates material "slumping" down steep slopes to reach a natural angle of repose.
 * This helps eliminate "blocky" or "histogramic" artifacts by smoothing out over-steep ridges.
 */
struct ThermalErosionGenerator {
    float talus_threshold = 0.1f;  // Maximum height difference before material slumps
    float erosion_rate    = 0.1f;  // How much material moves per iteration (0 to 1)
    int   iterations      = 5;     // Number of erosion passes

    ThermalErosionGenerator(float threshold = 0.1f, float rate = 0.1f, int iters = 5)
        : talus_threshold(threshold), erosion_rate(rate), iterations(iters) {
        std::cout << "[INIT] ThermalErosionGenerator threshold=" << talus_threshold 
                  << " rate=" << erosion_rate << " iters=" << iterations << "\n";
    }

    std::expected<void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] ThermalErosionGenerator eroding slopes...\n";
        
        int w = (int)grid.width();
        int h = (int)grid.height();
        
        // Use a temporary height buffer to avoid directional bias
        std::vector<float> heights(w * h);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                heights[y * w + x] = grid(x, y).height;
            }
        }

        for (int i = 0; i < iterations; ++i) {
            // In each iteration, material slumps from high cells to lower ones
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    float current_h = heights[y * w + x];
                    float max_delta = 0.0f;
                    int target_x = -1, target_y = -1;

                    // Check 4-connectivity neighbors for the steepest descent
                    const int dx[] = {0, 0, -1, 1};
                    const int dy[] = {-1, 1, 0, 0};

                    for (int n = 0; n < 4; ++n) {
                        int nx = x + dx[n];
                        int ny = y + dy[n];

                        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                            float neighbor_h = heights[ny * w + nx];
                            float delta = current_h - neighbor_h;
                            if (delta > max_delta) {
                                max_delta = delta;
                                target_x = nx;
                                target_y = ny;
                            }
                        }
                    }

                    // If the slope is greater than the talus threshold, move material
                    if (max_delta > talus_threshold) {
                        float amount = (max_delta - talus_threshold) * erosion_rate;
                        heights[y * w + x] -= amount;
                        heights[target_y * w + target_x] += amount;
                    }
                }
            }
        }

        // Apply back to the grid
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                grid(x, y).height = heights[y * w + x];
            }
        }

        return {};
    }

    std::string get_name() const { return "ThermalErosion"; }
};

} // namespace generation
