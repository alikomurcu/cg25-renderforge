#pragma once

#include "frg_model.hpp"

#include <glm/gtc/matrix_transform.hpp>

// std
#include <memory>

namespace frg
{
    struct TransformComponent
    {
        glm::vec3 translation{};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
        glm::vec3 rotation{}; // in radians

        // Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
        // Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
        // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
        glm::mat4 mat4();
        glm::mat3 normalMat();
    };

    class FrgGameObject
    {
    public:
        using id_t = unsigned int;

        static FrgGameObject createGameObject()
        {
            static id_t currentId = 0;
            return FrgGameObject{currentId++};
        }

        FrgGameObject(const FrgGameObject &) = delete;
        FrgGameObject &operator=(const FrgGameObject &) = delete;
        FrgGameObject(FrgGameObject &&) = default;
        FrgGameObject &operator=(FrgGameObject &&) = default;

        id_t getId() const { return id; }
        std::vector<VkDescriptorImageInfo> get_descriptors();

        std::shared_ptr<FrgModel> model{};
        glm::vec3 color{0.0f, 0.0f, 0.0f};
        TransformComponent transform{};

    private:
        FrgGameObject(id_t id) : id{id} { model = nullptr; }

        id_t id;
    };
} // namespace frg