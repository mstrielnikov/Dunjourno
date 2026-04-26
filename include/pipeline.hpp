#pragma once
#include <generation.hpp>
#include <tuple>
#include <format>
#include <iostream>

template <typename T>
concept PipelineType = requires(T t, Grid& grid) {
    { t.execute(grid) } -> std::same_as<std::expected<void, GenError>>;
};


template <GeneratorLayer... Layers>
struct Pipeline {
    std::tuple<Layers...> layers;
    
    explicit Pipeline(Layers... s) : layers(std::move(s)...) {}

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
};