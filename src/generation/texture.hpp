#pragma once
#include <generation/generation.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace generation {

/**
 * @brief Terrain Texture Generator
 *
 * This pass computes the slope/steepness at each cell and uses it alongside
 * elevation to determine the terrain's biome/color (e.g., grass, rock, snow).
 * Uses relative height thresholds so coloring adapts to any terrain generator.
 */
struct TerrainTextureGenerator {
    TerrainTextureGenerator() {
        std::cout << "[INIT] TerrainTextureGenerator ready.\n";
    }

    std::expected<void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] TerrainTextureGenerator assigning colors...\n";

        int w = (int)grid.width();
        int h = (int)grid.height();

        // First pass: find max height for relative thresholds
        float max_height = 0.01f;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                max_height = std::max(max_height, grid(x, y).height);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                Cell& cell = grid(x, y);

                // Compute local steepness by sampling neighbors
                float dx = 0.0f;
                float dy = 0.0f;
                if (x > 0 && x < w - 1) {
                    dx = (grid(x + 1, y).height - grid(x - 1, y).height) * 0.5f;
                }
                if (y > 0 && y < h - 1) {
                    dy = (grid(x, y + 1).height - grid(x, y - 1).height) * 0.5f;
                }
                float steepness = std::sqrt(dx * dx + dy * dy);

                // Relative height: 0.0 = lowest, 1.0 = peak
                float rel_h = cell.height / max_height;

                // Default colors
                uint8_t r = 60, g = 140, b = 60; // Base Grass

                if (cell.terrain == TerrainType::Tree) {
                    // Darker green for trees
                    r = 30; g = 100; b = 30;
                } else if (cell.terrain == TerrainType::Mountain) {
                    if (rel_h > 0.75f && steepness < 0.05f) {
                        // Flat high ground: Snow
                        r = 240; g = 240; b = 250;
                    } else if (steepness > 0.05f) {
                        // Steep rock: Grey, darker at lower altitudes
                        float grey = 80.0f + 50.0f * rel_h;
                        r = (uint8_t)grey; g = (uint8_t)grey; b = (uint8_t)(grey + 5);
                    } else if (rel_h > 0.4f) {
                        // Mid-altitude: Brown rock / highland
                        r = 110; g = 95; b = 75;
                    } else {
                        // Low mountain: Highland grass / dirt
                        r = 90; g = 130; b = 80;
                    }
                } else if (cell.terrain == TerrainType::Grass) {
                    if (steepness > 0.08f) {
                        r = 139; g = 115; b = 85; // Dirt
                    }
                }

                cell.r = r;
                cell.g = g;
                cell.b = b;
            }
        }

        return {};
    }

    std::string get_name() const { return "TerrainTexture"; }
};

} // namespace generation
