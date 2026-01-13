#pragma once
#include "frg_device.hpp"
#include "frg_swap_chain.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <cassert>
#include <stdexcept>

namespace frg {
class FrgDescriptor {
  public:
    FrgDescriptor(FrgDevice &device);
    ~FrgDescriptor();
    FrgDescriptor(const FrgDescriptor &) = delete;
    FrgDescriptor &operator=(const FrgDescriptor &) = delete;

    const VkDescriptorSetLayout *descriptorSetLayout() { return &descriptor_set_layout; }
    const VkDescriptorSet *descriptorSet() { return &descriptor_set; }
    uint32_t descriptorSetCount() { return 1; }

    void write_descriptor_sets(const std::vector<VkDescriptorImageInfo> &image_infos);
    void write_comp_descriptor_sets(
        std::vector<VkBuffer> &uni_buffers, size_t ubo_size, std::vector<VkBuffer> &shader_storage_buffers,
        size_t ssbo_size
    );

    void recordComputeCommandBuffer(
        VkCommandBuffer command_buf, VkPipelineLayout pipeline_layout, VkPipeline compute_pipeline, size_t dispatch,
        size_t desc_idx
    );

  private:
    void create_descriptor_set_layout_binding();
    void create_comp_descriptor_set_layout_binding();
    void create_descriptor_pool();
    void create_descriptor_sets();

    const uint32_t DEFAULT_POOL_SIZE_INCR = 256;
    uint32_t texture_descriptor_size = DEFAULT_POOL_SIZE_INCR;
    uint32_t texture_count = 0;
    FrgDevice &frg_device;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSetLayout comp_desc_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    std::vector<VkDescriptorSet> comp_descriptor_set;
};
} // namespace frg