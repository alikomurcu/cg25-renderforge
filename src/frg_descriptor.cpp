#include "frg_descriptor.hpp"

namespace frg {
FrgDescriptor::FrgDescriptor(FrgDevice &device) : frg_device{device} {}
FrgDescriptor::~FrgDescriptor() {
    vkDestroyDescriptorPool(frg_device.device(), descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(frg_device.device(),
                                 descriptor_set_layout,
                                 nullptr);
}

void FrgDescriptor::create_descriptor_set_layout_binding() {
    VkDescriptorSetLayoutBinding sampler_layout_binding{};
    sampler_layout_binding.binding = 0;
    sampler_layout_binding.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding image_array_binding{};
    image_array_binding.binding = 1;
    image_array_binding.descriptorCount = texture_descriptor_size;
    image_array_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    image_array_binding.pImmutableSamplers = nullptr;
    image_array_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
        sampler_layout_binding};

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(frg_device.device(),
                                    &layout_info,
                                    nullptr,
                                    &descriptor_set_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void FrgDescriptor::create_descriptor_pool() {
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = texture_descriptor_size;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = texture_descriptor_size;

    if (vkCreateDescriptorPool(frg_device.device(),
                               &pool_info,
                               nullptr,
                               &descriptor_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void FrgDescriptor::create_descriptor_sets() {
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptor_set_layout;

    if (vkAllocateDescriptorSets(frg_device.device(),
                                 &alloc_info,
                                 &descriptor_set) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor set!");
    }
}

void FrgDescriptor::write_descriptor_sets(
    const std::vector<VkDescriptorImageInfo> &image_infos) {
    VkDescriptorImageInfo sampler_info{};
    sampler_info.sampler = frg_device.textureSampler();

    std::array<VkWriteDescriptorSet, 2> set_writes{};
    set_writes[0] = {};
    set_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_writes[0].dstSet = descriptor_set;
    set_writes[0].dstBinding = 0;
    set_writes[0].dstArrayElement = 0;
    set_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    set_writes[0].descriptorCount = 1;
    set_writes[0].pBufferInfo = 0;
    set_writes[0].pImageInfo = &sampler_info;

    set_writes[1] = {};
    set_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_writes[1].dstSet = descriptor_set;
    set_writes[1].dstBinding = 1;
    set_writes[1].dstArrayElement = 0;
    set_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    set_writes[1].descriptorCount = texture_descriptor_size;
    set_writes[1].pBufferInfo = 0;
    set_writes[1].pImageInfo = image_infos.data();

    vkUpdateDescriptorSets(frg_device.device(),
                           static_cast<uint32_t>(set_writes.size()),
                           set_writes.data(),
                           0,
                           nullptr);
}

} // namespace frg