#include "ssao_render_system.hpp"

#include <array>
#include <cassert>
#include <stdexcept>

namespace frg {

SSAORenderSystem::SSAORenderSystem(FrgDevice &device, FrgGBuffer &gbuffer,
                                   FrgSSAO &ssao)
    : frgDevice{device}, gbuffer{gbuffer}, ssao{ssao} {
  createDescriptorSetLayouts();
  createDescriptorPool();
  createDescriptorSets();
  createGBufferPipelineLayout();
  createGBufferPipeline();
  createSSAOPipelineLayout();
  createSSAOPipeline();
  createBlurPipelineLayout();
  createBlurPipeline();
}

SSAORenderSystem::~SSAORenderSystem() {
  VkDevice dev = frgDevice.device();

  if (blurPipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(dev, blurPipelineLayout, nullptr);
  }
  if (ssaoPipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(dev, ssaoPipelineLayout, nullptr);
  }
  if (gbufferPipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(dev, gbufferPipelineLayout, nullptr);
  }
  if (descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(dev, descriptorPool, nullptr);
  }
  if (blurDescriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(dev, blurDescriptorSetLayout, nullptr);
  }
  if (ssaoDescriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(dev, ssaoDescriptorSetLayout, nullptr);
  }
}

void SSAORenderSystem::createDescriptorSetLayouts() {
  // SSAO descriptor set layout
  // Binding 0: gPosition   (sampler2D)
  // Binding 1: gNormal     (sampler2D)
  // Binding 2: texNoise    (sampler2D)
  // Binding 3: kernel      (uniform buffer)
  {
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(frgDevice.device(), &layoutInfo, nullptr,
                                    &ssaoDescriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create SSAO descriptor set layout!");
    }
  }

  // Blur descriptor set layout
  // Binding 0: ssaoInput (sampler2D)
  {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    if (vkCreateDescriptorSetLayout(frgDevice.device(), &layoutInfo, nullptr,
                                    &blurDescriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create blur descriptor set layout!");
    }
  }
}

void SSAORenderSystem::createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[0].descriptorCount = 5; // 3 for SSAO + 1 for blur + extra
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[1].descriptorCount = 1; // kernel buffer

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 2; // SSAO + blur

  if (vkCreateDescriptorPool(frgDevice.device(), &poolInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create SSAO descriptor pool!");
  }
}

void SSAORenderSystem::createDescriptorSets() {
  // Allocate SSAO descriptor set
  {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &ssaoDescriptorSetLayout;

    if (vkAllocateDescriptorSets(frgDevice.device(), &allocInfo,
                                 &ssaoDescriptorSet) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate SSAO descriptor set!");
    }

    // Update descriptor set
    VkDescriptorImageInfo positionInfo = gbuffer.getPositionDescriptor();
    VkDescriptorImageInfo normalInfo = gbuffer.getNormalDescriptor();
    VkDescriptorImageInfo noiseInfo = ssao.getNoiseDescriptor();
    VkDescriptorBufferInfo kernelInfo = ssao.getKernelDescriptor();

    std::array<VkWriteDescriptorSet, 4> writes{};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = ssaoDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &positionInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = ssaoDescriptorSet;
    writes[1].dstBinding = 1;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &normalInfo;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = ssaoDescriptorSet;
    writes[2].dstBinding = 2;
    writes[2].dstArrayElement = 0;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;
    writes[2].pImageInfo = &noiseInfo;

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = ssaoDescriptorSet;
    writes[3].dstBinding = 3;
    writes[3].dstArrayElement = 0;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[3].descriptorCount = 1;
    writes[3].pBufferInfo = &kernelInfo;

    vkUpdateDescriptorSets(frgDevice.device(),
                           static_cast<uint32_t>(writes.size()), writes.data(),
                           0, nullptr);
  }

  // Allocate blur descriptor set
  {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &blurDescriptorSetLayout;

    if (vkAllocateDescriptorSets(frgDevice.device(), &allocInfo,
                                 &blurDescriptorSet) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate blur descriptor set!");
    }

    // Update descriptor set
    VkDescriptorImageInfo ssaoInfo = ssao.getSSAODescriptor();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = blurDescriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &ssaoInfo;

    vkUpdateDescriptorSets(frgDevice.device(), 1, &write, 0, nullptr);
  }
}

void SSAORenderSystem::createGBufferPipelineLayout() {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(GBufferPushConstants);

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 0;
  layoutInfo.pSetLayouts = nullptr;
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(frgDevice.device(), &layoutInfo, nullptr,
                             &gbufferPipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create G-buffer pipeline layout!");
  }
}

void SSAORenderSystem::createGBufferPipeline() {
  assert(gbufferPipelineLayout != nullptr &&
         "Cannot create pipeline before layout!");

  PipelineConfigInfo pipelineConfig{};
  FrgPipeline::defaultPipelineConfigInfo(pipelineConfig);

  // G-buffer has 2 color attachments (position + normal)
  pipelineConfig.colorBlendAttachments.resize(2);
  pipelineConfig.colorBlendAttachments[0].colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  pipelineConfig.colorBlendAttachments[0].blendEnable = VK_FALSE;
  pipelineConfig.colorBlendAttachments[1] =
      pipelineConfig.colorBlendAttachments[0];

  pipelineConfig.colorBlendInfo.attachmentCount = 2;
  pipelineConfig.colorBlendInfo.pAttachments =
      pipelineConfig.colorBlendAttachments.data();

  pipelineConfig.renderPass = gbuffer.getRenderPass();
  pipelineConfig.pipelineLayout = gbufferPipelineLayout;

  gbufferPipeline =
      std::make_unique<FrgPipeline>(frgDevice, "shaders/gbuffer.vert.spv",
                                    "shaders/gbuffer.frag.spv", pipelineConfig);
}

void SSAORenderSystem::createSSAOPipelineLayout() {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(SSAOPushConstants);

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &ssaoDescriptorSetLayout;
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(frgDevice.device(), &layoutInfo, nullptr,
                             &ssaoPipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create SSAO pipeline layout!");
  }
}

void SSAORenderSystem::createSSAOPipeline() {
  assert(ssaoPipelineLayout != nullptr &&
         "Cannot create pipeline before layout!");

  PipelineConfigInfo pipelineConfig{};
  FrgPipeline::defaultPipelineConfigInfo(pipelineConfig);

  // Fullscreen quad - no vertex input
  pipelineConfig.bindingDescriptions.clear();
  pipelineConfig.attributeDescriptions.clear();

  // No depth testing for fullscreen pass
  pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
  pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

  pipelineConfig.renderPass = ssao.getSSAORenderPass();
  pipelineConfig.pipelineLayout = ssaoPipelineLayout;

  ssaoPipeline =
      std::make_unique<FrgPipeline>(frgDevice, "shaders/ssao.vert.spv",
                                    "shaders/ssao.frag.spv", pipelineConfig);
}

void SSAORenderSystem::createBlurPipelineLayout() {
  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &blurDescriptorSetLayout;
  layoutInfo.pushConstantRangeCount = 0;
  layoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(frgDevice.device(), &layoutInfo, nullptr,
                             &blurPipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create blur pipeline layout!");
  }
}

void SSAORenderSystem::createBlurPipeline() {
  assert(blurPipelineLayout != nullptr &&
         "Cannot create pipeline before layout!");

  PipelineConfigInfo pipelineConfig{};
  FrgPipeline::defaultPipelineConfigInfo(pipelineConfig);

  // Fullscreen quad - no vertex input
  pipelineConfig.bindingDescriptions.clear();
  pipelineConfig.attributeDescriptions.clear();

  // No depth testing for fullscreen pass
  pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
  pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

  pipelineConfig.renderPass = ssao.getBlurRenderPass();
  pipelineConfig.pipelineLayout = blurPipelineLayout;

  // Reuse ssao.vert for blur pass
  blurPipeline = std::make_unique<FrgPipeline>(
      frgDevice, "shaders/ssao.vert.spv", "shaders/ssao_blur.frag.spv",
      pipelineConfig);
}

void SSAORenderSystem::beginGBufferPass(VkCommandBuffer commandBuffer) {
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = gbuffer.getRenderPass();
  renderPassInfo.framebuffer = gbuffer.getFramebuffer();
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = gbuffer.getExtent();

  // Clear values for position, normal, and depth
  std::array<VkClearValue, 3> clearValues{};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}}; // Position
  clearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}}; // Normal
  clearValues[2].depthStencil = {1.0f, 0};           // Depth

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Set viewport and scissor
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(gbuffer.getExtent().width);
  viewport.height = static_cast<float>(gbuffer.getExtent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{{0, 0}, gbuffer.getExtent()};

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void SSAORenderSystem::endGBufferPass(VkCommandBuffer commandBuffer) {
  vkCmdEndRenderPass(commandBuffer);
}

void SSAORenderSystem::beginSSAOPass(VkCommandBuffer commandBuffer) {
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = ssao.getSSAORenderPass();
  renderPassInfo.framebuffer = ssao.getSSAOFramebuffer();
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = ssao.getExtent();

  VkClearValue clearValue{};
  clearValue.color = {{1.0f, 1.0f, 1.0f, 1.0f}}; // Default to no occlusion

  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(ssao.getExtent().width);
  viewport.height = static_cast<float>(ssao.getExtent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{{0, 0}, ssao.getExtent()};

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void SSAORenderSystem::endSSAOPass(VkCommandBuffer commandBuffer) {
  vkCmdEndRenderPass(commandBuffer);
}

void SSAORenderSystem::beginBlurPass(VkCommandBuffer commandBuffer) {
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = ssao.getBlurRenderPass();
  renderPassInfo.framebuffer = ssao.getBlurFramebuffer();
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = ssao.getExtent();

  VkClearValue clearValue{};
  clearValue.color = {{1.0f, 1.0f, 1.0f, 1.0f}};

  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(ssao.getExtent().width);
  viewport.height = static_cast<float>(ssao.getExtent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{{0, 0}, ssao.getExtent()};

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void SSAORenderSystem::endBlurPass(VkCommandBuffer commandBuffer) {
  vkCmdEndRenderPass(commandBuffer);
}

void SSAORenderSystem::renderGBuffer(VkCommandBuffer commandBuffer,
                                     std::vector<FrgGameObject> &gameObjects,
                                     const FrgCamera &camera) {
  gbufferPipeline->bind(commandBuffer);

  for (auto &gameObject : gameObjects) {
    GBufferPushConstants push{};
    push.modelView = camera.getViewMatrix() * gameObject.transform.mat4();
    push.projection = camera.getProjectionMatrix();
    push.normalMat = glm::transpose(glm::inverse(push.modelView));

    vkCmdPushConstants(commandBuffer, gbufferPipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(GBufferPushConstants), &push);

    gameObject.model->bind(commandBuffer);
    gameObject.model->draw(commandBuffer);
  }
}

void SSAORenderSystem::renderSSAO(VkCommandBuffer commandBuffer,
                                  const FrgCamera &camera) {
  ssaoPipeline->bind(commandBuffer);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          ssaoPipelineLayout, 0, 1, &ssaoDescriptorSet, 0,
                          nullptr);

  SSAOPushConstants push{};
  push.projection = camera.getProjectionMatrix();
  push.noiseScale = glm::vec2(static_cast<float>(ssao.getExtent().width) /
                                  static_cast<float>(FrgSSAO::NOISE_SIZE),
                              static_cast<float>(ssao.getExtent().height) /
                                  static_cast<float>(FrgSSAO::NOISE_SIZE));
  push.radius = FrgSSAO::RADIUS;
  push.bias = FrgSSAO::BIAS;
  push.kernelSize = FrgSSAO::KERNEL_SIZE;

  vkCmdPushConstants(commandBuffer, ssaoPipelineLayout,
                     VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SSAOPushConstants),
                     &push);

  // Draw fullscreen triangle (3 vertices, no vertex buffer)
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void SSAORenderSystem::renderBlur(VkCommandBuffer commandBuffer) {
  blurPipeline->bind(commandBuffer);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          blurPipelineLayout, 0, 1, &blurDescriptorSet, 0,
                          nullptr);

  // Draw fullscreen triangle
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

} // namespace frg
