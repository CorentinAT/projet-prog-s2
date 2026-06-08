#include "draw.hpp"

#include "app.hpp"

#include "generation.hpp"

#include "imgui.h"
#include "raylib.h"
#include "raymath.h"

void draw3DScene(AppContext& context) {
    ClearBackground((Color){ 108, 99, 145 });
    
    BeginMode3D(context.camera);

    Matrix const terrainCentering { getTerrainCenteringMatrix(context) };
    Vector3 const terrainCenterOffset { terrainCentering.m12, terrainCentering.m13, terrainCentering.m14 };

    DrawModel(context.model, terrainCenterOffset, 1.0f, WHITE);
    drawLollipops(context, terrainCentering);
    // DrawGrid(20, 1.0f);

    EndMode3D();
}

void drawLollipops(AppContext const& context, Matrix const& terrainCentering)
{
    if (context.objectPositions.empty()) {
        return;
    }

    float const lollipopHalfHeight { 0.5f * context.lollipopScale };

    for (glm::vec3 const& pos : context.objectPositions) {
        Matrix const objectTranslation { MatrixTranslate(
            pos.x * context.terrainSize.x,
            pos.z * context.terrainSize.y + lollipopHalfHeight,
            pos.y * context.terrainSize.z
        )};
        Matrix const centeredTranslation { MatrixMultiply(objectTranslation, terrainCentering) };
        Matrix const scale { MatrixScale(context.lollipopScale, context.lollipopScale, context.lollipopScale) };
        Matrix const transform { MatrixMultiply(scale, centeredTranslation) };

        Vector3 pos3d {
            centeredTranslation.m12,
            centeredTranslation.m13,
            centeredTranslation.m14
        };
        DrawModelEx(context.objectModel, pos3d, {0,1,0}, 0.0f,
        {context.lollipopScale, context.lollipopScale, context.lollipopScale}, WHITE);
    }
}

void drawImGui(AppContext& context) {
    if (ImGui::CollapsingHeader("Points placement", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Minimal radius", &context.pointsGenerationParameters.radius, 5.0f, 100.0f);
        ImGui::SliderInt("Tries before reject", &context.pointsGenerationParameters.nbTries, 1, 50);
        ImGui::Checkbox("No point in sea", &context.pointsGenerationParameters.notInSea);
    }

    auto& params = context.imageGenerationParameters;
    auto& mask_params = context.radialMaskGenerationParameters;
    if(ImGui::Button("Generate random positions")) {
        generateObjectsPositions(context);
    }
    if(ImGui::Button("Refresh")) {
        generateHeightmap(context);
        regenerateMeshFromImage(context);
        generateObjectsPositions(context);
    }

    if (ImGui::CollapsingHeader("objects", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Lollipop Scale", &context.lollipopScale, 0.05f, 0.5f);
        ImGui::SliderInt("Octaves", &params.octaves, 5, 12);
        ImGui::SliderFloat("Lacunarity", &params.lacunarity, 0.5f, 2.0f);
        ImGui::SliderFloat("Gain", &params.gain, 0.5f, 10.0f);
        ImGui::SliderFloat("Noise Scale", &params.scale, 0.5f, 10.0f);
        ImGui::SliderFloat("Frequency", &params.frequency, 0.5f, 2.0f);
        ImGui::SliderFloat("Persistence", &params.persistence, 0.5f, 2.0f);
        ImGui::SliderInt("Seed", &params.noiseSeed, 1, 20);

        ImGui::SliderFloat("Island Size", &mask_params.mask_scale, 0.5f, 66.0f);
        ImGui::SliderFloat("Island Amplitude", &mask_params.mask_amplitude, 0.5f, 63.0f);
    }
}

void drawRaylibUI(AppContext& context) {
    int screenWidth { GetScreenWidth() };
    
    float wanted_size { 500.f };
    float scale_factor { wanted_size / std::max(context.texture.width, context.texture.height) };
    float const preview_x { screenWidth - wanted_size - 20.f };
    float const preview_y { 20.f };
    float const preview_w { context.texture.width * scale_factor };
    float const preview_h { context.texture.height * scale_factor };
    // DrawTexture(context.texture, screenWidth - context.texture.width - 20, 20, WHITE);
    DrawTextureEx(context.texture, { preview_x, preview_y }, 0.0f, scale_factor, WHITE);
    DrawRectangleLines(screenWidth - wanted_size - 20, 20, wanted_size, wanted_size, GREEN);

    //draw positions on top of the heightmap
    for (auto const& pos : context.objectPositions)
    {
        // Remap normalized coordinates [0..1] to the preview image in screen space.
        float const px { preview_x + Clamp(pos.x, 0.0f, 1.0f) * preview_w };
        float const py { preview_y + Clamp(pos.y, 0.0f, 1.0f) * preview_h };

        DrawCircleV({ px, py }, 2.0f, RED);
    }

    DrawFPS(10, 10);
}

