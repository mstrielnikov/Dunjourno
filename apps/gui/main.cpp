#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <memory>

#include <raylib.h>
#include <raymath.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include <stb_perlin.h>

#include "platform/diagnostics.hpp"
#include "generation/pipeline.hpp"
#include "generation/landscape.hpp"
#include "generation/forest.hpp"
#include "generation/mountain.hpp"
#include "generation/grid.hpp"
#include "generation_config.hpp"

using namespace generation;

static std::unique_ptr<Grid> g_grid;
static GridView g_grid_view;
static GenConfig g_config;
static GenConfig g_last_config;

void RebuildGrid() {
    int new_size = (int)g_config.grid_size;
    auto grid_res = Grid::create(new_size, new_size);
    if (!grid_res.has_value()) {
        std::cerr << "Failed to allocate Grid\n";
        return;
    }
    g_grid = std::make_unique<Grid>(std::move(grid_res.value()));
    g_grid_view = g_grid->view();

    auto pipeline = Pipeline(
        RippleTerrainGenerator{g_config.ripple_amp},
        ForestMaskGenerator{0.7f, 2.5f},
        ForestPlacementGenerator{g_config.forest_density},
        MountainRidgeGenerator{g_config.mountain_peak, 8.0f, 0.08f, g_config.mountain_power}
    );
    pipeline.execute(*g_grid);
    g_last_config = g_config;
}

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

int main() {
    const int screenWidth = 1200;
    const int screenHeight = 840;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Donjourno");
    SetTargetFPS(60);

    RebuildGrid();

    Camera2D camera = { .offset = { 600, 200 }, .target = { 0, 0 }, .rotation = 0.0f, .zoom = 0.5f };

    while (!WindowShouldClose()) {
        // Check if mouse is over UI to prevent panning while dragging sliders
        int panelWidth = 280;
        int panelHeight = 250;
        int panelX = GetScreenWidth() - panelWidth - 20;
        int panelY = 80;
        Rectangle uiBounds = { (float)panelX, (float)panelY, (float)panelWidth, (float)panelHeight };
        bool isMouseOverUI = CheckCollisionPointRec(GetMousePosition(), uiBounds);

        if (!isMouseOverUI && (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT))) {
            Vector2 delta = GetMouseDelta();
            camera.target.x -= delta.x / camera.zoom;
            camera.target.y -= delta.y / camera.zoom;
        }
        camera.zoom = std::clamp(camera.zoom + GetMouseWheelMove() * 0.05f, 0.1f, 3.0f);

        BeginDrawing();
            ClearBackground({ 30, 30, 35, 255 });
            
            BeginMode2D(camera);
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

            // Top-Right Floating UI Panel
            DrawRectangle(panelX, panelY, panelWidth, panelHeight, Fade(BLACK, 0.85f));

            // UI Layout
            GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
            GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(RAYWHITE));
            
            // Zoom Control
            DrawText("Zoom", panelX + 15, panelY + 15, 16, RAYWHITE);
            float currentZoom = camera.zoom;
            GuiSliderBar({ (float)panelX + 100, (float)panelY + 10, 160, 25 }, "", TextFormat("%.2f", currentZoom), &currentZoom, 0.1f, 3.0f);
            if (currentZoom != camera.zoom) camera.zoom = currentZoom;

            // Grid Size
            DrawText("Grid Size", panelX + 15, panelY + 50, 16, RAYWHITE);
            GuiSliderBar({ (float)panelX + 100, (float)panelY + 45, 160, 25 }, "", TextFormat("%.0f", g_config.grid_size), &g_config.grid_size, 32.0f, 256.0f);

            // Ripple Amp
            DrawText("Ripple Amp", panelX + 15, panelY + 85, 16, RAYWHITE);
            GuiSliderBar({ (float)panelX + 100, (float)panelY + 80, 160, 25 }, "", TextFormat("%.1f", g_config.ripple_amp), &g_config.ripple_amp, 1.0f, 20.0f);

            // Mountain Peak
            DrawText("Mnt Peak", panelX + 15, panelY + 120, 16, RAYWHITE);
            GuiSliderBar({ (float)panelX + 100, (float)panelY + 115, 160, 25 }, "", TextFormat("%.1f", g_config.mountain_peak), &g_config.mountain_peak, 1.0f, 10.0f);

            // Forest Density
            DrawText("Forest Den", panelX + 15, panelY + 155, 16, RAYWHITE);
            GuiSliderBar({ (float)panelX + 100, (float)panelY + 150, 160, 25 }, "", TextFormat("%.2f", g_config.forest_density), &g_config.forest_density, 0.0f, 1.0f);

            // Generate Button
            if (GuiButton({ (float)panelX + 15, (float)panelY + 185, 245, 25 }, "Generate Terrain") ||
               (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && 
                (g_config.grid_size != g_last_config.grid_size ||
                 g_config.ripple_amp != g_last_config.ripple_amp ||
                 g_config.mountain_peak != g_last_config.mountain_peak ||
                 g_config.forest_density != g_last_config.forest_density))) {
                RebuildGrid();
            }

            // Reset View Button
            if (GuiButton({ (float)panelX + 15, (float)panelY + 215, 245, 25 }, "Reset View")) {
                camera.zoom = 0.5f;
                camera.target = { 0, 0 };
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
