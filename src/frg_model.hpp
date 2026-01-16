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
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace frg {
struct SimplePushConstantData {
  glm::mat4 transform{1.f};
  glm::mat4 modelMatrix{1.f};
  glm::mat4 normalMat{1.f};
  glm::vec4 pointLightPosition{0.f, 0.f, 0.f, 1.f};
  glm::vec4 pointLightColor{1.f, 1.f, 1.f, 1.f}; // w is intensity
  glm::vec2 screenSize{800.f, 600.f};            // For SSAO UV calculation
  int texture_idx;
  int debugMode{0}; // 0=normal, 1=SSAO only, 2=normals, 3=depth
};
class FrgModel {
public:
  FrgModel(FrgDevice &device, const std::string &path);
  void draw(VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout,
            SimplePushConstantData push);

  // For rendering passes without push constants (e.g., G-buffer)
  void bind(VkCommandBuffer command_buffer);
  void draw(VkCommandBuffer command_buffer);

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
