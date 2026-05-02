#pragma once
#include <generation/generation.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <vector>
#include <numeric>

#include "mountain.hpp"

namespace generation {

/**
 * @brief Hybrid Terrain Generator
 *
 * Uses noise-based coverage allocation for natural mountain formation:
 *   Phase 1: Generate a 2D noise mask and threshold it to allocate exactly
 *            coverage% of the map as mountain terrain in organic random shapes
 *   Phase 2: Generate fBm heightmap only inside the masked region
 *   Phase 3: Run hydraulic erosion droplets within the mask boundary
 *   Phase 4: Classify terrain — outside mask = flat Grass (walkable)
 *
 * The noise mask produces irregular, continent-like mountain regions
 * without explicit ellipse placement, giving photorealistic terrain shapes.
 * Flat terrain between mountains remains walkable for procedural cities,
 * roads, and grid-based RPG/D&D table play.
 */
struct HybridTerrainGenerator {
    // Coverage
    float terrain_coverage  = 0.25f;   // Fraction of grid allocated to mountains (0.0–1.0)

    // Heightmap
    float peak_height       = 3.0f;    // Maximum terrain height
    int   noise_octaves     = 6;       // fBm octaves

    // Erosion (Newtonian droplet model)
    int   num_droplets      = 70000;
    float dt                = 1.2f;
    float friction          = 0.05f;
    float density           = 1.0f;
    float deposition_rate   = 0.1f;
    float evaporation_rate  = 0.01f;
    float min_volume        = 0.01f;

    uint32_t seed = 42;

    HybridTerrainGenerator() {
        seed = std::random_device{}();
        std::cout << "[INIT] HybridTerrainGenerator seed=" << seed << "\n";
    }

    std::expected<void, GenError> apply(Grid& grid) {
        int w = (int)grid.width();
        int h = (int)grid.height();
        int total_cells = w * h;
        std::cout << "[WORK] HybridTerrainGenerator on " << w << "x" << h << " grid\n";

        // ═══════════════════════════════════════════════════════════════
        // Phase 1: Noise-based coverage mask
        // ═══════════════════════════════════════════════════════════════
        int target_cells = (int)(total_cells * std::clamp(terrain_coverage, 0.0f, 1.0f));
        std::cout << "  Phase 1: Allocating " << target_cells << "/" << total_cells
                  << " cells (" << (int)(terrain_coverage * 100) << "% coverage)...\n";

        // Generate a low-frequency noise field for the mask shape
        // Use a different seed offset so the mask shape differs from heightmap detail
        uint32_t mask_seed = seed + 99991;
        float mask_scale = 0.015f;  // Low frequency → large, continent-like regions

        std::vector<float> mask_noise(total_cells);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                mask_noise[y * w + x] = noise::fbm_2d(
                    (float)x * mask_scale, (float)y * mask_scale,
                    mask_seed, 4, 2.0f, 0.5f
                );
            }
        }

        // Find the threshold that gives exactly target_cells above it
        // Sort noise values and pick the (total - target)-th value as threshold
        std::vector<float> sorted_noise(mask_noise);
        std::sort(sorted_noise.begin(), sorted_noise.end());

        float threshold = 0.0f;
        if (target_cells <= 0) {
            threshold = sorted_noise.back() + 1.0f;  // Nothing passes
        } else if (target_cells >= total_cells) {
            threshold = sorted_noise.front() - 1.0f;  // Everything passes
        } else {
            // We want the top `target_cells` values → threshold at index (total - target)
            threshold = sorted_noise[total_cells - target_cells];
        }

        std::vector<bool> pool_mask(total_cells, false);
        for (int i = 0; i < total_cells; ++i) {
            pool_mask[i] = (mask_noise[i] >= threshold);
        }

        // ═══════════════════════════════════════════════════════════════
        // Phase 2: Generate fBm heightmap within mask
        // ═══════════════════════════════════════════════════════════════
        std::cout << "  Phase 2: Generating heightmap within coverage mask...\n";

        std::vector<float> heightmap(total_cells, 0.0f);

        // Auto-compute noise scale from coverage (more coverage = need larger features)
        float effective_radius = std::sqrt((float)target_cells / 3.14159265f);
        float height_scale = 2.0f / std::max(effective_radius, 1.0f);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (!pool_mask[y * w + x]) continue;

                float nx = (float)x * height_scale;
                float ny = (float)y * height_scale;
                float val = noise::fbm_2d(nx, ny, seed, noise_octaves, 2.0f, 0.5f);

                val = std::clamp(val * 1.3f - 0.15f, 0.0f, 1.0f);

                // Smooth blend at mask edges: check distance to nearest non-mask cell
                // Approximate by checking how deep inside the mask we are via the
                // noise value relative to threshold (higher noise = deeper inside)
                float depth = (mask_noise[y * w + x] - threshold);
                float blend_range = 0.05f;  // Noise units for edge blend
                float blend = std::clamp(depth / blend_range, 0.0f, 1.0f);

                heightmap[y * w + x] = val * peak_height * blend;
            }
        }

        // ═══════════════════════════════════════════════════════════════
        // Phase 3: Hydraulic erosion within mask
        // ═══════════════════════════════════════════════════════════════
        std::cout << "  Phase 3: Simulating " << num_droplets << " water droplets within mask...\n";

        std::vector<float> erosion_map(total_cells, 0.0f);

        // Build list of mask cells for droplet spawning
        std::vector<int> mask_indices;
        mask_indices.reserve(target_cells);
        for (int i = 0; i < total_cells; ++i) {
            if (pool_mask[i]) mask_indices.push_back(i);
        }

        if (mask_indices.empty()) {
            std::cout << "  No mask cells — skipping erosion.\n";
        } else {
            std::mt19937 rng(seed + 1);
            std::uniform_int_distribution<int> cell_dist(0, (int)mask_indices.size() - 1);

            auto surface_normal = [&](int x, int y) -> std::tuple<float, float, float> {
                float hL = heightmap[y * w + std::max(x - 1, 0)];
                float hR = heightmap[y * w + std::min(x + 1, w - 1)];
                float hD = heightmap[std::max(y - 1, 0) * w + x];
                float hU = heightmap[std::min(y + 1, h - 1) * w + x];
                float nx = (hL - hR) * 0.5f;
                float ny = 1.0f;
                float nz = (hD - hU) * 0.5f;
                float len = std::sqrt(nx * nx + ny * ny + nz * nz);
                return {nx / len, ny / len, nz / len};
            };

            auto modify_cell = [&](int x, int y, float amount) {
                x = std::clamp(x, 0, w - 1);
                y = std::clamp(y, 0, h - 1);
                heightmap[y * w + x] += amount;
                erosion_map[y * w + x] += std::abs(amount);
            };

            for (int drop = 0; drop < num_droplets; ++drop) {
                int idx = mask_indices[cell_dist(rng)];
                float px = (float)(idx % w);
                float py = (float)(idx / w);
                float sx = 0.0f, sy = 0.0f;
                float volume = 1.0f;
                float sediment = 0.0f;

                while (volume > min_volume) {
                    int ix = (int)px;
                    int iy = (int)py;

                    auto [n_x, n_y, n_z] = surface_normal(ix, iy);

                    float mass = volume * density;
                    sx += dt * n_x / mass;
                    sy += dt * n_z / mass;

                    px += dt * sx;
                    py += dt * sy;

                    sx *= (1.0f - dt * friction);
                    sy *= (1.0f - dt * friction);

                    if (px < 0 || px >= w || py < 0 || py >= h) break;

                    int new_ix = std::clamp((int)px, 0, w - 1);
                    int new_iy = std::clamp((int)py, 0, h - 1);

                    if (!pool_mask[new_iy * w + new_ix]) break;  // Left mask → die

                    float speed = std::sqrt(sx * sx + sy * sy);
                    float h_diff = heightmap[iy * w + ix] - heightmap[new_iy * w + new_ix];
                    float max_sediment = volume * speed * std::max(h_diff, 0.0f);
                    float sdiff = max_sediment - sediment;

                    sediment += dt * deposition_rate * sdiff;
                    modify_cell(ix, iy, -dt * volume * deposition_rate * sdiff);

                    volume *= (1.0f - dt * evaporation_rate);
                }
            }
        }

        // ═══════════════════════════════════════════════════════════════
        // Phase 4: Write back to grid + classify
        // ═══════════════════════════════════════════════════════════════
        std::cout << "  Phase 4: Classifying terrain...\n";

        float max_erosion = 0.01f;
        for (float e : erosion_map) max_erosion = std::max(max_erosion, e);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                Cell& cell = grid(x, y);
                float height = std::max(heightmap[y * w + x], 0.0f);
                cell.height = height;

                if (pool_mask[y * w + x] && height > 0.1f) {
                    cell.terrain = TerrainType::Mountain;
                } else {
                    cell.terrain = TerrainType::Grass;
                }

                float erosion_intensity = erosion_map[y * w + x] / max_erosion;
                cell.roughness = std::clamp(erosion_intensity * std::clamp(height, 0.0f, 1.0f), 0.0f, 1.0f);
            }
        }

        std::cout << "  Hybrid terrain complete.\n";
        return {};
    }

    std::string get_name() const { return "HybridTerrain"; }
};

} // namespace generation
