#pragma once

#include "frg_device.hpp"
#include "frg_particle_dispenser.hpp"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace frg {

class FrgSwapChain {
  public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    FrgSwapChain(FrgDevice &deviceRef, VkExtent2D windowExtent);
    FrgSwapChain(FrgDevice &deviceRef, VkExtent2D windowExtent, std::shared_ptr<FrgSwapChain> previous);
    ~FrgSwapChain();

    FrgSwapChain(const FrgSwapChain &) = delete;
    FrgSwapChain &operator=(const FrgSwapChain &) = delete;

    VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
    VkRenderPass getRenderPass() { return renderPass; }
    VkImageView getImageView(int index) { return swapChainImageViews[index]; }
    size_t imageCount() { return swapChainImages.size(); }
    VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return swapChainExtent; }
    uint32_t width() { return swapChainExtent.width; }
    uint32_t height() { return swapChainExtent.height; }

    float extentAspectRatio() {
        return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
    }
    VkFormat findDepthFormat();

    VkResult acquireNextImage(uint32_t *imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex, bool has_compute = false);
    void submitComputeCommandBuffer(
        std::vector<VkCommandBuffer> &buffers, std::vector<void *> &ubos_mapped,
        std::function<void(VkCommandBuffer, VkPipelineLayout, VkPipeline, size_t, size_t)> renderFnc,
        VkPipelineLayout layout, VkPipeline pipeline, size_t particle_count, UniformBufferObject ubo
    );
    bool compareSwapFormats(const FrgSwapChain &otherSwapChain) const {
        return otherSwapChain.swapChainDepthFormat == swapChainDepthFormat &&
               otherSwapChain.swapChainImageFormat == swapChainImageFormat;
    }

    void updateUniformBuffer(std::vector<void *> &ubos, const UniformBufferObject &obj);
    void bindAndDrawCompute(VkCommandBuffer comm_buff, std::vector<VkBuffer> &ssbos, uint32_t point_count);

  private:
    void init();
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();

    // Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    VkFormat swapChainImageFormat;
    VkFormat swapChainDepthFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemorys;
    std::vector<VkImageView> depthImageViews;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    FrgDevice &device;
    VkExtent2D windowExtent;

    VkSwapchainKHR swapChain;
    std::shared_ptr<FrgSwapChain> oldSwapChain;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkSemaphore> computeFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> inComputeFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
};

} // namespace frg
