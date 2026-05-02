#pragma once
#include <generation/generation.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <vector>

#include "mountain.hpp"

namespace generation {

/**
 * @brief Hybrid Terrain Generator
 *
 * Combines explicit mountain pool placement with Newtonian hydraulic erosion:
 *   Phase 1: Place N elliptical mountain pools, sized by terrain coverage %
 *   Phase 2: Generate fBm heightmap only inside pools (cosine-blended edges)
 *   Phase 3: Run hydraulic erosion droplets within pool boundaries
 *   Phase 4: Classify terrain — outside pools = flat Grass (walkable)
 *
 * Designed for tabletop RPG use: flat terrain between mountains remains
 * perfectly walkable for future procedural cities, roads, and grid play.
 */
struct HybridTerrainGenerator {
    // Pool placement
    int   num_mountains     = 3;       // Number of mountain pools
    float terrain_coverage  = 0.25f;   // Fraction of grid area allocated to mountains

    // Heightmap
    float peak_height       = 3.0f;    // Maximum terrain height
    int   noise_octaves     = 6;       // fBm octaves

    // Erosion (Newtonian droplet model from SimpleErosion)
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

    // ── Pool definition ──────────────────────────────────────────────
    struct Pool {
        float cx, cy;          // Center
        float rx, ry;          // Semi-axes
        float cos_a, sin_a;    // Rotation
        uint32_t seed;         // Per-pool noise seed

        // Normalized elliptical distance (0 at center, 1 at boundary)
        float distance(float x, float y) const {
            float dx = x - cx;
            float dy = y - cy;
            float lx = dx * cos_a + dy * sin_a;
            float ly = -dx * sin_a + dy * cos_a;
            return std::sqrt((lx * lx) / (rx * rx) + (ly * ly) / (ry * ry));
        }

        bool contains(float x, float y) const {
            return distance(x, y) < 1.0f;
        }
    };

    std::expected<void, GenError> apply(Grid& grid) {
        int w = (int)grid.width();
        int h = (int)grid.height();
        std::cout << "[WORK] HybridTerrainGenerator on " << w << "x" << h << " grid\n";

        std::mt19937 rng(seed);

        // ═══════════════════════════════════════════════════════════════
        // Phase 1: Place mountain pools
        // ═══════════════════════════════════════════════════════════════
        std::cout << "  Phase 1: Placing " << num_mountains << " mountain pools ("
                  << (int)(terrain_coverage * 100) << "% coverage)...\n";

        float grid_area = (float)(w * h);
        float total_pool_area = grid_area * terrain_coverage;
        float area_per_pool = total_pool_area / (float)num_mountains;

        // r from A = pi * r^2  →  r = sqrt(A / pi)
        float base_r = std::sqrt(area_per_pool / 3.14159265f);

        std::uniform_real_distribution<float> axis_ratio(0.6f, 1.4f);
        std::uniform_real_distribution<float> angle_dist(0.0f, 3.14159265f);

        // Place pools with margin and separation
        std::vector<Pool> pools;
        pools.reserve(num_mountains);

        float margin = base_r * 0.3f;
        std::uniform_real_distribution<float> pos_x(margin + base_r, (float)w - margin - base_r);
        std::uniform_real_distribution<float> pos_y(margin + base_r, (float)h - margin - base_r);

        int max_attempts = num_mountains * 50;
        for (int i = 0; i < num_mountains && max_attempts > 0; ++i) {
            for (int attempt = 0; attempt < 50; ++attempt, --max_attempts) {
                float cx = pos_x(rng);
                float cy = pos_y(rng);

                // Check separation from existing pools
                bool too_close = false;
                for (const auto& p : pools) {
                    float dist = std::sqrt((cx - p.cx) * (cx - p.cx) + (cy - p.cy) * (cy - p.cy));
                    if (dist < base_r * 0.5f) { too_close = true; break; }
                }
                if (too_close) continue;

                float k = axis_ratio(rng);
                float a = angle_dist(rng);
                pools.push_back({
                    cx, cy,
                    base_r * k, base_r / k,
                    std::cos(a), std::sin(a),
                    seed + (uint32_t)i * 7919
                });
                break;
            }
        }

        std::cout << "  Placed " << pools.size() << " pools (base_r=" << base_r << ")\n";

        // ═══════════════════════════════════════════════════════════════
        // Phase 2: Generate fBm heightmap within pools
        // ═══════════════════════════════════════════════════════════════
        std::cout << "  Phase 2: Generating heightmap within pools...\n";

        std::vector<float> heightmap(w * h, 0.0f);
        // Pool membership mask (used to constrain erosion)
        std::vector<bool> pool_mask(w * h, false);

        // Auto-compute noise scale from pool radius (larger pools = larger features)
        float noise_scale = 2.0f / base_r;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float best_height = 0.0f;
                bool in_pool = false;

                for (const auto& pool : pools) {
                    float d = pool.distance((float)x, (float)y);
                    if (d >= 1.0f) continue;

                    in_pool = true;

                    // fBm noise at this position (using pool-specific seed)
                    float nx = (float)x * noise_scale;
                    float ny = (float)y * noise_scale;
                    float val = noise::fbm_2d(nx, ny, pool.seed, noise_octaves, 2.0f, 0.5f);

                    // Remap to [0, 1]
                    val = std::clamp(val * 1.3f - 0.15f, 0.0f, 1.0f);

                    // Cosine blend at pool edge: smooth falloff from d=0.7 to d=1.0
                    float blend = 1.0f;
                    if (d > 0.7f) {
                        blend = 0.5f * (1.0f + std::cos((d - 0.7f) / 0.3f * 3.14159265f));
                    }

                    float h_val = val * peak_height * blend;
                    best_height = std::max(best_height, h_val);
                }

                heightmap[y * w + x] = best_height;
                pool_mask[y * w + x] = in_pool;
            }
        }

        // ═══════════════════════════════════════════════════════════════
        // Phase 3: Hydraulic erosion within pools
        // ═══════════════════════════════════════════════════════════════
        std::cout << "  Phase 3: Simulating " << num_droplets << " water droplets within pools...\n";

        std::vector<float> erosion_map(w * h, 0.0f);

        // Build list of pool cells for droplet spawning
        std::vector<std::pair<int, int>> pool_cells;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (pool_mask[y * w + x])
                    pool_cells.push_back({x, y});

        if (pool_cells.empty()) {
            std::cout << "  No pool cells — skipping erosion.\n";
        } else {
            std::uniform_int_distribution<int> cell_dist(0, (int)pool_cells.size() - 1);

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
                // Spawn only within pool cells
                auto [start_x, start_y] = pool_cells[cell_dist(rng)];
                float px = (float)start_x;
                float py = (float)start_y;
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

                    // Kill droplet if it leaves the grid or exits a pool
                    if (px < 0 || px >= w || py < 0 || py >= h) break;

                    int new_ix = std::clamp((int)px, 0, w - 1);
                    int new_iy = std::clamp((int)py, 0, h - 1);

                    if (!pool_mask[new_iy * w + new_ix]) break;  // Left pool → die

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
