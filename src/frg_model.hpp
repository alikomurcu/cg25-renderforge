#pragma once

#include "frg_device.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <vector>

namespace frg
{
    class FrgModel
    {
    public:
        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 color;
            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        };

        FrgModel(
            FrgDevice &device,
            const std::vector<Vertex> &vertices);
        ~FrgModel();

        FrgModel(const FrgModel &) = delete;
        FrgModel &operator=(const FrgModel &) = delete;

        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);

    private:
        void createVertexBuffers(const std::vector<Vertex> &vertices);
        FrgDevice &frgDevice;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        uint32_t vertexCount;
    };
} // namespace frg