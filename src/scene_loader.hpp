#pragma once

#include "frg_device.hpp"
#include "frg_game_object.hpp"

#include "frg_lighting.hpp"
#include <string>
#include <vector>

namespace frg {

struct SceneSettings {
  bool autoCamera{true};
  bool ssaoEnabled{true};
  int debugMode{0};
};

class SceneLoader {
public:
  static void load(FrgDevice &device, const std::string &filepath,
                   std::vector<FrgGameObject> &gameObjects,
                   FrgGameObject &cameraObject, LightManager &lightManager,
                   SceneSettings &sceneSettings);
};

} // namespace frg
