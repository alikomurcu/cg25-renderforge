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
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace frg {
class FrgModel {
  public:
    FrgModel(FrgDevice &device, const std::string &path);
    void draw(VkCommandBuffer command_buffer);

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