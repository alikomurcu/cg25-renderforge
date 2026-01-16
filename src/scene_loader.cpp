#include "scene_loader.hpp"
#include <iostream>
#include <tinyxml2.h>

namespace frg {

void SceneLoader::load(FrgDevice &device, const std::string &filepath,
                       std::vector<FrgGameObject> &gameObjects) {
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLError result = doc.LoadFile(filepath.c_str());
  if (result != tinyxml2::XML_SUCCESS) {
    std::cerr << "Failed to load scene file: " << filepath << std::endl;
    return;
  }

  tinyxml2::XMLElement *scene = doc.FirstChildElement("Scene");
  if (!scene) {
    std::cerr << "No Scene element found in file: " << filepath << std::endl;
    return;
  }

  for (tinyxml2::XMLElement *obj = scene->FirstChildElement("GameObject"); obj;
       obj = obj->NextSiblingElement("GameObject")) {
    const char *modelPath = obj->Attribute("model");
    if (!modelPath) {
      continue;
    }

    auto gameObject = FrgGameObject::createGameObject();
    try {
      gameObject.model = std::make_shared<FrgModel>(device, modelPath);
    } catch (const std::exception &e) {
      std::cerr << "Failed to load model: " << modelPath
                << " Error: " << e.what() << std::endl;
      continue;
    }

    tinyxml2::XMLElement *transform = obj->FirstChildElement("Transform");
    if (transform) {
      tinyxml2::XMLElement *translation =
          transform->FirstChildElement("Translation");
      if (translation) {
        gameObject.transform.translation.x =
            translation->FloatAttribute("x", 0.0f);
        gameObject.transform.translation.y =
            translation->FloatAttribute("y", 0.0f);
        gameObject.transform.translation.z =
            translation->FloatAttribute("z", 0.0f);
      }

      tinyxml2::XMLElement *rotation = transform->FirstChildElement("Rotation");
      if (rotation) {
        gameObject.transform.rotation.x = rotation->FloatAttribute("x", 0.0f);
        gameObject.transform.rotation.y = rotation->FloatAttribute("y", 0.0f);
        gameObject.transform.rotation.z = rotation->FloatAttribute("z", 0.0f);
      }

      tinyxml2::XMLElement *scale = transform->FirstChildElement("Scale");
      if (scale) {
        gameObject.transform.scale.x = scale->FloatAttribute("x", 1.0f);
        gameObject.transform.scale.y = scale->FloatAttribute("y", 1.0f);
        gameObject.transform.scale.z = scale->FloatAttribute("z", 1.0f);
      }
    }

    std::cout << "Loaded object: " << modelPath << std::endl;
    gameObjects.emplace_back(std::move(gameObject));
  }
}

} // namespace frg
