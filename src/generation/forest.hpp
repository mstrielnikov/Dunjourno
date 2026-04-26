#pragma once
#include <generation.hpp>
#include <iostream>
#include <vector>

struct ForestMaskGenerator {
    float limit = {};
    
    ForestMaskGenerator(float l) : limit(l) {
        std::cout << "[INIT] ForestMaskGenerator created with limit: " << limit << "\n";
    }
    
    std::expected<void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] ForestMaskGenerator calculating fertile areas...\n";
        for (auto& cell : grid.get_all_cells()){
            cell.forest_mask = (cell.height > limit) ? 0.0f : 1.0f - (cell.height / limit);
        }
        return {};
    }

    std::string get_name() const { return "ForestMaskGenerator"; }
};

struct ForestPlacementGenerator {
    float density = {};
    std::mt19937 gen;
    std::uniform_real_distribution<float> dist;

    ForestPlacementGenerator(float d) : density(d), gen(std::random_device()()), dist(0.0f, 1.0f) {
        std::cout << "[INIT] ForestPlacementGenerator created with density: " << density << "\n";
    }

    std::expected<void, GenError> apply(Grid& grid){
        std::cout << "[WORK] ForestPlacementGenerator planting trees stochastically...\n";
        for (auto& cell: grid.get_all_cells()){
            float noise = dist(gen);
            if (noise < (cell.forest_mask * density)) {
                cell.textureId = 2; // Tree
            }
            else if (cell.height > 0.8f){
                cell.textureId = 1; // Peak
            } else {
                cell.textureId = 0; // Grass
            }
        }
        return {};
    }

    std::string get_name() const { return "ForestPlacementGenerator"; }
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
        for (size_t y = 0; y < grid.get_height(); ++y) {
            for (size_t x = 0; x < grid.get_width(); ++x) {
                Cell& cell = grid(x, y);
                float total_weight = 0.0f;

                for (const auto& s : clusters) {
                    float dx = (static_cast<float>(x) - s.x);
                    float dy = (static_cast<float>(y) - s.y);
                    
                    // Multivariate Gaussian Formula
                    float exponent = -( (dx*dx)/(2*s.sx*s.sx) + (dy*dy)/(2*s.sy*s.sy) );
                    total_weight += s.amp * std::exp(exponent);
                }

                // IMPORTANT: The Gaussian weight is MULTIPLIED by the height mask
                // This ensures that even if a "cluster" is centered on a mountain,
                // the height mask suppresses the trees.
                cell.forest_mask = std::clamp(total_weight * cell.forest_mask, 0.0f, 1.0f);
            }
        }
        return {};
    }
    std::string get_name() const { return "GaussianClustering"; }
};