#include <iostream>
#include <algorithm>

#include <raylib.h>
#include <raymath.h>

#include <emscripten/emscripten.h>

#include <stb_perlin.h>

#include "generation/pipeline.hpp"
#include "generation/landscape.hpp"
#include "generation/forest.hpp"
#include "generation/mountain.hpp"
#include "generation/grid.hpp"
#include "platform/diagnostics.hpp"

using namespace generation;

// --- Global State (required for emscripten_set_main_loop) ---

static GridView g_grid_view;
static Camera2D g_camera = {};

void DrawGPUTile3D(float x, float y, float w, float h, float elevation, Color baseColor) {
    // Top face
    Vector2 top = { x, y - elevation };
    Vector2 left = { x - w / 2.0f, y + h / 2.0f - elevation };
    Vector2 right = { x + w / 2.0f, y + h / 2.0f - elevation };
    Vector2 bottom = { x, y + h - elevation };
    
    DrawTriangle(top, left, bottom, baseColor);
    DrawTriangle(top, bottom, right, ColorBrightness(baseColor, -0.1f));

    // If elevation > 0, draw sides
    if (elevation > 0.0f) {
        Vector2 left_base = { x - w / 2.0f, y + h / 2.0f };
        Vector2 bottom_base = { x, y + h };
        Vector2 right_base = { x + w / 2.0f, y + h / 2.0f };

        Color leftColor = ColorBrightness(baseColor, -0.3f);
        Color rightColor = ColorBrightness(baseColor, -0.5f);

        // Left face
        DrawTriangle(left, left_base, bottom_base, leftColor);
        DrawTriangle(left, bottom_base, bottom, leftColor);

        // Right face
        DrawTriangle(bottom, bottom_base, right_base, rightColor);
        DrawTriangle(bottom, right_base, right, rightColor);
    }
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
                    float elevation = 0.0f;

                    if (cell.terrain == TerrainType::Mountain) {
                        tint = {110, 100, 90, 255};
                        elevation = cell.height * 80.0f; // Dramatic spine-to-edge spread
                    } else if (cell.terrain == TerrainType::Tree) {
                        tint = {20, 140, 40, 255};       // Distinct tree color
                        elevation = 10.0f;               // Small flat bump for trees
                    }
                    
                    DrawGPUTile3D(isoX, isoY, 64.0f, 32.0f, elevation, tint);
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

    auto pipeline = Pipeline(
        RippleTerrainGenerator{6.0f},
        GaussianForestSeeder{ {20, 100, 15.0, 15.0, 1.8} },
        ForestPlacementGenerator{0.4f},
        MountainRidgeGenerator{3.0f, 8.0f, 0.08f, 2.5f}
    );
    pipeline.execute(grid);

    g_camera = { .offset = { 600, 200 }, .target = { 0, 0 }, .rotation = 0.0f, .zoom = 0.5f };

    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);

    CloseWindow();
    return 0;
}
