#include "first_app.hpp"

#include "frg_camera.hpp"
#include "keyboard_movement_controller.hpp"
#include "simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace frg {
FirstApp::FirstApp() {
  loadGameObjects();
  frgDescriptor.write_descriptor_sets(get_descriptors_of_game_objects());
}

FirstApp::~FirstApp() {}

std::vector<VkDescriptorImageInfo> FirstApp::get_descriptors_of_game_objects() {
  std::vector<VkDescriptorImageInfo> all_descriptor_infos{};
  for (const auto &obj : gameObjects) {
    std::vector<VkDescriptorImageInfo> tmp_infos = obj.model->get_descriptors();
    all_descriptor_infos.insert(all_descriptor_infos.end(), tmp_infos.begin(),
                                tmp_infos.end());
  }

  return all_descriptor_infos;
}

void FirstApp::run() {
  // Get swap chain extent for G-buffer and SSAO - MUST match swap chain!
  VkExtent2D extent = frgRenderer.getSwapChainExtent();
  std::cout << "Swap chain extent: " << extent.width << "x" << extent.height
            << std::endl;

  // Create G-buffer for deferred rendering (same size as swap chain)
  FrgGBuffer gbuffer{frgDevice, extent};

  // Create SSAO system (same size as swap chain)
  FrgSSAO ssao{frgDevice, extent};

  // Create SSAO render system (manages G-buffer, SSAO, and blur passes)
  SSAORenderSystem ssaoRenderSystem{frgDevice, gbuffer, ssao};

  // Create the main render system for final lighting
  SimpleRenderSystem simpleRenderSystem{
      frgDevice, frgRenderer.getSwapChainRenderPass(), frgDescriptor};
  FrgCamera camera{};
  // example camera setup
  camera.setViewTarget(glm::vec3{0.f, 0.f, -2.f}, glm::vec3{0.f, 0.f, 2.5f});
  auto viewerObject = FrgGameObject::createGameObject();
  KeyboardMovementController cameraController;

  auto curTime = std::chrono::high_resolution_clock::now();

  // Add a point light to the scene
  simpleRenderSystem.getLightManager().addPointLight(
      glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.f, 1.f, 1.f), 1.f, 3.f);

  // Wire the blurred SSAO texture to the descriptor set for final lighting
  VkDescriptorImageInfo ssaoDescriptor = ssao.getBlurredDescriptor();
  frgDescriptor.setSSAOTexture(ssaoDescriptor);

  // SSAO toggle (press 'O' to toggle)
  bool ssaoEnabled = true;
  bool oKeyWasPressed = false;

  // Debug mode toggle (press 'D' to cycle)
  // 0=normal, 1=SSAO only, 2=normals, 3=depth
  int debugMode = 0;
  bool cKeyWasPressed = false;

  std::cout << "\n=== Controls ===\n";
  std::cout << "WASD: Move camera\n";
  std::cout << "Arrow keys: Look around\n";
  std::cout << "O: Toggle SSAO\n";
  std::cout << "C: Cycle debug mode (Normal/SSAO/Normals/Depth)\n";
  std::cout << "================\n\n";

  while (!frgWindow.shouldClose()) {
    glfwPollEvents();

    // Check for SSAO toggle (O key) - detect key press, not hold
    bool oKeyPressed =
        glfwGetKey(frgWindow.getGLFWwindow(), GLFW_KEY_O) == GLFW_PRESS;
    if (oKeyPressed && !oKeyWasPressed) {
      ssaoEnabled = !ssaoEnabled;
      std::cout << "SSAO: " << (ssaoEnabled ? "ON" : "OFF") << std::endl;
    }
    oKeyWasPressed = oKeyPressed;

    // Check for debug mode toggle (C key)
    bool cKeyPressed =
        glfwGetKey(frgWindow.getGLFWwindow(), GLFW_KEY_C) == GLFW_PRESS;
    if (cKeyPressed && !cKeyWasPressed) {
      debugMode = (debugMode + 1) % 4;
      const char *modeNames[] = {"Normal", "SSAO Only", "Normals", "Depth"};
      std::cout << "Debug Mode: " << modeNames[debugMode] << std::endl;
    }
    cKeyWasPressed = cKeyPressed;

    float aspect = frgRenderer.getAspectRatio();
    auto newTime = std::chrono::high_resolution_clock::now();
    float frameTime =
        std::chrono::duration<float, std::chrono::seconds::period>(newTime -
                                                                   curTime)
            .count();
    curTime = newTime;

    cameraController.moveInPlaneXZ(frgWindow.getGLFWwindow(), frameTime,
                                   viewerObject);
    camera.setViewYXZ(viewerObject.transform.translation,
                      viewerObject.transform.rotation);

    camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);

    if (auto commandBuffer = frgRenderer.beginFrame()) {
      if (ssaoEnabled) {
        // === PASS 1: G-Buffer ===
        // Render scene to position and normal textures
        ssaoRenderSystem.beginGBufferPass(commandBuffer);
        ssaoRenderSystem.renderGBuffer(commandBuffer, gameObjects, camera);
        ssaoRenderSystem.endGBufferPass(commandBuffer);

        // === PASS 2: SSAO Calculation ===
        // Calculate ambient occlusion from G-buffer
        ssaoRenderSystem.beginSSAOPass(commandBuffer);
        ssaoRenderSystem.renderSSAO(commandBuffer, camera);
        ssaoRenderSystem.endSSAOPass(commandBuffer);

        // === PASS 3: Blur ===
        // Blur the noisy SSAO output
        ssaoRenderSystem.beginBlurPass(commandBuffer);
        ssaoRenderSystem.renderBlur(commandBuffer);
        ssaoRenderSystem.endBlurPass(commandBuffer);
      }

      // === PASS 4: Final Lighting ===
      // Render the scene with lighting (uses blurred SSAO for ambient)
      frgRenderer.beginSwapChainRenderPass(commandBuffer);
      simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera,
                                           frameTime, extent, debugMode);

      frgRenderer.endSwapChainRenderPass(commandBuffer);
      frgRenderer.endFrame();
    }
  }

  vkDeviceWaitIdle(frgDevice.device());
}

void FirstApp::loadGameObjects() {
  // Load viking_room model with texture - great for SSAO testing
  auto g_obj = FrgGameObject::createGameObject();
  g_obj.model = std::make_shared<FrgModel>(
      frgDevice, "../resources/models/viking_room/viking_room.obj");

  std::cout << "# of vertices: " << g_obj.model->vertex_count() << std::endl;

  gameObjects.emplace_back(std::move(g_obj));

  // Position and scale the viking room for good SSAO visibility
  gameObjects[0].transform.scale = glm::vec3{1.0f, 1.0f, 1.0f};
  gameObjects[0].transform.rotation.x =
      -glm::half_pi<float>(); // Rotate to face up
  gameObjects[0].transform.translation = glm::vec3{0.0f, 0.0f, 2.0f};
}
} // namespace frg