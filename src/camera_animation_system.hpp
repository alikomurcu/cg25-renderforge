#pragma once

#include "frg_game_object.hpp"

// libs
#include <glm/glm.hpp>

// std
#include <vector>

namespace frg {

struct Keyframe {
  float time;
  glm::vec3 position;
  glm::vec3 rotation; // Euler angles in radians (Y, X, Z)
};

class CameraAnimationSystem {
public:
  void addKeyframe(float time, glm::vec3 position, glm::vec3 rotation);

  // Updates the camera object's transform based on the global time
  void update(float globalTime, FrgGameObject& cameraObject);
  float getEndTime() const;

private:
  std::vector<Keyframe> keyframes;
};

} // namespace frg
