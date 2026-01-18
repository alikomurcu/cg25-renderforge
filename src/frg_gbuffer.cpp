#include "frg_gbuffer.hpp"

#include <array>
#include <stdexcept>

namespace frg {

FrgGBuffer::FrgGBuffer(FrgDevice &device, VkExtent2D extent)
    : device{device}, extent{extent} {
  // Find depth format
  depthFormat = device.findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  createImages();
  createImageViews();
  createSampler();
  createRenderPass();
  createFramebuffer();
}

FrgGBuffer::~FrgGBuffer() { cleanup(); }

void FrgGBuffer::resize(VkExtent2D newExtent) {
  vkDeviceWaitIdle(device.device());
  cleanup();
  extent = newExtent;
  createImages();
  createImageViews();
  createSampler();
  createRenderPass();
  createFramebuffer();
}

void FrgGBuffer::cleanup() {
  VkDevice dev = device.device();

  if (framebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(dev, framebuffer, nullptr);
    framebuffer = VK_NULL_HANDLE;
  }
  if (renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(dev, renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;
  }
  if (sampler != VK_NULL_HANDLE) {
    vkDestroySampler(dev, sampler, nullptr);
    sampler = VK_NULL_HANDLE;
  }

  // Cleanup position
  if (positionImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(dev, positionImageView, nullptr);
    positionImageView = VK_NULL_HANDLE;
  }
  if (positionImage != VK_NULL_HANDLE) {
    vkDestroyImage(dev, positionImage, nullptr);
    positionImage = VK_NULL_HANDLE;
  }
  if (positionMemory != VK_NULL_HANDLE) {
    vkFreeMemory(dev, positionMemory, nullptr);
    positionMemory = VK_NULL_HANDLE;
  }

  // Cleanup normal
  if (normalImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(dev, normalImageView, nullptr);
    normalImageView = VK_NULL_HANDLE;
  }
  if (normalImage != VK_NULL_HANDLE) {
    vkDestroyImage(dev, normalImage, nullptr);
    normalImage = VK_NULL_HANDLE;
  }
  if (normalMemory != VK_NULL_HANDLE) {
    vkFreeMemory(dev, normalMemory, nullptr);
    normalMemory = VK_NULL_HANDLE;
  }

  // Cleanup depth
  if (depthImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(dev, depthImageView, nullptr);
    depthImageView = VK_NULL_HANDLE;
  }
  if (depthImage != VK_NULL_HANDLE) {
    vkDestroyImage(dev, depthImage, nullptr);
    depthImage = VK_NULL_HANDLE;
  }
  if (depthMemory != VK_NULL_HANDLE) {
    vkFreeMemory(dev, depthMemory, nullptr);
    depthMemory = VK_NULL_HANDLE;
  }
}

void FrgGBuffer::createImages() {
  // Position image
  {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = POSITION_FORMAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               positionImage, positionMemory);
  }

  // Normal image
  {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = NORMAL_FORMAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               normalImage, normalMemory);
  }

  // Depth image
  {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               depthImage, depthMemory);
  }
}

void FrgGBuffer::createImageViews() {
  // Position image view
  {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = positionImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = POSITION_FORMAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device.device(), &viewInfo, nullptr,
                          &positionImageView) != VK_SUCCESS) {
      throw std::runtime_error(
          "Failed to create G-buffer position image view!");
    }
  }

  // Normal image view
  {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = normalImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = NORMAL_FORMAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device.device(), &viewInfo, nullptr,
                          &normalImageView) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create G-buffer normal image view!");
    }
  }

  // Depth image view
  {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device.device(), &viewInfo, nullptr,
                          &depthImageView) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create G-buffer depth image view!");
    }
  }
}

void FrgGBuffer::createSampler() {
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_NEAREST; // Nearest for G-buffer sampling
  samplerInfo.minFilter = VK_FILTER_NEAREST;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create G-buffer sampler!");
  }
}

void FrgGBuffer::createRenderPass() {
  // Attachment 0: Position (color)
  VkAttachmentDescription positionAttachment{};
  positionAttachment.format = POSITION_FORMAT;
  positionAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  positionAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  positionAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  positionAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  positionAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  positionAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  positionAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // Attachment 1: Normal (color)
  VkAttachmentDescription normalAttachment{};
  normalAttachment.format = NORMAL_FORMAT;
  normalAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  normalAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // Attachment 2: Depth
  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = depthFormat;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

  // Color attachment references
  std::array<VkAttachmentReference, 2> colorRefs{};
  colorRefs[0].attachment = 0;
  colorRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorRefs[1].attachment = 1;
  colorRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // Depth attachment reference
  VkAttachmentReference depthRef{};
  depthRef.attachment = 2;
  depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  // Single subpass
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
  subpass.pColorAttachments = colorRefs.data();
  subpass.pDepthStencilAttachment = &depthRef;

  // Subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies{};

  // Before G-buffer pass
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  // After G-buffer pass (for reading in SSAO pass)
  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  std::array<VkAttachmentDescription, 3> attachments = {
      positionAttachment, normalAttachment, depthAttachment};

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr,
                         &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create G-buffer render pass!");
  }
}

void FrgGBuffer::createFramebuffer() {
  std::array<VkImageView, 3> attachments = {positionImageView, normalImageView,
                                            depthImageView};

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  framebufferInfo.pAttachments = attachments.data();
  framebufferInfo.width = extent.width;
  framebufferInfo.height = extent.height;
  framebufferInfo.layers = 1;

  if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr,
                          &framebuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create G-buffer framebuffer!");
  }
}

VkDescriptorImageInfo FrgGBuffer::getPositionDescriptor() const {
  VkDescriptorImageInfo info{};
  info.sampler = sampler;
  info.imageView = positionImageView;
  info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return info;
}

VkDescriptorImageInfo FrgGBuffer::getNormalDescriptor() const {
  VkDescriptorImageInfo info{};
  info.sampler = sampler;
  info.imageView = normalImageView;
  info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return info;
}

VkDescriptorImageInfo FrgGBuffer::getDepthDescriptor() const {
  VkDescriptorImageInfo info{};
  info.sampler = sampler;
  info.imageView = depthImageView;
  info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  return info;
}

} // namespace frg
