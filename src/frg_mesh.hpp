#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "frg_device.hpp"

namespace frg {
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coord;

    static std::vector<VkVertexInputBindingDescription>
        get_binding_descriptions();
    static std::array<VkVertexInputAttributeDescription, 3>
        get_attribute_descriptions();
};

class LoadedTextures {
  public:
    static uint32_t assign_texture_idx(const std::string &path);

  private:
    inline static uint32_t texture_cntr = 0;
    inline static std::map<std::string, uint32_t> loaded_texture_names{};
};

class Texture {
  public:
    Texture(FrgDevice &device, const std::string &type,
            const std::string &path);
    ~Texture();
    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;
    void transition_image_layout(VkImage image, VkFormat format,
                                 VkImageLayout old_layout,
                                 VkImageLayout new_layout);
    std::string type;
    std::string path;

    VkDescriptorImageInfo descriptor_image_info;
    uint32_t textureIdx() { return texture_idx; }

  private:
    void create_descriptor_image_info();

    uint32_t texture_idx;

    FrgDevice &device;
    VkImage texture_image;
    VkDeviceMemory texture_image_memory;
    VkImageView texture_image_view;
};
class FrgMesh {
  public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<std::unique_ptr<Texture>> textures;
    FrgMesh(FrgDevice &device, const std::vector<Vertex> &vertices,
            const std::vector<unsigned int> &indices,
            std::vector<std::unique_ptr<Texture>> textures);
    ~FrgMesh();

    void draw(VkCommandBuffer command_buffer);
    void bind(VkCommandBuffer command_buffer);
    std::optional<uint32_t> getTextureIndex() {
        if (textures.empty())
            return std::optional<uint32_t>();
        return textures[0]->textureIdx();
    }

    FrgMesh(const FrgMesh &) = delete;
    FrgMesh &operator=(const FrgMesh &) = delete;

    void create_texture_image(const std::string &path_to_file,
                              const std::string &type);

  private:
    FrgDevice &frg_device;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;

    void setup_mesh();
    void create_vertex_buffer(VkBuffer &buffer, VkDeviceMemory &buffer_memory);
    void create_index_buffer(VkBuffer &buffer, VkDeviceMemory &buffer_memory);
};
} // namespace frg
