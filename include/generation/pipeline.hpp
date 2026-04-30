#pragma once
#include "generation.hpp"
#include <tuple>
#include <format>
#include <iostream>
#include <memory>

namespace generation {

template <GeneratorLayer... Layers>
struct Pipeline {
    std::tuple<Layers...> layers;
    
    explicit Pipeline(Layers... s) : layers(std::move(s)...) {}

    /**
     * @brief Executes the pipeline on a raw Grid reference for stack-allocated or externally managed grids
     */
    std::expected<void, GenError> execute(Grid& grid) {
        std::cout << "--- Executing Static Pipeline ---\n";
        
        std::expected<void, GenError> status = {};
        
        std::apply([&](auto&... layer) {
            (..., ([&](){
                if (!status.has_value()) return;
                status = layer.apply(grid);
                if (status.has_value()){
                    std::cout << std::format("Layer {} executed successfully\n", layer.get_name());
                } else {
                    std::cout << std::format("Layer {} failed\n", layer.get_name());
                }
            }()));
        }, layers);
        
        return status;
    }

    /**
     * @brief Convenience overload for shared_ptr managed grids
     */
    std::expected<void, GenError> execute(std::shared_ptr<Grid> grid) {
        if (!grid) return std::unexpected(GenError::InvalidParameters);
        return execute(*grid);
    }
};

} // namespace generation