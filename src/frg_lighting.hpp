#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <array>

namespace frg
{

    // Maximum number of lights that can be rendered
    constexpr uint32_t MAX_LIGHTS = 10;

    // Point Light structure - aligned for GPU transfer
    struct PointLight
    {
        glm::vec4 position; // w component unused, kept for alignment
        glm::vec4 color;    // w component is intensity
        float radius;       // attenuation radius
        float padding[3];   // padding for alignment

        PointLight();
        PointLight(glm::vec3 pos, glm::vec3 col, float intensity, float rad);
    };

    // Directional Light structure - aligned for GPU transfer
    struct DirectionalLight
    {
        glm::vec4 direction; // w component unused
        glm::vec4 color;     // w component is intensity
        float padding[4];    // padding for alignment

        DirectionalLight();
        DirectionalLight(glm::vec3 dir, glm::vec3 col, float intensity);
    };

    // Light configuration passed to GPU
    struct LightData
    {
        std::array<PointLight, MAX_LIGHTS> pointLights;
        uint32_t pointLightCount = 0;
        uint32_t padding[3]; // padding for alignment

        DirectionalLight directionalLight;

        void addPointLight(const PointLight &light);
        void clearPointLights();
    };

    // Light Manager - manages all lights in the scene
    class LightManager
    {
    public:
        LightManager() = default;
        ~LightManager() = default;

        // Point light management
        void addPointLight(glm::vec3 position, glm::vec3 color, float intensity, float radius);
        void updatePointLight(size_t index, glm::vec3 position);
        void updatePointLightColor(size_t index, glm::vec3 color, float intensity);
        void clearPointLights();

        // Directional light management
        void setDirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity);

        // Get light data for GPU
        LightData getLightData() const;

        const std::vector<PointLight> &getPointLights() const { return point_lights; }
        const DirectionalLight &getDirectionalLight() const { return directional_light; }
        size_t getPointLightCount() const { return point_lights.size(); }

    private:
        std::vector<PointLight> point_lights;
        DirectionalLight directional_light;
    };

} // namespace frg
