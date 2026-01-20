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

SimpleRenderSystem::SimpleRenderSystem(FrgDevice &device, VkRenderPass renderPass,
                                       FrgDescriptor &descriptor, LightManager &lightManagerPtr)
    : frgDevice{device}, frgDescriptor{descriptor}, lightManager{lightManagerPtr} {
  createPipelineLayout();
  createPipeline(renderPass);
  createComputeGraphicsPipelineLayout();
  createComputePipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
  vkDestroyPipelineLayout(frgDevice.device(), pipelineLayout, nullptr);
  if (computeGraphicsPipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(frgDevice.device(), computeGraphicsPipelineLayout, nullptr);

  for (size_t i = 0; i < FrgSwapChain::MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroyBuffer(frgDevice.device(), ubos[i], nullptr);
    vkFreeMemory(frgDevice.device(), ubos_memory[i], nullptr);
  }
}

void SimpleRenderSystem::set_up_compute_desc_sets(size_t ssbo_size) {
  frgDescriptor.write_comp_descriptor_sets(
        ubos,
        sizeof(UniformBufferObject),
        frgComputePipeline->getShaderStorageBuffers(),
        ssbo_size
    );
}

void SimpleRenderSystem::setup_ssbos(FrgParticleDispenser &dispenser) {
    dispenser.cpy_host2dev(
        frgComputePipeline->getShaderStorageBuffers(),
        frgComputePipeline->getShaderStorageBuffersMemory()
    );
}

void SimpleRenderSystem::bindComputeGraphicsPipeline(VkCommandBuffer buff) { frgComputePipeline->bind(buff); }

void SimpleRenderSystem::createPipelineLayout() {
  VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(SimplePushConstantData);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = frgDescriptor.descriptorSetCount();
  pipelineLayoutInfo.pSetLayouts = frgDescriptor.descriptorSetLayout();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(frgDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void SimpleRenderSystem::createComputeGraphicsPipelineLayout() {
  VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(SimplePushConstantData);
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = frgDescriptor.getComputeDescriptorSetCount();
    pipelineLayoutInfo.pSetLayouts = frgDescriptor.getComputeDescriptorSetLayout();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(frgDevice.device(), &pipelineLayoutInfo, nullptr, &computeGraphicsPipelineLayout) !=
        VK_SUCCESS)
    {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

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

void SimpleRenderSystem::createComputePipeline(VkRenderPass renderPass) {
    assert(computeGraphicsPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipelineConfig{};
  FrgPipeline::defaultPipelineConfigInfo(pipelineConfig, true);
  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = computeGraphicsPipelineLayout;
  std::vector<VkDescriptorSetLayout> inp_layouts;
  inp_layouts.push_back(*frgDescriptor.getComputeDescriptorSetLayout());
  frgComputePipeline = std::make_unique<FrgPipeline>(
        frgDevice,
        "shaders/particles.vert.spv",
        "shaders/particles.frag.spv",
        "shaders/particles.comp.spv",
        pipelineConfig,
        inp_layouts
    );
  createUniformBuffers();
}

void SimpleRenderSystem::createUniformBuffers() {
  VkDeviceSize buff_size = sizeof(UniformBufferObject);

  ubos.resize(FrgSwapChain::MAX_FRAMES_IN_FLIGHT);
  ubos_memory.resize(FrgSwapChain::MAX_FRAMES_IN_FLIGHT);
  ubos_mapped.resize(FrgSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < FrgSwapChain::MAX_FRAMES_IN_FLIGHT; ++i) {
        frgDevice.createBuffer(
            buff_size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            ubos[i],
            ubos_memory[i]
        );

        vkMapMemory(frgDevice.device(), ubos_memory[i], 0, buff_size, 0, &ubos_mapped[i]);
  }
}

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer,
                                           std::vector<FrgGameObject> &gameObjects,
                                           const FrgCamera &camera, float frameTime,
                                           VkExtent2D screenSize, int debugMode) {
  frgPipeline->bind(commandBuffer);
  auto projectionView = camera.getProjectionMatrix() * camera.getViewMatrix();

  // Track total time for orbit animation
  static float totalTime = 0.f;
  totalTime += frameTime;

  // Update light position based on orbit animation
  float angle = totalTime * glm::radians(30.0f); // 90 degrees per second
  //angle = 0.f;
  float radius = 5.0f;
    glm::vec3 lightPos = glm::vec3(radius * glm::cos(angle), 1.5f, radius * glm::sin(angle));

  // Update the light manager with the orbiting point light
  if (lightManager.getPointLightCount() > 0) {
    lightManager.updatePointLight(0, lightPos);
  }

  for (auto &gameObject : gameObjects) {
    SimplePushConstantData push{};
    auto modelMat = gameObject.transform.mat4();
    push.transform = projectionView * modelMat;
    push.modelMatrix = modelMat;
    push.normalMat = gameObject.transform.normalMat();
        push.screenSize =
            glm::vec2(static_cast<float>(screenSize.width), static_cast<float>(screenSize.height));
    push.debugMode = debugMode;

    // Get light data from manager
    LightData lightData = lightManager.getLightData();
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
