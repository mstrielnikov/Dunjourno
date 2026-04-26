#include <concepts>
#include <expected>
#include <algorithm>
#include <cmath>
#include <string>
#include <random>
#include "grid.hpp"

enum class GenError {InvalidParameters, StepFailed};

// The Concept: Defines what it means to be a "Generation Step"
template <typename T>
concept GeneratorLayer = requires(T t, Grid& grid) {
    { t.apply(grid) } -> std::same_as<std::expected<void, GenError>>;
    { t.get_name() } -> std::convertible_to<std::string>;
};

struct LandscapeGenerator {
    float freequency = {};
    
    LandscapeGenerator(float f) : freequency(f) {
        std::cout << "[INIT] LandscapeGenerator created with freq: " << freequency << "\n";
    }
    
    std::expected <void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] LandscapeGenerator applying transformation...\n";
        float centerX = grid.get_width() / 2.0f;
        float centerY = grid.get_height() / 2.0f;
        for (size_t y = 0; y < grid.get_height(); y++){
            for (size_t x = 0; x < grid.get_width(); x++){
                float dx = (x - centerX) / centerX;
                float dy = (y - centerY) / centerY;
                float distance = std::sqrt(dx * dx + dy * dy);
                grid(x, y).height = std::clamp(0.5f + 0.5f * std::sin(distance + freequency), 0.0f, 1.0f);
            }
        }
        return {};
    }

    std::string get_name() const { return "LandscapeGenerator"; }
};

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
