#pragma once

#include <generation.hpp>
#include <glm/glm.hpp>
#include <functional>

float perlinNoise(glm::vec2 const& position);
float perlinNoiseSeeded(glm::vec2 const& position, int seed);

float octaveNoise(glm::vec2 const& position, std::function<float(glm::vec2 const&)> noiseFunction, float scale, AppContext& context);
float radialMask(glm::vec2 const &p, RadialMaskGenerationParameters &maskGenerationParameters);