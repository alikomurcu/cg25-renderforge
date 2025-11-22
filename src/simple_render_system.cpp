#include "simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace frg {
struct SimplePushConstantData {
    glm::mat4 transform{1.f};
    alignas(16) glm::vec3 color;
};

SimpleRenderSystem::SimpleRenderSystem(FrgDevice &device,
                                       VkRenderPass renderPass,
                                       FrgDescriptor &descriptor)
    : frgDevice{device}, frgDescriptor{descriptor} {
    createPipelineLayout();
    createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
    vkDestroyPipelineLayout(frgDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = frgDescriptor.descriptorSetCount();
    pipelineLayoutInfo.pSetLayouts = frgDescriptor.descriptorSetLayout();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(frgDevice.device(),
                               &pipelineLayoutInfo,
                               nullptr,
                               &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr &&
           "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    FrgPipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    frgPipeline = std::make_unique<FrgPipeline>(frgDevice,
                                                "shaders/triangle.vert.spv",
                                                "shaders/triangle.frag.spv",
                                                pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(
    VkCommandBuffer commandBuffer, std::vector<FrgGameObject> &gameObjects,
    const FrgCamera &camera) {
    frgPipeline->bind(commandBuffer);
    auto projectionView = camera.getProjectionMatrix() * camera.getViewMatrix();

    for (auto &gameObject : gameObjects) {
        SimplePushConstantData push{};
        push.color = gameObject.color;
        push.transform = projectionView * gameObject.transform.mat4();
        vkCmdPushConstants(commandBuffer,
                           pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT |
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(SimplePushConstantData),
                           &push);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout,
                                0,
                                frgDescriptor.descriptorSetCount(),
                                frgDescriptor.descriptorSet(),
                                0,
                                nullptr);
        gameObject.model->draw(commandBuffer);
    }
}
} // namespace frg
