#include "frg_model.hpp"

namespace frg {
FrgModel::FrgModel(FrgDevice &device, const std::string &path)
    : frg_device(device) {
  load_model(path);
}
void FrgModel::draw(VkCommandBuffer command_buffer,
                    VkPipelineLayout pipeline_layout,
                    SimplePushConstantData push) {
  for (const auto &mesh : meshes) {
    std::optional<uint32_t> tex_idx = mesh->getTextureIndex();
    if (tex_idx.has_value()) {
      push.texture_idx = static_cast<int>(tex_idx.value());
    } else {
      push.texture_idx = -1; // Negative indicates no texture
    }

    vkCmdPushConstants(command_buffer, pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(SimplePushConstantData), &push);
    mesh->bind(command_buffer);
    mesh->draw(command_buffer);
  }
}

void FrgModel::bind(VkCommandBuffer command_buffer) {
  // Bind all meshes - they share the same vertex/index buffers
  for (const auto &mesh : meshes) {
    mesh->bind(command_buffer);
  }
}

void FrgModel::draw(VkCommandBuffer command_buffer) {
  // Draw all meshes without push constants
  for (const auto &mesh : meshes) {
    mesh->bind(command_buffer);
    mesh->draw(command_buffer);
  }
}

void FrgModel::load_model(const std::string &path) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      path, aiProcess_Triangulate | aiProcess_SortByPType |
                aiProcess_GenNormals | aiProcess_FlipUVs |
                aiProcess_PreTransformVertices | aiProcess_ConvertToLeftHanded);

  if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    throw std::runtime_error(importer.GetErrorString());
  }

  dir = path.substr(0, path.find_last_of('/'));

  process_node(scene->mRootNode, scene);
}

void FrgModel::process_node(aiNode *node, const aiScene *scene) {
  for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    meshes.emplace_back(process_mesh(mesh, scene));
  }

  for (unsigned int i = 0; i < node->mNumChildren; ++i) {
    process_node(node->mChildren[i], scene);
  }
}

std::unique_ptr<FrgMesh> FrgModel::process_mesh(aiMesh *mesh,
                                                const aiScene *scene) {
  std::vector<Vertex> vertices;
  std::vector<std::unique_ptr<Texture>> textures;
  std::vector<unsigned int> indices;

  glm::vec3 vector;
  for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
    Vertex vertex;
    vector.x = mesh->mVertices[i].x;
    vector.y = mesh->mVertices[i].y;
    vector.z = mesh->mVertices[i].z;
    vertex.position = vector;
    if ((mesh->mNormals != nullptr)) {
      vector.x = mesh->mNormals[i].x;
      vector.y = mesh->mNormals[i].y;
      vector.z = mesh->mNormals[i].z;
    }
    vertex.normal = vector;

    if (mesh->mTextureCoords[0]) {
      glm::vec2 vec;
      vec.x = mesh->mTextureCoords[0][i].x;
      vec.y = mesh->mTextureCoords[0][i].y;
      vertex.tex_coord = vec;
    } else
      vertex.tex_coord = {0.0f, 0.0f};

    vertices.emplace_back(vertex);
  }

  for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; ++j) {
      indices.push_back(face.mIndices[j]);
    }
  }

  if (mesh->mMaterialIndex >= 0) {
    aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
    std::vector<std::unique_ptr<Texture>> diffuse_maps =
        load_material_textures(mat, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(),
                    std::make_move_iterator(diffuse_maps.begin()),
                    std::make_move_iterator(diffuse_maps.end()));
    std::vector<std::unique_ptr<Texture>> specular_maps =
        load_material_textures(mat, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(),
                    std::make_move_iterator(specular_maps.begin()),
                    std::make_move_iterator(specular_maps.end()));
  }

  return std::make_unique<FrgMesh>(frg_device, vertices, indices,
                                   std::move(textures));
}

std::vector<std::unique_ptr<Texture>>
FrgModel::load_material_textures(aiMaterial *mat, aiTextureType type,
                                 std::string type_name) {
  std::vector<std::unique_ptr<Texture>> textures;
  for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
    aiString str;
    mat->GetTexture(type, i, &str);
    std::string texture_path = dir + "/" + str.C_Str();
    textures.emplace_back(
        std::make_unique<Texture>(frg_device, type_name, texture_path));
  }
  return textures;
}

std::vector<VkDescriptorImageInfo> FrgModel::get_descriptors() {
  std::vector<VkDescriptorImageInfo> descriptor_infos{};
  std::set<uint32_t> added_text_idx{};
  for (const auto &mesh : meshes) {
    for (const auto &texture : mesh->textures) {
      if (added_text_idx.find(texture->textureIdx()) != added_text_idx.end())
        continue;
      descriptor_infos.emplace_back(texture->descriptor_image_info);
      added_text_idx.insert(texture->textureIdx());
    }
  }

  return descriptor_infos;
}

// Beware that this function takes ownership of the texture pointer passed in as
// argument
void FrgModel::add_texture_to_mesh(size_t idx,
                                   std::unique_ptr<Texture> &texture) {
  if (idx >= meshes.size()) {
    std::cerr << "Index is larger than there are number of meshes."
              << std::endl;
    return;
  }

  meshes[idx]->textures.emplace_back(std::move(texture));
}
} // namespace frg