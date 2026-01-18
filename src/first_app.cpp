#include "first_app.hpp"

#include "camera_animation_system.hpp"
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

#include "scene_loader.hpp"

namespace frg {
FirstApp::FirstApp() {
    loadGameObjects();
    frgDescriptor.write_descriptor_sets(get_descriptors_of_game_objects());
    computeCommandBuffers = frgDevice.createComputeCommandBuffers(FrgSwapChain::MAX_FRAMES_IN_FLIGHT);
}

FirstApp::~FirstApp() {
}

std::vector<VkDescriptorImageInfo> FirstApp::get_descriptors_of_game_objects() {
    std::vector<VkDescriptorImageInfo> all_descriptor_infos{};
    for (const auto &obj : gameObjects) {
        std::vector<VkDescriptorImageInfo> tmp_infos = obj.model->get_descriptors();
        all_descriptor_infos.insert(all_descriptor_infos.end(), tmp_infos.begin(), tmp_infos.end());
    }

    return all_descriptor_infos;
}

void FirstApp::run() {
    // Get swap chain extent for G-buffer and SSAO - MUST match swap chain!
    VkExtent2D extent = frgRenderer.getSwapChainExtent();
    std::cout << "Swap chain extent: " << extent.width << "x" << extent.height << std::endl;

    // Create G-buffer for deferred rendering (same size as swap chain)
    FrgGBuffer gbuffer{frgDevice, extent};

    // Create SSAO system (same size as swap chain)
    FrgSSAO ssao{frgDevice, extent};

    // Create SSAO render system (manages G-buffer, SSAO, and blur passes)
    SSAORenderSystem ssaoRenderSystem{frgDevice, gbuffer, ssao};

    // Create the main render system for final lighting
    SimpleRenderSystem simpleRenderSystem{frgDevice, frgRenderer.getSwapChainRenderPass(),
                                          frgDescriptor, lightManager};
    simpleRenderSystem.setup_ssbos(frgParticleDispenser);
    simpleRenderSystem.set_up_compute_desc_sets(frgParticleDispenser.particle_count() * sizeof(Particle));
  
    FrgCamera camera{};
    // example camera setup - now loaded from scene if available
    // camera.setViewTarget(glm::vec3{0.f, 0.f, -2.f}, glm::vec3{0.f, 0.f, 2.5f});

    KeyboardMovementController cameraController;

    // Camera Animation System Setup
    CameraAnimationSystem cameraAnimationSystem;
    // Create a circular path around the viking room at (0,0,2)
    // Radius ~3.0
    // t=0: Start position
    cameraAnimationSystem.addKeyframe(0.f, glm::vec3{-6.f, -1.f, -6.f}, glm::vec3{0.f, 0.78f, 0.f});
    // t=2.5: Middle position
    cameraAnimationSystem.addKeyframe(2.5f, glm::vec3{-6.f, -1.f, 6.f}, glm::vec3{0.f, 1.57f, 0.f});
    cameraAnimationSystem.addKeyframe(5.f, glm::vec3{6.f, -2.f, 6.f}, glm::vec3{0.f, 3.92f, 0.f});
    cameraAnimationSystem.addKeyframe(7.5f, glm::vec3{6.f, -2.f, -6.f}, glm::vec3{0.f, 3.92f + 1.57f, 0.f});
    cameraAnimationSystem.addKeyframe(10.f, glm::vec3{-6.f, -1.f, -6.f}, glm::vec3{0.f, 0.78f, 0.f});

    bool isAutoCamera = sceneSettings.autoCamera; // Start with animation enabled from config
    bool mKeyWasPressed = false;
    float animationTime = 0.f;

    auto curTime = std::chrono::high_resolution_clock::now();

    // Wire the blurred SSAO texture to the descriptor set for final lighting
    VkDescriptorImageInfo ssaoDescriptor = ssao.getBlurredDescriptor();
    frgDescriptor.setSSAOTexture(ssaoDescriptor);

    // SSAO toggle (press 'O' to toggle)
    bool ssaoEnabled = sceneSettings.ssaoEnabled;
    bool oKeyWasPressed = false;

    // Debug mode toggle (press 'D' to cycle)
    // 0=normal, 1=SSAO only, 2=normals, 3=depth
    int debugMode = sceneSettings.debugMode;
    bool cKeyWasPressed = false;

    std::cout << "\n=== Controls ===\n";
    std::cout << "M: Toggle Camera Animation (Auto/Manual)\n";
    std::cout << "WASD: Move camera (Manual mode)\n";
    std::cout << "Arrow keys: Look around (Manual mode)\n";
    std::cout << "O: Toggle SSAO\n";
    std::cout << "C: Cycle debug mode (Normal/SSAO/Normals/Depth)\n";
    std::cout << "================\n\n";

    while (!frgWindow.shouldClose()) {
        glfwPollEvents();

        // Check for Camera Mode toggle (M key)
        bool mKeyPressed = glfwGetKey(frgWindow.getGLFWwindow(), GLFW_KEY_M) == GLFW_PRESS;
        if (mKeyPressed && !mKeyWasPressed) {
            isAutoCamera = !isAutoCamera;
            std::cout << "Camera Mode: " << (isAutoCamera ? "Auto (Animation)" : "Manual")
                      << std::endl;
            // Reset animation time when switching to auto? Or keep it running?
            // Let's reset to restart the cinematic feel.
            if (isAutoCamera)
                animationTime = 0.f;
        }
        mKeyWasPressed = mKeyPressed;

        // Check for SSAO toggle (O key) - detect key press, not hold
        bool oKeyPressed = glfwGetKey(frgWindow.getGLFWwindow(), GLFW_KEY_O) == GLFW_PRESS;
        if (oKeyPressed && !oKeyWasPressed) {
            ssaoEnabled = !ssaoEnabled;
            std::cout << "SSAO: " << (ssaoEnabled ? "ON" : "OFF") << std::endl;
        }
        oKeyWasPressed = oKeyPressed;

        // Check for debug mode toggle (C key)
        bool cKeyPressed = glfwGetKey(frgWindow.getGLFWwindow(), GLFW_KEY_C) == GLFW_PRESS;
        if (cKeyPressed && !cKeyWasPressed) {
            debugMode = (debugMode + 1) % 4;
            const char *modeNames[] = {"Normal", "SSAO Only", "Normals", "Depth"};
            std::cout << "Debug Mode: " << modeNames[debugMode] << std::endl;
        }
        cKeyWasPressed = cKeyPressed;

        float aspect = frgRenderer.getAspectRatio();
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime =
            std::chrono::duration<float, std::chrono::seconds::period>(newTime - curTime).count();
        curTime = newTime;

        if (isAutoCamera) {
            animationTime += frameTime;
            // Loop the animation
            float animationEndTime = cameraAnimationSystem.getEndTime();
            if (animationTime > animationEndTime) {
                animationTime = 0.f;
            }
          cameraAnimationSystem.update(animationTime, viewerObject);
        } else {
          cameraController.moveInPlaneXZ(frgWindow.getGLFWwindow(), frameTime,
                                        viewerObject);
        }

        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

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
            } else {
                // If SSAO is disabled, we still need to clear the texture that the
                // lighting pass reads from. The blur pass RenderPass is configured to
                // loadOp = CLEAR (to white 1.0) So just beginning and ending the pass
                // is enough to clear it.
                ssaoRenderSystem.beginBlurPass(commandBuffer);
                ssaoRenderSystem.endBlurPass(commandBuffer);
            }

            // === PASS 4: Final Lighting ===
            // Render the scene with lighting (uses blurred SSAO for ambient)
            frgRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera, frameTime, extent, debugMode);
            simpleRenderSystem.bindComputeGraphicsPipeline(commandBuffer);
            UniformBufferObject ubo{};
            ubo.deltaTime = frameTime;
            SimplePushConstantData push{};
            auto projView = camera.getProjectionMatrix() * camera.getViewMatrix();
            auto modelMat = frgParticleDispenser.transform.mat4();
            push.transform = projView * modelMat;
            vkCmdPushConstants(
                commandBuffer,
                simpleRenderSystem.getComputeGraphicsPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(SimplePushConstantData),
                &push
            );

            ubo.w_parent_pos = {frgParticleDispenser.transform.translation, .6f};
            frgRenderer.renderComputePipeline(
                computeCommandBuffers,
                frgDescriptor,
                simpleRenderSystem.getComputePipelineLayout(),
                simpleRenderSystem.getComputePipeline(),
                frgParticleDispenser.particle_count(),
                ubo,
                simpleRenderSystem.getUbosMapped()
            );
            frgRenderer.delegateComputeBindAndDraw(
                commandBuffer,
                simpleRenderSystem.getSSBOS(),
                frgParticleDispenser.particle_count()
            );


            frgRenderer.endSwapChainRenderPass(commandBuffer);
            frgRenderer.endFrame(true);
        }
    }

    vkDeviceWaitIdle(frgDevice.device());
}

void FirstApp::loadGameObjects() {
    SceneLoader::load(frgDevice, "../resources/scenes/scene0.xml", gameObjects, viewerObject,
                      lightManager, sceneSettings);
}
} // namespace frg