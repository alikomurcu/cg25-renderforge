#pragma once

#include "frg_device.hpp"
#include "frg_mesh.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

// std
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace frg {
class FrgModel {
  public:
    FrgModel(FrgDevice &device, const std::string &path);
    void draw(VkCommandBuffer command_buffer);
    void drawMesh(VkCommandBuffer command_buffer, size_t mesh_idx);
    uint32_t vertex_count() {
        uint32_t v_count = 0;
        for (const auto &mesh : meshes) {
            v_count += mesh->vertices.size();
        }

        return v_count;
    }

    void add_texture_to_mesh(size_t idx, std::unique_ptr<Texture> &texture);
    std::vector<VkDescriptorImageInfo> get_descriptors();
    void set_texture_index_for_mesh(size_t mesh_idx, uint32_t index);
    std::vector<uint32_t> get_mesh_texture_indices() const;

  private:
    std::vector<std::unique_ptr<FrgMesh>> meshes;
    std::string dir;
    FrgDevice &frg_device;

    void load_model(const std::string &path);
    void process_node(aiNode *node, const aiScene *scene);
    std::unique_ptr<FrgMesh> process_mesh(aiMesh *mesh, const aiScene *scene);
    std::vector<std::unique_ptr<Texture>>
        load_material_textures(aiMaterial *mat, aiTextureType type,
                               std::string type_name);
};
} // namespace frg