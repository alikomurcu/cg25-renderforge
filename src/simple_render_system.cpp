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

SimpleRenderSystem::SimpleRenderSystem(
    FrgDevice &device, VkRenderPass renderPass, FrgDescriptor &descriptor
)
    : frgDevice{device}, frgDescriptor{descriptor} {
    createPipelineLayout();
    createPipeline(renderPass);
    create_storage_buffers();
}

SimpleRenderSystem::~SimpleRenderSystem() {
    vkDestroyPipelineLayout(frgDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::create_storage_buffers() {
    storage_buffers.resize(1);
    FrgSsbo &storage_buffer = storage_buffers[0];
    frgDevice.createBuffer()
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

    if (vkCreatePipelineLayout(
            frgDevice.device(),
            &pipelineLayoutInfo,
            nullptr,
            &pipelineLayout
        ) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(
        pipelineLayout != nullptr &&
        "Cannot create pipeline before pipeline layout"
    );

    PipelineConfigInfo pipelineConfig{};
    FrgPipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    frgPipeline = std::make_unique<FrgPipeline>(
        frgDevice,
        "shaders/triangle.vert.spv",
        "shaders/triangle.frag.spv",
        pipelineConfig
    );
}

void SimpleRenderSystem::renderGameObjects(
    VkCommandBuffer commandBuffer, std::vector<FrgGameObject> &gameObjects,
    const FrgCamera &camera, float frameTime
) {
    frgPipeline->bind(commandBuffer);
    auto projectionView = camera.getProjectionMatrix() * camera.getViewMatrix();

    // Track total time for orbit animation
    static float totalTime = 0.f;
    totalTime += frameTime;

    // Update light position based on orbit animation
    float angle = totalTime * glm::radians(90.0f); // 90 degrees per second
    float radius = 3.0f;
    glm::vec3 lightPos =
        glm::vec3(radius * glm::cos(angle), 1.5f, radius * glm::sin(angle));

    // Update the light manager with the orbiting point light
    if (light_manager.getPointLightCount() > 0) {
        light_manager.updatePointLight(0, lightPos);
    }

    for (auto &gameObject : gameObjects) {
        SimplePushConstantData push{};
        auto modelMat = gameObject.transform.mat4();
        push.transform = projectionView * modelMat;
        push.normalMat = gameObject.transform.normalMat();

        // Get light data from manager
        LightData lightData = light_manager.getLightData();
        if (lightData.pointLightCount > 0) {
            push.pointLightPosition = lightData.pointLights[0].position;
            push.pointLightColor = lightData.pointLights[0].color;
        }

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            frgDescriptor.descriptorSetCount(),
            frgDescriptor.descriptorSet(),
            0,
            nullptr
        );
        gameObject.model->draw(commandBuffer, pipelineLayout, push);
    }
}
} // namespace frg
