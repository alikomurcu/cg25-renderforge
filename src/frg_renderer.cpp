#include "frg_renderer.hpp"

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace frg {
FrgRenderer::FrgRenderer(FrgWindow &window, FrgDevice &device) : frgWindow{window}, frgDevice{device} {
    recreateSwapChain();
    createCommandBuffers();
}

FrgRenderer::~FrgRenderer() { freeCommandBuffers(); }

void FrgRenderer::recreateSwapChain() {
    auto extent = frgWindow.getExtent();
    while (extent.width == 0 || extent.height == 0) {
        extent = frgWindow.getExtent();
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(frgDevice.device());

    if (frgSwapChain == nullptr) {
        frgSwapChain = std::make_unique<FrgSwapChain>(frgDevice, extent);
    } else {
        std::shared_ptr<FrgSwapChain> oldSwapChain = std::move(frgSwapChain);
        frgSwapChain = std::make_unique<FrgSwapChain>(frgDevice, extent, oldSwapChain);

        if (!oldSwapChain->compareSwapFormats(*frgSwapChain.get())) {
            throw std::runtime_error("Swap chain image or depth format has changed!");
        }
    }
    createCommandBuffers();
}

void FrgRenderer::createCommandBuffers() {
    commandBuffers.resize(FrgSwapChain::MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = frgDevice.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(frgDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void FrgRenderer::freeCommandBuffers() {
    vkFreeCommandBuffers(
        frgDevice.device(),
        frgDevice.getCommandPool(),
        static_cast<uint32_t>(commandBuffers.size()),
        commandBuffers.data()
    );
    commandBuffers.clear();
}

VkCommandBuffer FrgRenderer::beginFrame() {
    assert(!isFrameStarted && "Cannot call beginFrame while already in progress");
    auto result = frgSwapChain->acquireNextImage(&currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    isFrameStarted = true;
    VkCommandBuffer commandBuffer = getCurrentCommandBuffer();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    return commandBuffer;
}

void FrgRenderer::endFrame(bool compute) {
    assert(isFrameStarted && "Cannot call endFrame while frame not in progress");
    VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to end recording command buffer!");
    }

    auto result = frgSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex, compute);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frgWindow.wasWindowResized()) {
        frgWindow.resetWindowResizedFlag();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to submit command buffers!");
    }
    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % FrgSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void FrgRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "Cannot call beginSwapChainRenderPass if frame not in progress");
    assert(
        commandBuffer == getCurrentCommandBuffer() &&
        "Cannot begin render pass on command buffer from a different frame"
    );

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = frgSwapChain->getRenderPass();
    renderPassInfo.framebuffer = frgSwapChain->getFrameBuffer(currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = frgSwapChain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(frgSwapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(frgSwapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{
        {0, 0},
        frgSwapChain->getSwapChainExtent()
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void FrgRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "Cannot call endSwapChainRenderPass if frame not in progress");
    assert(
        commandBuffer == getCurrentCommandBuffer() && "Cannot end render pass on command buffer from a different frame"
    );

    vkCmdEndRenderPass(commandBuffer);
}
void FrgRenderer::renderComputePipeline(
    std::vector<VkCommandBuffer> &buffers, FrgDescriptor &desc, VkPipelineLayout pipe_layout, VkPipeline pipeline,
    size_t particle_count, float dt, std::vector<void *> &ubos_mapped
) {
    UniformBufferObject ubo{};
    ubo.deltaTime = dt;
    auto fncPtr = std::bind(
        &FrgDescriptor::recordComputeCommandBuffer,
        &desc,
        std::placeholders::_1,
        pipe_layout,
        pipeline,
        particle_count,
        std::placeholders::_5
    );
    frgSwapChain->submitComputeCommandBuffer(buffers, ubos_mapped, fncPtr, pipe_layout, pipeline, particle_count, ubo);
}
void FrgRenderer::delegateComputeBindAndDraw(
    VkCommandBuffer comm_buff, std::vector<VkBuffer> &ssbos, uint32_t point_count
) {
    frgSwapChain->bindAndDrawCompute(comm_buff, ssbos, point_count);
}
} // namespace frg
