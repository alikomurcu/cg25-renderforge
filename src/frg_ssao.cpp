#include "frg_ssao.hpp"

#include <array>
#include <cstring>
#include <random>
#include <stdexcept>

namespace frg {

// Lerp helper function
static float lerp(float a, float b, float t) { return a + t * (b - a); }

FrgSSAO::FrgSSAO(FrgDevice &device, VkExtent2D extent)
    : device{device}, extent{extent} {
  generateKernel();
  createKernelBuffer();
  createNoiseTexture();
  createSSAOImage();
  createBlurImage();
  createSamplers();
  createSSAORenderPass();
  createBlurRenderPass();
  createFramebuffers();
}

FrgSSAO::~FrgSSAO() { cleanup(); }

void FrgSSAO::resize(VkExtent2D newExtent) {
  vkDeviceWaitIdle(device.device());

  // Only need to recreate size-dependent resources
  VkDevice dev = device.device();

  if (blurFramebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(dev, blurFramebuffer, nullptr);
  }
  if (ssaoFramebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(dev, ssaoFramebuffer, nullptr);
  }

  if (blurredImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(dev, blurredImageView, nullptr);
  }
  if (blurredImage != VK_NULL_HANDLE) {
    vkDestroyImage(dev, blurredImage, nullptr);
  }
  if (blurredMemory != VK_NULL_HANDLE) {
    vkFreeMemory(dev, blurredMemory, nullptr);
  }

  if (ssaoImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(dev, ssaoImageView, nullptr);
  }
  if (ssaoImage != VK_NULL_HANDLE) {
    vkDestroyImage(dev, ssaoImage, nullptr);
  }
  if (ssaoMemory != VK_NULL_HANDLE) {
    vkFreeMemory(dev, ssaoMemory, nullptr);
  }

  extent = newExtent;
  createSSAOImage();
  createBlurImage();
  createFramebuffers();
}

void FrgSSAO::cleanup() {
  VkDevice dev = device.device();

  if (blurFramebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(dev, blurFramebuffer, nullptr);
  }
  if (ssaoFramebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(dev, ssaoFramebuffer, nullptr);
  }
  if (blurRenderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(dev, blurRenderPass, nullptr);
  }
  if (ssaoRenderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(dev, ssaoRenderPass, nullptr);
  }
  if (sampler != VK_NULL_HANDLE) {
    vkDestroySampler(dev, sampler, nullptr);
  }
  if (noiseSampler != VK_NULL_HANDLE) {
    vkDestroySampler(dev, noiseSampler, nullptr);
  }

  // Blurred image
  if (blurredImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(dev, blurredImageView, nullptr);
  }
  if (blurredImage != VK_NULL_HANDLE) {
    vkDestroyImage(dev, blurredImage, nullptr);
  }
  if (blurredMemory != VK_NULL_HANDLE) {
    vkFreeMemory(dev, blurredMemory, nullptr);
  }

  // SSAO image
  if (ssaoImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(dev, ssaoImageView, nullptr);
  }
  if (ssaoImage != VK_NULL_HANDLE) {
    vkDestroyImage(dev, ssaoImage, nullptr);
  }
  if (ssaoMemory != VK_NULL_HANDLE) {
    vkFreeMemory(dev, ssaoMemory, nullptr);
  }

  // Noise
  if (noiseImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(dev, noiseImageView, nullptr);
  }
  if (noiseImage != VK_NULL_HANDLE) {
    vkDestroyImage(dev, noiseImage, nullptr);
  }
  if (noiseMemory != VK_NULL_HANDLE) {
    vkFreeMemory(dev, noiseMemory, nullptr);
  }

  // Kernel buffer
  if (kernelBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(dev, kernelBuffer, nullptr);
  }
  if (kernelMemory != VK_NULL_HANDLE) {
    vkFreeMemory(dev, kernelMemory, nullptr);
  }
}

void FrgSSAO::generateKernel() {
  // Generate SSAO sample kernel
  // Samples are in tangent space, oriented along +z (normal direction)

  std::default_random_engine generator;
  std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);

  kernel.resize(KERNEL_SIZE);

  for (int i = 0; i < KERNEL_SIZE; ++i) {
    // Random point in hemisphere (tangent space, +z is up)
    glm::vec3 sample(randomFloats(generator) * 2.0f - 1.0f, // x: [-1, 1]
                     randomFloats(generator) * 2.0f - 1.0f, // y: [-1, 1]
                     randomFloats(generator) // z: [0, 1] (hemisphere)
    );
    sample = glm::normalize(sample);
    sample *= randomFloats(generator); // Random length [0, 1]

    // Scale samples to be more concentrated near the origin
    // This gives better quality with fewer samples
    float scale = static_cast<float>(i) / static_cast<float>(KERNEL_SIZE);
    scale = lerp(0.1f, 1.0f, scale * scale); // Accelerating interpolation
    sample *= scale;

    // Store as vec4 for std140 alignment
    kernel[i] = glm::vec4(sample, 0.0f);
  }
}

void FrgSSAO::createKernelBuffer() {
  VkDeviceSize bufferSize = sizeof(glm::vec4) * KERNEL_SIZE;

  // Create staging buffer
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;
  device.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      stagingBuffer, stagingMemory);

  // Copy kernel data to staging buffer
  void *data;
  vkMapMemory(device.device(), stagingMemory, 0, bufferSize, 0, &data);
  memcpy(data, kernel.data(), bufferSize);
  vkUnmapMemory(device.device(), stagingMemory);

  // Create device-local buffer
  device.createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, kernelBuffer, kernelMemory);

  // Copy from staging to device
  device.copyBuffer(stagingBuffer, kernelBuffer, bufferSize);

  // Cleanup staging
  vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
  vkFreeMemory(device.device(), stagingMemory, nullptr);
}

void FrgSSAO::createNoiseTexture() {
  // Create 4x4 noise texture with random rotation vectors
  // These vectors rotate the sample kernel around the normal

  std::default_random_engine generator;
  std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);

  std::vector<glm::vec4> noiseData(NOISE_SIZE * NOISE_SIZE);
  for (int i = 0; i < NOISE_SIZE * NOISE_SIZE; ++i) {
    // Random rotation around z-axis (the normal in tangent space)
    glm::vec3 noise(randomFloats(generator) * 2.0f - 1.0f,
                    randomFloats(generator) * 2.0f - 1.0f,
                    0.0f // z = 0 means rotation around normal
    );
    noiseData[i] = glm::vec4(noise, 0.0f);
  }

  VkDeviceSize imageSize = NOISE_SIZE * NOISE_SIZE * sizeof(glm::vec4);

  // Create staging buffer
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;
  device.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      stagingBuffer, stagingMemory);

  void *data;
  vkMapMemory(device.device(), stagingMemory, 0, imageSize, 0, &data);
  memcpy(data, noiseData.data(), imageSize);
  vkUnmapMemory(device.device(), stagingMemory);

  // Create noise image
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = NOISE_SIZE;
  imageInfo.extent.height = NOISE_SIZE;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             noiseImage, noiseMemory);

  // Transition image layout and copy data
  VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

  // Transition to transfer dst
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = noiseImage;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  // Copy buffer to image
  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {NOISE_SIZE, NOISE_SIZE, 1};

  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, noiseImage,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  // Transition to shader read
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  device.endSingleTimeCommands(commandBuffer);

  // Cleanup staging
  vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
  vkFreeMemory(device.device(), stagingMemory, nullptr);

  // Create image view
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = noiseImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device.device(), &viewInfo, nullptr, &noiseImageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create noise image view!");
  }
}

void FrgSSAO::createSSAOImage() {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = extent.width;
  imageInfo.extent.height = extent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = SSAO_FORMAT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             ssaoImage, ssaoMemory);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = ssaoImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = SSAO_FORMAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device.device(), &viewInfo, nullptr, &ssaoImageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create SSAO image view!");
  }
}

void FrgSSAO::createBlurImage() {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = extent.width;
  imageInfo.extent.height = extent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = SSAO_FORMAT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             blurredImage, blurredMemory);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = blurredImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = SSAO_FORMAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device.device(), &viewInfo, nullptr,
                        &blurredImageView) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create blurred SSAO image view!");
  }
}

void FrgSSAO::createSamplers() {
  // Sampler for SSAO textures (linear filtering for blur)
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create SSAO sampler!");
  }

  // Sampler for noise texture (repeat for tiling)
  VkSamplerCreateInfo noiseSamplerInfo{};
  noiseSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  noiseSamplerInfo.magFilter = VK_FILTER_NEAREST;
  noiseSamplerInfo.minFilter = VK_FILTER_NEAREST;
  noiseSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  noiseSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  noiseSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  noiseSamplerInfo.anisotropyEnable = VK_FALSE;
  noiseSamplerInfo.maxAnisotropy = 1.0f;
  noiseSamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  noiseSamplerInfo.unnormalizedCoordinates = VK_FALSE;
  noiseSamplerInfo.compareEnable = VK_FALSE;
  noiseSamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  noiseSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  noiseSamplerInfo.mipLodBias = 0.0f;
  noiseSamplerInfo.minLod = 0.0f;
  noiseSamplerInfo.maxLod = 0.0f;

  if (vkCreateSampler(device.device(), &noiseSamplerInfo, nullptr,
                      &noiseSampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create noise sampler!");
  }
}

void FrgSSAO::createSSAORenderPass() {
  VkAttachmentDescription attachment{};
  attachment.format = SSAO_FORMAT;
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference colorRef{};
  colorRef.attachment = 0;
  colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;

  std::array<VkSubpassDependency, 2> dependencies{};

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &attachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr,
                         &ssaoRenderPass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create SSAO render pass!");
  }
}

void FrgSSAO::createBlurRenderPass() {
  VkAttachmentDescription attachment{};
  attachment.format = SSAO_FORMAT;
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference colorRef{};
  colorRef.attachment = 0;
  colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;

  std::array<VkSubpassDependency, 2> dependencies{};

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &attachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr,
                         &blurRenderPass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create blur render pass!");
  }
}

void FrgSSAO::createFramebuffers() {
  // SSAO framebuffer
  {
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = ssaoRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &ssaoImageView;
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr,
                            &ssaoFramebuffer) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create SSAO framebuffer!");
    }
  }

  // Blur framebuffer
  {
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = blurRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &blurredImageView;
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr,
                            &blurFramebuffer) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create blur framebuffer!");
    }
  }
}

VkDescriptorImageInfo FrgSSAO::getSSAODescriptor() const {
  VkDescriptorImageInfo info{};
  info.sampler = sampler;
  info.imageView = ssaoImageView;
  info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return info;
}

VkDescriptorImageInfo FrgSSAO::getBlurredDescriptor() const {
  VkDescriptorImageInfo info{};
  info.sampler = sampler;
  info.imageView = blurredImageView;
  info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return info;
}

VkDescriptorImageInfo FrgSSAO::getNoiseDescriptor() const {
  VkDescriptorImageInfo info{};
  info.sampler = noiseSampler;
  info.imageView = noiseImageView;
  info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return info;
}

VkDescriptorBufferInfo FrgSSAO::getKernelDescriptor() const {
  VkDescriptorBufferInfo info{};
  info.buffer = kernelBuffer;
  info.offset = 0;
  info.range = sizeof(glm::vec4) * KERNEL_SIZE;
  return info;
}

} // namespace frg
