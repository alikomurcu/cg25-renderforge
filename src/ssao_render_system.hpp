#pragma once

#include "frg_camera.hpp"
#include "frg_device.hpp"
#include "frg_game_object.hpp"
#include "frg_gbuffer.hpp"
#include "frg_pipeline.hpp"
#include "frg_ssao.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <memory>
#include <vector>

namespace frg {

/**
 * SSAO Render System
 *
 * Manages all SSAO-related rendering:
 * - G-Buffer pass (geometry to position/normal textures)
 * - SSAO pass (calculate ambient occlusion)
 * - Blur pass (smooth SSAO output)
 */
class SSAORenderSystem {
public:
  // Push constants for G-buffer pass
  struct GBufferPushConstants {
    glm::mat4 modelView;
    glm::mat4 projection;
    glm::mat4 normalMat;
  };

  // Push constants for SSAO pass
  struct SSAOPushConstants {
    glm::mat4 projection;
    glm::vec2 noiseScale;
    float radius;
    float bias;
    int kernelSize;
  };

  SSAORenderSystem(FrgDevice &device, FrgGBuffer &gbuffer, FrgSSAO &ssao);
  ~SSAORenderSystem();

  SSAORenderSystem(const SSAORenderSystem &) = delete;
  SSAORenderSystem &operator=(const SSAORenderSystem &) = delete;

  // Render passes
  void renderGBuffer(VkCommandBuffer commandBuffer,
                     std::vector<FrgGameObject> &gameObjects,
                     const FrgCamera &camera);

  void renderSSAO(VkCommandBuffer commandBuffer, const FrgCamera &camera);

  void renderBlur(VkCommandBuffer commandBuffer);

  // Begin/end render passes
  void beginGBufferPass(VkCommandBuffer commandBuffer);
  void endGBufferPass(VkCommandBuffer commandBuffer);

  void beginSSAOPass(VkCommandBuffer commandBuffer);
  void endSSAOPass(VkCommandBuffer commandBuffer);

  void beginBlurPass(VkCommandBuffer commandBuffer);
  void endBlurPass(VkCommandBuffer commandBuffer);

private:
  void createDescriptorSetLayouts();
  void createDescriptorPool();
  void createDescriptorSets();
  void createGBufferPipelineLayout();
  void createGBufferPipeline();
  void createSSAOPipelineLayout();
  void createSSAOPipeline();
  void createBlurPipelineLayout();
  void createBlurPipeline();

  FrgDevice &frgDevice;
  FrgGBuffer &gbuffer;
  FrgSSAO &ssao;

  // Descriptor management
  VkDescriptorSetLayout ssaoDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout blurDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet ssaoDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet blurDescriptorSet = VK_NULL_HANDLE;

  // G-buffer pipeline
  VkPipelineLayout gbufferPipelineLayout = VK_NULL_HANDLE;
  std::unique_ptr<FrgPipeline> gbufferPipeline;

  // SSAO pipeline
  VkPipelineLayout ssaoPipelineLayout = VK_NULL_HANDLE;
  std::unique_ptr<FrgPipeline> ssaoPipeline;

  // Blur pipeline
  VkPipelineLayout blurPipelineLayout = VK_NULL_HANDLE;
  std::unique_ptr<FrgPipeline> blurPipeline;
};

} // namespace frg
