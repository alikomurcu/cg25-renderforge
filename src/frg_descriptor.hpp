#pragma once
#include "frg_device.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <stdexcept>

namespace frg {
class FrgDescriptor {
  public:
    FrgDescriptor(FrgDevice &device);
    ~FrgDescriptor();
    FrgDescriptor(const FrgDescriptor &) = delete;
    FrgDescriptor &operator=(const FrgDescriptor &) = delete;

    const VkDescriptorSetLayout *descriptorSetLayout() {
        return &descriptor_set_layout;
    }
    const VkDescriptorSet *descriptorSet() { return &descriptor_set; }
    uint32_t descriptorSetCount() { return 1; }

    void write_descriptor_sets(
        const std::vector<VkDescriptorImageInfo> &image_infos);

  private:
    void create_descriptor_set_layout_binding();
    void create_descriptor_pool();
    void create_descriptor_sets();
    const uint32_t DEFAULT_POOL_SIZE_INCR = 256;
    uint32_t texture_descriptor_size = DEFAULT_POOL_SIZE_INCR;
    uint32_t texture_count = 0;
    FrgDevice &frg_device;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
};
} // namespace frg