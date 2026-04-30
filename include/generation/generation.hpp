#pragma once
#include <expected>
#include "grid.hpp"

namespace generation {

// Compile-time dispatch for chaining generator layers
template <typename T>
concept GeneratorLayer = requires(T t, Grid& grid) {
    { t.apply(grid) } -> std::same_as<std::expected<void, GenError>>;
    { t.get_name() } -> std::convertible_to<std::string>;
};

} // namespace generation
