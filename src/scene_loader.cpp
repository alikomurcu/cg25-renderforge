#include "scene_loader.hpp"
#include <iostream>
#include <tinyxml2.h>

namespace frg {

// ... (previous code)

void SceneLoader::load(FrgDevice &device, const std::string &filepath,
                       std::vector<FrgGameObject> &gameObjects,
                       FrgGameObject &cameraObject, LightManager &lightManager,
                       SceneSettings &sceneSettings) {
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

  // Settings Loader
  tinyxml2::XMLElement *settings = scene->FirstChildElement("Settings");
  if (settings) {
    tinyxml2::XMLElement *autoCam = settings->FirstChildElement("AutoCamera");
    if (autoCam) {
      sceneSettings.autoCamera = autoCam->BoolAttribute("enabled", true);
    }
    tinyxml2::XMLElement *ssao = settings->FirstChildElement("SSAO");
    if (ssao) {
      sceneSettings.ssaoEnabled = ssao->BoolAttribute("enabled", true);
    }
    tinyxml2::XMLElement *debug = settings->FirstChildElement("DebugMode");
    if (debug) {
      sceneSettings.debugMode = debug->IntAttribute("value", 0);
    }
  }

  // Camera Loader
  tinyxml2::XMLElement *cam = scene->FirstChildElement("Camera");
  if (cam) {
    tinyxml2::XMLElement *transform = cam->FirstChildElement("Transform");
    if (transform) {
      tinyxml2::XMLElement *translation =
          transform->FirstChildElement("Translation");
      if (translation) {
        cameraObject.transform.translation.x =
            translation->FloatAttribute("x", 0.0f);
        cameraObject.transform.translation.y =
            translation->FloatAttribute("y", -2.0f); // default fallback
        cameraObject.transform.translation.z =
            translation->FloatAttribute("z", -5.0f);
      }
      tinyxml2::XMLElement *rotation = transform->FirstChildElement("Rotation");
      if (rotation) {
        cameraObject.transform.rotation.x = rotation->FloatAttribute("x", 0.0f);
        cameraObject.transform.rotation.y = rotation->FloatAttribute("y", 0.0f);
        cameraObject.transform.rotation.z = rotation->FloatAttribute("z", 0.0f);
      }
    }
  }

  // Light Loader
  for (tinyxml2::XMLElement *light = scene->FirstChildElement("Light"); light;
       light = light->NextSiblingElement("Light")) {
    const char *type = light->Attribute("type");
    if (type && std::string(type) == "Point") {
      glm::vec3 pos{0.f};
      glm::vec3 color{1.f};
      float intensity = 1.0f;
      float radius = 1.0f;

      tinyxml2::XMLElement *posEl = light->FirstChildElement("Position");
      if (posEl) {
        pos.x = posEl->FloatAttribute("x", 0.f);
        pos.y = posEl->FloatAttribute("y", 0.f);
        pos.z = posEl->FloatAttribute("z", 0.f);
      }
      tinyxml2::XMLElement *colEl = light->FirstChildElement("Color");
      if (colEl) {
        color.r = colEl->FloatAttribute("r", 1.f);
        color.g = colEl->FloatAttribute("g", 1.f);
        color.b = colEl->FloatAttribute("b", 1.f);
        intensity = colEl->FloatAttribute("intensity", 1.f);
      }
      // Assuming naive radius mapping from intensity or a separate field if
      // needed. Or we can add Radius element. For now let's just assume simple
      // defaults if not specified.
      radius = intensity * 10.0f;

      lightManager.addPointLight(pos, color, intensity, radius);
      std::cout << "Loaded Point Light at " << pos.x << "," << pos.y << ","
                << pos.z << std::endl;
    }
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
