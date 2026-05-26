#include "generation.hpp"

#include "noise.hpp"
#include "raylib.h"

#include "utils/raylibUtils.hpp"
#include <algorithm> // for std::clamp

#include "lib/random.hpp"

std::vector<glm::vec2> generate2DPositions([[maybe_unused]] PointsGenerationParameters const& params) {
    int regionSize = 1000;

    std::vector<glm::vec2> positions {};
    std::vector<glm::vec2> spawnPositions {};

    positions.reserve(regionSize);

    float cellSize = params.radius / sqrt(2.);

    int cols = (int)std::ceil(regionSize / cellSize);
    int rows = (int)std::ceil(regionSize / cellSize);

    std::vector<std::vector<int>> grid(cols, std::vector<int>(rows, -1));

    glm::vec2 firstPoint = {random_int(0, regionSize), random_int(0, regionSize)};

    positions.push_back(firstPoint);
    spawnPositions.push_back(firstPoint);
    grid[(int)(firstPoint.x / cellSize)][(int)(firstPoint.y / cellSize)] = 0;

    while(spawnPositions.size() > 0) {
        int spawnIndex = random_int(0, spawnPositions.size());
        glm::vec2 spawnCenter = spawnPositions[spawnIndex];
        bool candidateAccepted = false;

        for(int i = 0; i < params.nbTries; i++) {
            float angle = random_float(0., 1.) * PI * 2.;
            glm::vec2 dir = {sin(angle), cos(angle)};
            glm::vec2 candidate = spawnCenter + dir * random_float((float)(params.radius), (float)(params.radius) * 2.);

            if(candidate.x < 0 || candidate.x >= regionSize || candidate.y < 0 || candidate.y >= regionSize) {
                continue;
            }

            int cellX = (int)(candidate.x / cellSize);
            int cellY = (int)(candidate.y / cellSize);

            bool isValid = true;
            for(int deltaX = -2; deltaX <= 2 && isValid; deltaX++) {
                for(int deltaY = -2; deltaY <= 2 && isValid; deltaY++) {
                    int neighborX = cellX + deltaX;
                    int neighborY = cellY + deltaY;
                    if(neighborX >= 0 && neighborX < cols && neighborY >= 0 && neighborY < rows && grid[neighborX][neighborY] != -1) {
                        float dist = glm::length(candidate - positions[grid[neighborX][neighborY]]);
                        if(dist < params.radius) {
                            isValid = false;
                        }
                    }
                }
            }

            if(isValid) {
                positions.push_back(candidate);
                spawnPositions.push_back(candidate);
                grid[cellX][cellY] = positions.size() - 1;
                candidateAccepted = true;
                break;
            }
        }
        if(!candidateAccepted) {
            spawnPositions.erase(spawnPositions.begin() + spawnIndex);
        }
    }

    for(int i = 0; i < static_cast<int>(positions.size()); i++) {
        positions[i].x = positions[i].x / regionSize;
        positions[i].y = positions[i].y / regionSize;
    }
    
    // points output should be in [0..1] range, where (0,0) is one corner of the terrain and (1,1) is the opposite corner, so they can be easily scaled to terrain size and sampled from heightmap.
    return positions;
}

void generateObjectsPositions(AppContext& context) {
    std::vector<glm::vec2> const positions {generate2DPositions(context.pointsGenerationParameters)};

    context.objectPositions.clear();
    context.objectPositions.reserve(positions.size());
    for (glm::vec2 const& p : positions)
    {
        float height = sampleHeightmap(context, p.x, p.y);

        if(height >= 0.3 || !context.pointsGenerationParameters.notInSea) {
            context.objectPositions.emplace_back(
                p.x, // x
                p.y, // y
                // sample height from heightmap for each point (asuming positions are normalized in [0..1] range)
                height
            );
        }
    }
    // TODO(student): extension - filter positions by sampled height range.
}

float sampleHeightmap(AppContext const& context, float u, float v)
{
    if (!context.heightmapImage.data || context.heightmapImage.width <= 0 || context.heightmapImage.height <= 0) return 0.0f;

    int const px = std::clamp(static_cast<int>(u * static_cast<float>(context.heightmapImage.width - 1)), 0, context.heightmapImage.width - 1);
    int const py = std::clamp(static_cast<int>(v * static_cast<float>(context.heightmapImage.height - 1)), 0, context.heightmapImage.height - 1);

    // If the heightmap is in R32 format, we can directly read the height value as a float. 
    if (context.heightmapImage.format == PIXELFORMAT_UNCOMPRESSED_R32)
    {
        float const* heightData = static_cast<float const*>(context.heightmapImage.data);
        int const idx = py * context.heightmapImage.width + px;
        return std::clamp(heightData[idx], 0.0f, 1.0f);
    }

    // Otherwise, we assume it's in a color format and we read the red channel as height (with normalization from [0..255] to [0..1]).
    Color const c = GetImageColor(context.heightmapImage, px, py);
    return static_cast<float>(c.r)/255.0f;
}

void generateHeightmap(AppContext& context) {

    if (context.texture.id > 0) {
        UnloadTexture(context.texture);
        context.texture = {};
    }

    if(context.image.data) {
        UnloadImage(context.image);
        context.image = {};
    }

    if (context.heightmapImage.data) {
        UnloadImage(context.heightmapImage);
        context.heightmapImage = {};
    }

    int const resolution = std::max(1, context.imageGenerationParameters.resolution);

    context.heightmapImage = GenImageFromNoiseFunction<float>(resolution, resolution, PIXELFORMAT_UNCOMPRESSED_R32,
        [&](glm::vec2 const& p)->float {
            // TODO(student): implement stack based noise and island mask
                
            return (octaveNoise(p * context.imageGenerationParameters.noiseScale, 
                [&](glm::vec2 const& p)->float {
                  return  perlinNoiseSeeded(p, context.imageGenerationParameters.noiseSeed);
                },
                context 
            ) * 0.5f + 0.5f)-0.1f;
        });

    // exemple conversion from heightmap to color image
    context.image = TransformImage<float, Color>(context.heightmapImage, [&](float const& v, int const, int const) {
        if (v < 0.3f)
        {
            return color_from({ 120, 180, 230 }); // water
        }
        else if (v < 0.5f)
        {
            return color_from({ 238, 214, 175 }); // sand
        }
        else
        {
            return color_from({ 84, 189, 84 }); // grass
        }
        
    }, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    context.texture = LoadTextureFromImage(context.image);
    if (context.model.meshCount > 0) {
        context.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.texture;
    }
}