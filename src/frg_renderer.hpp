#pragma once

#include "frg_descriptor.hpp"
#include "frg_device.hpp"
#include "frg_model.hpp"
#include "frg_pipeline.hpp"
#include "frg_swap_chain.hpp"
#include "frg_window.hpp"

// std
#include <cassert>
#include <memory>
#include <vector>

namespace frg {
class FrgRenderer {
public:
  FrgRenderer(FrgWindow &window, FrgDevice &device);
  ~FrgRenderer();

  FrgRenderer(const FrgRenderer &) = delete;
  FrgRenderer &operator=(const FrgRenderer &) = delete;

  VkRenderPass getSwapChainRenderPass() const { return frgSwapChain->getRenderPass(); }
  VkExtent2D getSwapChainExtent() const { return frgSwapChain->getSwapChainExtent(); }
  float getAspectRatio() const { return frgSwapChain->extentAspectRatio(); }

  bool isFrameInProgress() const { return isFrameStarted; }

  VkCommandBuffer getCurrentCommandBuffer() const {
        assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
    return commandBuffers[currentFrameIndex];
  }

  uint32_t getCurrentFrameIndex() const {
        assert(isFrameStarted && "Cannot get frame index when frame not in progress");
    return currentFrameIndex;
  }

  VkCommandBuffer beginFrame();
  void endFrame(bool compute = false);
  void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
  void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void renderComputePipeline(
        std::vector<VkCommandBuffer> &buffers, FrgDescriptor &desc, VkPipelineLayout pipe_layout, VkPipeline pipeline,
        size_t particle_count, UniformBufferObject &ubo, std::vector<void *> &ubos_mapped
    );

  // Delegate to swapchain
    void delegateComputeBindAndDraw(VkCommandBuffer comm_buff, std::vector<VkBuffer> &ssbos, uint32_t point_count);

private:
  void createCommandBuffers();
  void freeCommandBuffers();
  void recreateSwapChain();

  FrgWindow &frgWindow;
  FrgDevice &frgDevice;
  std::unique_ptr<FrgSwapChain> frgSwapChain;
  std::vector<VkCommandBuffer> commandBuffers;

  uint32_t currentImageIndex;
  int currentFrameIndex{0};
  bool isFrameStarted{false};
};
} // namespace frg