#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace frg
{
    class FrgCamera
    {
    public:
        void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
        void setPerspectiveProjection(float fovy, float aspect, float near, float far);

        void setViewDirection(const glm::vec3 &position, const glm::vec3 &direction, const glm::vec3 &up = glm::vec3{0, -1, 0});
        void setViewTarget(const glm::vec3 &position, const glm::vec3 &target, const glm::vec3 &up = glm::vec3{0, -1, 0});
        void setViewYXZ(const glm::vec3 &position, const glm::vec3 &rotation);

        const glm::mat4 &getProjectionMatrix() const { return projectionMatrix; }
        const glm::mat4 &getViewMatrix() const { return viewMatrix; }

    private:
        glm::mat4 projectionMatrix{1.f};
        glm::mat4 viewMatrix{1.f};
    };

} // namespace frg
