#pragma once

#include "frg_device.hpp"
#include "frg_game_object.hpp"

#include <string>
#include <vector>

namespace frg {

class SceneLoader {
public:
  static void load(FrgDevice &device, const std::string &filepath,
                   std::vector<FrgGameObject> &gameObjects);
};

} // namespace frg
