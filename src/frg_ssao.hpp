#pragma once

#include "frg_device.hpp"
#include "frg_gbuffer.hpp"

// libs
#include <glm/glm.hpp>

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <vector>

namespace frg {

/**
 * SSAO (Screen-Space Ambient Occlusion) system
 *
 * Implements SSAO following the LearnOpenGL approach:
 * 1. Generate hemisphere sample kernel
 * 2. Create 4x4 noise texture for random rotations
 * 3. SSAO pass: calculate occlusion using G-buffer
 * 4. Blur pass: smooth the noisy SSAO output
 */
class FrgSSAO {
public:
  // SSAO parameters
  static constexpr int KERNEL_SIZE = 64;
  static constexpr int NOISE_SIZE = 4; // 4x4 noise texture
  static constexpr float RADIUS = 0.5f;
  static constexpr float BIAS = 0.025f;

  FrgSSAO(FrgDevice &device, VkExtent2D extent);
  ~FrgSSAO();

  FrgSSAO(const FrgSSAO &) = delete;
  FrgSSAO &operator=(const FrgSSAO &) = delete;

  // Recreate for new window size
  void resize(VkExtent2D newExtent);

  // Accessors
  VkRenderPass getSSAORenderPass() const { return ssaoRenderPass; }
  VkRenderPass getBlurRenderPass() const { return blurRenderPass; }
  VkFramebuffer getSSAOFramebuffer() const { return ssaoFramebuffer; }
  VkFramebuffer getBlurFramebuffer() const { return blurFramebuffer; }
  VkExtent2D getExtent() const { return extent; }

  // SSAO output (after blur)
  VkImageView getSSAOImageView() const { return ssaoImageView; }
  VkImageView getBlurredImageView() const { return blurredImageView; }
  VkSampler getSampler() const { return sampler; }

  // Noise texture for shader
  VkImageView getNoiseImageView() const { return noiseImageView; }
  VkSampler getNoiseSampler() const { return noiseSampler; }

  // Kernel samples for shader (uniform buffer)
  const std::vector<glm::vec4> &getKernel() const { return kernel; }
  VkBuffer getKernelBuffer() const { return kernelBuffer; }

  // Descriptor info
  VkDescriptorImageInfo getSSAODescriptor() const;
  VkDescriptorImageInfo getBlurredDescriptor() const;
  VkDescriptorImageInfo getNoiseDescriptor() const;
  VkDescriptorBufferInfo getKernelDescriptor() const;

private:
  void generateKernel();
  void createNoiseTexture();
  void createKernelBuffer();
  void createSSAOImage();
  void createBlurImage();
  void createSamplers();
  void createSSAORenderPass();
  void createBlurRenderPass();
  void createFramebuffers();
  void cleanup();

  FrgDevice &device;
  VkExtent2D extent;

  // Sample kernel (hemisphere samples in tangent space)
  std::vector<glm::vec4> kernel; // vec4 for std140 alignment
  VkBuffer kernelBuffer = VK_NULL_HANDLE;
  VkDeviceMemory kernelMemory = VK_NULL_HANDLE;

  // Noise texture (4x4 random rotation vectors)
  VkImage noiseImage = VK_NULL_HANDLE;
  VkDeviceMemory noiseMemory = VK_NULL_HANDLE;
  VkImageView noiseImageView = VK_NULL_HANDLE;
  VkSampler noiseSampler = VK_NULL_HANDLE;

  // SSAO output texture
  VkImage ssaoImage = VK_NULL_HANDLE;
  VkDeviceMemory ssaoMemory = VK_NULL_HANDLE;
  VkImageView ssaoImageView = VK_NULL_HANDLE;

  // Blurred SSAO texture
  VkImage blurredImage = VK_NULL_HANDLE;
  VkDeviceMemory blurredMemory = VK_NULL_HANDLE;
  VkImageView blurredImageView = VK_NULL_HANDLE;

  VkSampler sampler = VK_NULL_HANDLE;
  VkRenderPass ssaoRenderPass = VK_NULL_HANDLE;
  VkRenderPass blurRenderPass = VK_NULL_HANDLE;
  VkFramebuffer ssaoFramebuffer = VK_NULL_HANDLE;
  VkFramebuffer blurFramebuffer = VK_NULL_HANDLE;

  // Format for SSAO textures (single channel, 8-bit is enough for AO)
  static constexpr VkFormat SSAO_FORMAT = VK_FORMAT_R8_UNORM;
};

} // namespace frg
