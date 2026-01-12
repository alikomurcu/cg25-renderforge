#include "frg_descriptor.hpp"

namespace frg {
FrgDescriptor::FrgDescriptor(FrgDevice &device) : frg_device{device} {
    create_descriptor_set_layout_binding();
    create_comp_descriptor_set_layout_binding();
    create_descriptor_pool();
    create_descriptor_sets();
}
FrgDescriptor::~FrgDescriptor() {
    vkDestroyDescriptorPool(frg_device.device(), descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(frg_device.device(), descriptor_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(frg_device.device(), comp_desc_set_layout, nullptr);
}

void FrgDescriptor::create_descriptor_set_layout_binding() {
    VkDescriptorSetLayoutBinding sampler_layout_binding{};
    sampler_layout_binding.binding = 0;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding image_array_binding{};
    image_array_binding.binding = 1;
    image_array_binding.descriptorCount = texture_descriptor_size;
    image_array_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    image_array_binding.pImmutableSamplers = nullptr;
    image_array_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {sampler_layout_binding, image_array_binding};

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    VkDescriptorBindingFlags binding_flags[] = {0, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};
    VkDescriptorSetLayoutBindingFlagsCreateInfo layout_flags_info{};
    layout_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    layout_flags_info.bindingCount = 2;
    layout_flags_info.pBindingFlags = binding_flags;
    layout_flags_info.pNext = nullptr;
    layout_info.pNext = reinterpret_cast<void *>(&layout_flags_info);

    if (vkCreateDescriptorSetLayout(frg_device.device(), &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void FrgDescriptor::create_comp_descriptor_set_layout_binding() {
    std::array<VkDescriptorSetLayoutBinding, 3> layout_bindings{};
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].pImmutableSamplers = nullptr;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorCount = 1;
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layout_bindings[1].pImmutableSamplers = nullptr;
    layout_bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layout_bindings[2].binding = 2;
    layout_bindings[2].descriptorCount = 1;
    layout_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[2].pImmutableSamplers = nullptr;
    layout_bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
    layout_info.pBindings = layout_bindings.data();

    if (vkCreateDescriptorSetLayout(frg_device.device(), &layout_info, nullptr, &comp_desc_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute descriptor set layout!");
    }
}

void FrgDescriptor::create_descriptor_pool() {
    std::array<VkDescriptorPoolSize, 4> pool_sizes;
    pool_sizes[0] = {};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    pool_sizes[0].descriptorCount = 1;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    pool_sizes[1].descriptorCount = texture_descriptor_size;
    pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[2].descriptorCount = static_cast<uint32_t>(FrgSwapChain::MAX_FRAMES_IN_FLIGHT);
    pool_sizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_sizes[3].descriptorCount = static_cast<uint32_t>(FrgSwapChain::MAX_FRAMES_IN_FLIGHT) * 2;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = texture_descriptor_size + static_cast<uint32_t>(FrgSwapChain::MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(frg_device.device(), &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void FrgDescriptor::create_descriptor_sets() {
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptor_set_layout;

    if (vkAllocateDescriptorSets(frg_device.device(), &alloc_info, &descriptor_set) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }
}

void FrgDescriptor::write_descriptor_sets(const std::vector<VkDescriptorImageInfo> &image_infos) {
    VkDescriptorImageInfo sampler_info{};
    sampler_info.sampler = frg_device.textureSampler();

    std::array<VkWriteDescriptorSet, 2> set_writes{};
    set_writes[0] = {};
    set_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_writes[0].dstSet = descriptor_set;
    set_writes[0].dstBinding = 0;
    set_writes[0].dstArrayElement = 0;
    set_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    set_writes[0].descriptorCount = 1;
    set_writes[0].pBufferInfo = 0;
    set_writes[0].pImageInfo = &sampler_info;

    set_writes[1] = {};
    set_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_writes[1].dstSet = descriptor_set;
    set_writes[1].dstBinding = 1;
    set_writes[1].dstArrayElement = 0;
    set_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    set_writes[1].descriptorCount = static_cast<uint32_t>(image_infos.size());
    set_writes[1].pBufferInfo = 0;
    set_writes[1].pImageInfo = image_infos.data();

    vkUpdateDescriptorSets(
        frg_device.device(),
        static_cast<uint32_t>(set_writes.size()),
        set_writes.data(),
        0,
        nullptr
    );
}

void FrgDescriptor::write_comp_descriptor_sets(
    std::vector<VkBuffer> &uni_buffers, size_t ubo_size, std::vector<VkBuffer> &shader_storage_buffers, size_t ssbo_size
) {
    uint32_t layout_count = FrgSwapChain::MAX_FRAMES_IN_FLIGHT;
    assert(
        layout_count == uni_buffers.size() &&
        "The program requires the same amount of descriptor sets as uniform buffers!"
    );
    std::vector<VkDescriptorSetLayout> layouts(layout_count, comp_desc_set_layout);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = layout_count;
    alloc_info.pSetLayouts = layouts.data();

    comp_descriptor_set.resize(layout_count);
    if (vkAllocateDescriptorSets(frg_device.device(), &alloc_info, comp_descriptor_set.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate compute descriptor sets!");
    }

    // TODO: implement ubo
    for (uint32_t i = 0; i < layout_count; ++i) {
        VkDescriptorBufferInfo uniform_buffer_info{};
        uniform_buffer_info.buffer = uni_buffers[i];
        uniform_buffer_info.offset = 0;
        uniform_buffer_info.range = ubo_size;

        std::array<VkWriteDescriptorSet, 3> descriptor_writes{};
        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = comp_descriptor_set[i];
        descriptor_writes[0].dstBinding = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo = &uniform_buffer_info;

        VkDescriptorBufferInfo storage_buffer_info_last_frame{};
        storage_buffer_info_last_frame.buffer = shader_storage_buffers[(i + layout_count - 1) % layout_count];
        storage_buffer_info_last_frame.offset = 0;
        storage_buffer_info_last_frame.range = ssbo_size;

        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = comp_descriptor_set[i];
        descriptor_writes[1].dstBinding = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pBufferInfo = &storage_buffer_info_last_frame;

        VkDescriptorBufferInfo storage_buffer_info_current_frame{};
        storage_buffer_info_current_frame.buffer = shader_storage_buffers[i];
        storage_buffer_info_current_frame.offset = 0;
        storage_buffer_info_current_frame.range = ssbo_size;

        descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[2].dstSet = comp_descriptor_set[i];
        descriptor_writes[2].dstBinding = 2;
        descriptor_writes[2].dstArrayElement = 0;
        descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptor_writes[2].descriptorCount = 1;
        descriptor_writes[2].pBufferInfo = &storage_buffer_info_current_frame;

        vkUpdateDescriptorSets(frg_device.device(), descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
    }
}

} // namespace frg