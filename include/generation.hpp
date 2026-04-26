#pragma once
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
