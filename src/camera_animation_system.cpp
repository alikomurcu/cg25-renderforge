#include "camera_animation_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

// std
#include <algorithm>
#include <iostream>

namespace frg {

void CameraAnimationSystem::addKeyframe(float time, glm::vec3 position,
                                        glm::vec3 rotation) {
  keyframes.push_back({time, position, rotation});
  // Keep keyframes sorted by time
  std::sort(
      keyframes.begin(), keyframes.end(),
      [](const Keyframe &a, const Keyframe &b) { return a.time < b.time; });
}

void CameraAnimationSystem::update(float globalTime,
                                   FrgGameObject &cameraObject) {
  if (keyframes.empty())
    return;

  // Handle case before first keyframe
  if (globalTime <= keyframes.front().time) {
    cameraObject.transform.translation = keyframes.front().position;
    cameraObject.transform.rotation = keyframes.front().rotation;
    return;
  }

  // Handle case after last keyframe
  if (globalTime >= keyframes.back().time) {
    cameraObject.transform.translation = keyframes.back().position;
    cameraObject.transform.rotation = keyframes.back().rotation;
    return;
  }

  // Find the two keyframes surrounding the current time
  size_t nextIdx = 0;
  while (nextIdx < keyframes.size() && keyframes[nextIdx].time < globalTime) {
    nextIdx++;
  }

  size_t prevIdx = nextIdx - 1;
  const auto &prevFrame = keyframes[prevIdx];
  const auto &nextFrame = keyframes[nextIdx];

  // Calculate interpolation factor (0.0 to 1.0)
  float t = (globalTime - prevFrame.time) / (nextFrame.time - prevFrame.time);

  // Linear interpolation for position
  cameraObject.transform.translation =
      glm::mix(prevFrame.position, nextFrame.position, t);

  // Linear interpolation for rotation (Euler angles)
  // This avoids artifacts from Quaternion conversions when coordinate systems mismatch
  cameraObject.transform.rotation =
      glm::mix(prevFrame.rotation, nextFrame.rotation, t);
}

float CameraAnimationSystem::getEndTime() const {
  if (keyframes.empty()) {
    return 0.f;
  }
  return keyframes.back().time;
}

} // namespace frg
