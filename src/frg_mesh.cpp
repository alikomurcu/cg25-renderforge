#include "frg_mesh.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace frg {
Texture::Texture(FrgDevice &device, const std::string &type, const std::string &path) : device{device} {
    this->type = type;
    this->path = path;

    int tex_width, tex_height, tex_channels;
    stbi_uc *pixels = stbi_load(path.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
    VkDeviceSize image_size = 4 * tex_height * tex_width;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    device.createBuffer(
        image_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer,
        staging_buffer_memory
    );
    void *data;
    vkMapMemory(device.device(), staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(image_size));
    vkUnmapMemory(device.device(), staging_buffer_memory);
    stbi_image_free(pixels);

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = static_cast<uint32_t>(tex_width);
    image_info.extent.height = static_cast<uint32_t>(tex_height);
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;

    device.createImageWithInfo(image_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image, texture_image_memory);
    transition_image_layout(
        texture_image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    device.copyBufferToImage(
        staging_buffer,
        texture_image,
        static_cast<uint32_t>(tex_width),
        static_cast<uint32_t>(tex_height),
        1
    );
    transition_image_layout(
        texture_image,
        VK_FORMAT_R8G8B8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    vkDestroyBuffer(device.device(), staging_buffer, nullptr);
    vkFreeMemory(device.device(), staging_buffer_memory, nullptr);

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = texture_image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device.device(), &view_info, nullptr, &texture_image_view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    texture_idx = LoadedTextures::assign_texture_idx(path);
    create_descriptor_image_info();
}

void Texture::create_descriptor_image_info() {
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_image_info.imageView = texture_image_view;
    descriptor_image_info.sampler = device.textureSampler();
}

void Texture::transition_image_layout(
    VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout
) {

    VkCommandBuffer command_buffer = device.beginSingleTimeCommands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (
        old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    )
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(command_buffer, source_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    device.endSingleTimeCommands(command_buffer);
}

Texture::~Texture() {
    vkDestroyImageView(device.device(), texture_image_view, nullptr);
    vkDestroyImage(device.device(), texture_image, nullptr);
    vkFreeMemory(device.device(), texture_image_memory, nullptr);
}

uint32_t LoadedTextures::assign_texture_idx(const std::string &path) {
    if (loaded_texture_names.find(path) != loaded_texture_names.end()) {
        return loaded_texture_names[path];
    }
    loaded_texture_names.insert({path, texture_cntr++});
    return loaded_texture_names[path];
}

std::vector<VkVertexInputBindingDescription> Vertex::get_binding_descriptions() {
    std::vector<VkVertexInputBindingDescription> binding_descriptions(1);
    binding_descriptions[0].binding = 0;
    binding_descriptions[0].stride = sizeof(Vertex);
    binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding_descriptions;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::get_attribute_descriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};

    // Position attribute
    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(Vertex, position);

    // Normal attribute
    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(Vertex, normal);

    // Texture coord attribute
    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(Vertex, tex_coord);

    return attribute_descriptions;
}

FrgMesh::FrgMesh(
    FrgDevice &device, const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices,
    std::vector<std::unique_ptr<Texture>> textures
)
    : vertices{vertices}, indices{indices}, frg_device{device}, textures(std::move(textures)) {
    setup_mesh();
}

FrgMesh::~FrgMesh() {
    vkDestroyBuffer(frg_device.device(), vertex_buffer, nullptr);
    vkFreeMemory(frg_device.device(), vertex_buffer_memory, nullptr);

    if (indices.empty())
        return;

    vkDestroyBuffer(frg_device.device(), index_buffer, nullptr);
    vkFreeMemory(frg_device.device(), index_buffer_memory, nullptr);
}

void FrgMesh::draw(VkCommandBuffer command_buffer) {
    if (indices.empty()) {
        vkCmdDraw(command_buffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
    } else {
        vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }
}
void FrgMesh::bind(VkCommandBuffer command_buffer) {
    VkBuffer buffers[] = {vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, buffers, offsets);
    if (!indices.empty())
        vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
}

bool FrgMesh::hasNormalTexture() {
    if (textures.empty())
        return false;

    for (const auto &texture : textures) {
        if (std::strcmp(texture->type.c_str(), "texture_normal") == 0) {
            return true;
        }
    }

    return false;
}

void FrgMesh::create_texture_image(const std::string &path_to_file, const std::string &type) {
    Texture texture{frg_device, type, path_to_file};
    // textures.emplace_back(texture);
}

void FrgMesh::setup_mesh() {
    create_vertex_buffer(vertex_buffer, vertex_buffer_memory);
    if (!indices.empty())
        create_index_buffer(index_buffer, index_buffer_memory);
}

void FrgMesh::create_vertex_buffer(VkBuffer &buffer, VkDeviceMemory &buffer_memory) {
    uint32_t vertex_count = static_cast<uint32_t>(vertices.size());
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertex_count;
    frg_device.createBuffer(
        buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer,
        buffer_memory
    );

    void *data;
    vkMapMemory(frg_device.device(), vertex_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(frg_device.device(), vertex_buffer_memory);
}

void FrgMesh::create_index_buffer(VkBuffer &buffer, VkDeviceMemory &buffer_memory) {
    VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    frg_device.createBuffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        staging_buffer,
        staging_memory
    );
    void *data;
    vkMapMemory(frg_device.device(), staging_memory, 0, buffer_size, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(frg_device.device(), staging_memory);

    frg_device.createBuffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        index_buffer,
        index_buffer_memory
    );

    frg_device.copyBuffer(staging_buffer, index_buffer, buffer_size);

    vkDestroyBuffer(frg_device.device(), staging_buffer, nullptr);
    vkFreeMemory(frg_device.device(), staging_memory, nullptr);
}

} // namespace frg