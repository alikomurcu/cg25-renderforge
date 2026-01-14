#pragma once

#include "frg_device.hpp"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <vector>

namespace frg {

/**
 * G-Buffer for deferred rendering / SSAO
 *
 * Contains:
 * - Position texture (view-space, RGBA16F)
 * - Normal texture (view-space, RGBA16F)
 * - Depth texture (reuses existing depth format)
 */
class FrgGBuffer {
public:
  FrgGBuffer(FrgDevice &device, VkExtent2D extent);
  ~FrgGBuffer();

  FrgGBuffer(const FrgGBuffer &) = delete;
  FrgGBuffer &operator=(const FrgGBuffer &) = delete;

  // Recreate G-buffer for new window size
  void resize(VkExtent2D newExtent);

  // Accessors
  VkRenderPass getRenderPass() const { return renderPass; }
  VkFramebuffer getFramebuffer() const { return framebuffer; }
  VkExtent2D getExtent() const { return extent; }

  VkImageView getPositionImageView() const { return positionImageView; }
  VkImageView getNormalImageView() const { return normalImageView; }
  VkImageView getDepthImageView() const { return depthImageView; }

  VkSampler getSampler() const { return sampler; }

  // Descriptor info for sampling in shaders
  VkDescriptorImageInfo getPositionDescriptor() const;
  VkDescriptorImageInfo getNormalDescriptor() const;
  VkDescriptorImageInfo getDepthDescriptor() const;

private:
  void createImages();
  void createImageViews();
  void createSampler();
  void createRenderPass();
  void createFramebuffer();
  void cleanup();

  FrgDevice &device;
  VkExtent2D extent;

  // Position attachment (view-space positions)
  VkImage positionImage = VK_NULL_HANDLE;
  VkDeviceMemory positionMemory = VK_NULL_HANDLE;
  VkImageView positionImageView = VK_NULL_HANDLE;

  // Normal attachment (view-space normals)
  VkImage normalImage = VK_NULL_HANDLE;
  VkDeviceMemory normalMemory = VK_NULL_HANDLE;
  VkImageView normalImageView = VK_NULL_HANDLE;

  // Depth attachment
  VkImage depthImage = VK_NULL_HANDLE;
  VkDeviceMemory depthMemory = VK_NULL_HANDLE;
  VkImageView depthImageView = VK_NULL_HANDLE;

  VkSampler sampler = VK_NULL_HANDLE;
  VkRenderPass renderPass = VK_NULL_HANDLE;
  VkFramebuffer framebuffer = VK_NULL_HANDLE;

  // Formats
  static constexpr VkFormat POSITION_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
  static constexpr VkFormat NORMAL_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
  VkFormat depthFormat;
};

} // namespace frg
