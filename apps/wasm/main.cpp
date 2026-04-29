#include <iostream>
#include <algorithm>

#include <raylib.h>
#include <raymath.h>

#include <emscripten/emscripten.h>

#include "generation/pipeline.hpp"
#include "generation/landscape.hpp"
#include "generation/forest.hpp"
#include "generation/grid.hpp"
#include "platform/diagnostics.hpp"

using namespace generation;

// --- Global State (required for emscripten_set_main_loop) ---

static GridView g_grid_view;
static Camera2D g_camera = {};

void DrawGPUTile(float x, float y, float w, float h, Color baseColor) {
    Vector2 top = { x, y };
    Vector2 left = { x - w / 2.0f, y + h / 2.0f };
    Vector2 right = { x + w / 2.0f, y + h / 2.0f };
    Vector2 bottom = { x, y + h };
    DrawTriangle(top, left, bottom, baseColor);
    DrawTriangle(top, bottom, right, ColorBrightness(baseColor, -0.15f));
}

void UpdateDrawFrame() {
    // Input
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        g_camera.target.x -= delta.x / g_camera.zoom;
        g_camera.target.y -= delta.y / g_camera.zoom;
    }
    g_camera.zoom = std::clamp(g_camera.zoom + GetMouseWheelMove() * 0.05f, 0.1f, 3.0f);

    // Draw
    BeginDrawing();
        ClearBackground({ 30, 30, 35, 255 });
        
        BeginMode2D(g_camera);
            for (size_t y = 0; y < g_grid_view.height(); ++y) {
                for (size_t x = 0; x < g_grid_view.width(); ++x) {
                    float isoX = (float)((int)x - (int)y) * 32.0f;
                    float isoY = (float)((int)x + (int)y) * 16.0f;
                    
                    const Cell& cell = g_grid_view(x, y);
                    Color tint = {40, (unsigned char)(cell.height * 120.0f + 60.0f), 40, 255};
                    if (cell.terrain == TerrainType::Mountain) tint = {110, 100, 90, 255};
                    
                    DrawGPUTile(isoX, isoY, 64.0f, 32.0f, tint);
                }
            }
        EndMode2D();

        DrawRectangle(0, 0, GetScreenWidth(), 60, Fade(BLACK, 0.8f));
        DrawText("Donjourno", 20, 10, 20, GOLD);
        DrawText(TextFormat("Renderer: %s | VRAM: %.1f%%", diagnostics::get_gpu_name().c_str(), diagnostics::get_vram_usage_percent()), 20, 35, 14, LIME);
        DrawFPS(GetScreenWidth() - 80, 10);
    EndDrawing();
}

int main() {
    const int screenWidth = 1200;
    const int screenHeight = 840;

    InitWindow(screenWidth, screenHeight, "Donjourno - Web Edition");

    // Generate terrain safely
    auto grid_res = Grid::create(128, 128);
    if (!grid_res.has_value()) {
        std::cerr << "Failed to allocate Grid\n";
        return 1;
    }
    
    // We must keep the actual Grid alive in static scope so the memory persists
    static Grid grid = std::move(grid_res.value());
    
    g_grid_view = grid.view();

    auto pipeline = Pipeline(RippleTerrainGenerator{6.0f}, ForestMaskGenerator{0.7f, 2.5f}, ForestPlacementGenerator{0.4f});
    pipeline.execute(grid);

    g_camera = { .offset = { 600, 200 }, .target = { 0, 0 }, .rotation = 0.0f, .zoom = 0.5f };

    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);

    CloseWindow();
    return 0;
}
