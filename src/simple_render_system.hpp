#pragma once

#include "frg_device.hpp"
#include "frg_game_object.hpp"
#include "frg_pipeline.hpp"
#include "frg_model.hpp"
#include "frg_camera.hpp"

// std
#include <memory>
#include <vector>

namespace frg
{
    class SimpleRenderSystem
    {
    public:
        SimpleRenderSystem(FrgDevice &device, VkRenderPass renderPass);
        ~SimpleRenderSystem();

        SimpleRenderSystem(const SimpleRenderSystem &) = delete;
        SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;
        void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<FrgGameObject> &gameObjects, const FrgCamera &camera);

    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);

        FrgDevice &frgDevice;

        std::unique_ptr<FrgPipeline> frgPipeline;
        VkPipelineLayout pipelineLayout;
    };
} // namespace frg