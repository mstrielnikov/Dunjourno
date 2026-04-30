#pragma once
#include <generation/generation.hpp>
#include <iostream>
#include <vector>
#include <random>

namespace generation {

struct ForestMaskGenerator {
    float limit = {};       // Height where trees approximately hit 0%
    float steepness = {};   // Higher value = trees forced lower into valleys

    ForestMaskGenerator(float l, float s) : limit(l), steepness(s) {
        std::cout << "[INIT] ForestMaskGeneratorSteep created with limit: " << limit << " and steepness: " << steepness << "\n";
    }

    std::expected<void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] ForestMaskGeneratorSteep calculating fertile areas...\n";
        for (auto [x, y, cell] : grid.iter()) {
            // No trees on mountains
            if (cell.terrain == TerrainType::Mountain) {
                cell.forest_mask = 0.0f;
                continue;
            }

            if (cell.height > limit) {
                // Rare "Stunted" tree chance even above limit
                cell.forest_mask = 0.5f;
            } else {
                float h_norm = cell.height / limit;
                cell.forest_mask = std::clamp(1.0f - std::pow(h_norm, steepness), 0.0f, 1.0f);
            }

        }
        return {};
    }

    std::string get_name() const { return "ForestMaskGeneratorSteep"; }
};

struct GaussianForestSeeder {
    struct Seed { float x, y, sx, sy, amp; };
    std::vector<Seed> clusters;

    GaussianForestSeeder(float x, float y, float sx, float sy, float amp) {
        clusters.push_back({x, y, sx, sy, amp});
        std::cout << "[INIT] GaussianForestSeeder created with single seed at (" << x << ", " << y << ")\n";
    }

    GaussianForestSeeder(std::initializer_list<Seed> s) : clusters(s) {
        std::cout << "[INIT] GaussianForestSeeder created with " << clusters.size() << " seeds\n";
    }

    std::expected<void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] GaussianForestSeeder clusters applying...\n";
        for (auto [x, y, cell] : grid.iter()) {
            float total_weight = 0.0f;

            for (const auto& s : clusters) {
                float dx = x - s.x;
                float dy = y - s.y;
                
                // Multivariate Gaussian Formula
                float exponent = -( (dx*dx)/(2*s.sx*s.sx) + (dy*dy)/(2*s.sy*s.sy) );
                total_weight += s.amp * std::exp(exponent);
            }

            // IMPORTANT: The Gaussian weight is MULTIPLIED by the height mask
            // This ensures that even if a "cluster" is centered on a mountain,
            // the height mask suppresses the trees.
            cell.forest_mask = std::clamp(total_weight * cell.forest_mask, 0.0f, 1.0f);
        }
        return {};
    }
    std::string get_name() const { return "GaussianClustering"; }
};

struct ForestPlacementGenerator {
    bool allow_anomalies = true;
    float anomaly_chance = 0.005f;
    float density = {};
    std::mt19937 gen = std::mt19937(std::random_device()());
    std::uniform_real_distribution<float> dist = std::uniform_real_distribution<float>(0.0f, 1.0f);

    ForestPlacementGenerator(float d) : density(d) {
        std::cout << "[INIT] ForestPlacementGenerator created with density: " << density << "\n";
    }

    ForestPlacementGenerator(float d, uint32_t seed) : density(d), gen(seed) {
        std::cout << "[INIT] ForestPlacementGenerator created with density: " << density << " and fixed seed: " << seed << "\n";
    }

    std::expected<void, GenError> apply(Grid& grid){
        std::cout << "[WORK] ForestPlacementGenerator planting trees stochastically...\n";
        for (auto& cell: grid.cells()){
            // Mountains are already placed upstream — never override them
            if (cell.terrain == TerrainType::Mountain) continue;

            float noise = dist(gen);
            
            // Spawn if we meet the density threshold OR if it's a valid anomaly
            bool should_spawn = (noise < (cell.forest_mask * density)) || 
                                (allow_anomalies && cell.height < 0.95f && noise < anomaly_chance);
            
            if (should_spawn) {
                cell.terrain = TerrainType::Tree;
            } else {
                cell.terrain = TerrainType::Grass;
            }
        }
        return {};
    }

    std::string get_name() const { return "ForestPlacementGenerator"; }
};

} // namespace generation
