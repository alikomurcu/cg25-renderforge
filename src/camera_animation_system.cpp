#include "camera_animation_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

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

  // Slerp for rotation
  // Convert Euler angles to Quaternions
  // Note: glm::quat constructor from euler angles expects (pitch, yaw, roll)
  // usually roughly (x, y, z) But our FrgCamera uses YXZ order in setViewYXZ.
  // Let's create Quaternions consistent with YXZ rotation order if possible,
  // or generally simple YXZ euler to quat conversion.
  // glm::quat(vec3) takes euler angles.
  glm::quat prevRot = glm::quat(prevFrame.rotation);
  glm::quat nextRot = glm::quat(nextFrame.rotation);

  // Slerp
  glm::quat currentRot = glm::slerp(prevRot, nextRot, t);

  // Convert back to Euler angles
  cameraObject.transform.rotation = glm::eulerAngles(currentRot);
}

float CameraAnimationSystem::getEndTime() const {
  if (keyframes.empty()) {
    return 0.f;
  }
  return keyframes.back().time;
}

} // namespace frg
