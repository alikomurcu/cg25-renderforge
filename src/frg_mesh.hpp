#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <glm/glm.hpp>
#include <memory>
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

class Texture {
  public:
    Texture(FrgDevice &device, const std::string &type,
            const std::string &path);
    ~Texture();
    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;
    const VkImage &get_texture() const;
    const VkDeviceMemory &get_texture_memory() const;
    void transition_image_layout(VkImage image, VkFormat format,
                                 VkImageLayout old_layout,
                                 VkImageLayout new_layout);
    std::string type;
    std::string path;

  private:
    void create_descriptor_image_info();
    FrgDevice &device;
    VkImage texture_image;
    VkDeviceMemory texture_image_memory;
    VkImageView texture_image_view;
    VkDescriptorImageInfo descriptor_image_info;
};
class FrgMesh {
  public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::unique_ptr<Texture>> textures;
    FrgMesh(FrgDevice &device, const std::vector<Vertex> &vertices,
            const std::vector<unsigned int> &indices,
            std::vector<std::unique_ptr<Texture>> textures);
    ~FrgMesh();

    void draw(VkCommandBuffer command_buffer);
    void bind(VkCommandBuffer command_buffer);

    FrgMesh(const FrgMesh &) = delete;
    FrgMesh &operator=(const FrgMesh &) = delete;

    void create_texture_image(const std::string &path_to_file,
                              const std::string &type);

  private:
    FrgDevice &frg_device;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    void setup_mesh();
};
} // namespace frg
