#include "frg_lighting.hpp"
#include <glm/gtc/constants.hpp>

namespace frg
{
    // PointLight implementation
    PointLight::PointLight()
        : position(0.0f, 0.0f, 0.0f, 1.0f),
          color(1.0f, 1.0f, 1.0f, 1.0f),
          radius(10.0f) {}

    PointLight::PointLight(glm::vec3 pos, glm::vec3 col, float intensity, float rad)
        : position(pos, 1.0f),
          color(col, intensity),
          radius(rad) {}

    // DirectionalLight implementation
    DirectionalLight::DirectionalLight()
        : direction(0.0f, -1.0f, 0.0f, 0.0f),
          color(1.0f, 1.0f, 1.0f, 1.0f) {}

    DirectionalLight::DirectionalLight(glm::vec3 dir, glm::vec3 col, float intensity)
        : direction(dir, 0.0f),
          color(col, intensity) {}

    // LightData implementation
    void LightData::addPointLight(const PointLight &light)
    {
        if (pointLightCount < MAX_LIGHTS)
        {
            pointLights[pointLightCount++] = light;
        }
    }

    void LightData::clearPointLights()
    {
        pointLightCount = 0;
    }

    // LightManager implementation
    void LightManager::addPointLight(glm::vec3 position, glm::vec3 color, float intensity, float radius)
    {
        if (point_lights.size() < MAX_LIGHTS)
        {
            point_lights.emplace_back(position, color, intensity, radius);
        }
    }

    void LightManager::updatePointLight(size_t index, glm::vec3 position)
    {
        if (index < point_lights.size())
        {
            point_lights[index].position = glm::vec4(position, 1.0f);
        }
    }

    void LightManager::updatePointLightColor(size_t index, glm::vec3 color, float intensity)
    {
        if (index < point_lights.size())
        {
            point_lights[index].color = glm::vec4(color, intensity);
        }
    }

    void LightManager::clearPointLights()
    {
        point_lights.clear();
    }

    void LightManager::setDirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity)
    {
        directional_light = DirectionalLight(glm::normalize(direction), color, intensity);
    }

    LightData LightManager::getLightData() const
    {
        LightData data{};

        // Copy point lights
        for (size_t i = 0; i < point_lights.size() && i < MAX_LIGHTS; ++i)
        {
            data.pointLights[i] = point_lights[i];
            data.pointLightCount++;
        }

        // Copy directional light
        data.directionalLight = directional_light;

        return data;
    }
} // namespace frg
