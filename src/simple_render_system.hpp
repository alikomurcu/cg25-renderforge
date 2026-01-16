#pragma once

#include "frg_camera.hpp"
#include "frg_descriptor.hpp"
#include "frg_device.hpp"
#include "frg_game_object.hpp"
#include "frg_lighting.hpp"
#include "frg_model.hpp"
#include "frg_particle_dispenser.hpp"
#include "frg_pipeline.hpp"
#include "frg_renderer.hpp"
// std
#include <memory>
#include <vector>

namespace frg {
class SimpleRenderSystem {
  public:
    SimpleRenderSystem(FrgDevice &device, VkRenderPass renderPass, FrgDescriptor &descriptor);
    ~SimpleRenderSystem();

    SimpleRenderSystem(const SimpleRenderSystem &) = delete;
    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;
    void renderGameObjects(
        VkCommandBuffer commandBuffer, std::vector<FrgGameObject> &gameObjects, const FrgCamera &camera, float frameTime
    );

    // Lighting interface
    LightManager &getLightManager() { return light_manager; }
    VkPipelineLayout getComputeGraphicsPipelineLayout() { return frgComputePipeline->getComputePipelineLayout(); }
    VkPipeline getComputeGraphicsPipeline() { return frgComputePipeline->getComputePipeline(); }
    std::vector<void *> &getUbosMapped() { return ubos_mapped; }
    void set_up_compute_desc_sets(size_t ssbo_size);
    void setup_ssbos(FrgParticleDispenser &dispenser);
    std::vector<VkBuffer> &getSSBOS() { return frgComputePipeline->getShaderStorageBuffers(); }
    void bindComputeGraphicsPipeline(VkCommandBuffer buff);

  private:
    void createPipelineLayout();
    void createComputeGraphicsPipelineLayout();
    void createPipeline(VkRenderPass renderPass);
    void createComputePipeline(VkRenderPass renderPass);
    void createUniformBuffers();

    FrgDevice &frgDevice;
    FrgDescriptor &frgDescriptor;
    LightManager light_manager;

    std::unique_ptr<FrgPipeline> frgPipeline;
    std::unique_ptr<FrgPipeline> frgComputePipeline;
    VkPipelineLayout pipelineLayout;
    VkPipelineLayout computeGraphicsPipelineLayout;
    std::vector<VkBuffer> ubos;
    std::vector<VkDeviceMemory> ubos_memory;
    std::vector<void *> ubos_mapped;
};
} // namespace frg