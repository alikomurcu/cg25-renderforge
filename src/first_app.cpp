#include "first_app.hpp"

#include "frg_camera.hpp"
#include "simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace frg {
FirstApp::FirstApp() { loadGameObjects(); }

FirstApp::~FirstApp() {}

void FirstApp::run() {
    SimpleRenderSystem simpleRenderSystem{frgDevice,
                                          frgRenderer.getSwapChainRenderPass(),
                                          frgDescriptor};
    FrgCamera camera{};
    // example camera setup
    camera.setViewTarget(glm::vec3{0.f, 0.f, -2.f}, glm::vec3{0.f, 0.f, 2.5f});

    while (!frgWindow.shouldClose()) {
        glfwPollEvents();
        float aspect = frgRenderer.getAspectRatio();
        // camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);

        if (auto commandBuffer = frgRenderer.beginFrame()) {
            // make the render passes here, for example: shadow pass, post
            // processing pass, etc.
            frgRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer,
                                                 gameObjects,
                                                 camera);

            frgRenderer.endSwapChainRenderPass(commandBuffer);
            frgRenderer.endFrame();
        }
    }

    vkDeviceWaitIdle(frgDevice.device());
}
// temporary helper function, creates a 1x1x1 cube centered at offset
std::unique_ptr<FrgModel> createCubeModel(FrgDevice &device, glm::vec3 offset) {
    // std::vector<FrgModel::Vertex> vertices{

    //     // left face (white)
    //     { {-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
    //     {   {-.5f, .5f, .5f}, {.9f, .9f, .9f}},
    //     {  {-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
    //     { {-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
    //     {  {-.5f, .5f, -.5f}, {.9f, .9f, .9f}},
    //     {   {-.5f, .5f, .5f}, {.9f, .9f, .9f}},

    //     // right face (yellow)
    //     {  {.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
    //     {    {.5f, .5f, .5f}, {.8f, .8f, .1f}},
    //     {   {.5f, -.5f, .5f}, {.8f, .8f, .1f}},
    //     {  {.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
    //     {   {.5f, .5f, -.5f}, {.8f, .8f, .1f}},
    //     {    {.5f, .5f, .5f}, {.8f, .8f, .1f}},

    //     // top face (orange, remember y axis points down)
    //     { {-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
    //     {   {.5f, -.5f, .5f}, {.9f, .6f, .1f}},
    //     {  {-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
    //     { {-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
    //     {  {.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
    //     {   {.5f, -.5f, .5f}, {.9f, .6f, .1f}},

    //     // bottom face (red)
    //     {  {-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
    //     {    {.5f, .5f, .5f}, {.8f, .1f, .1f}},
    //     {   {-.5f, .5f, .5f}, {.8f, .1f, .1f}},
    //     {  {-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
    //     {   {.5f, .5f, -.5f}, {.8f, .1f, .1f}},
    //     {    {.5f, .5f, .5f}, {.8f, .1f, .1f}},

    //     // nose face (blue)
    //     { {-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
    //     {   {.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
    //     {  {-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
    //     { {-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
    //     {  {.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
    //     {   {.5f, .5f, 0.5f}, {.1f, .1f, .8f}},

    //     // tail face (green)
    //     {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
    //     {  {.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
    //     { {-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
    //     {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
    //     { {.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
    //     {  {.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
    // };
    // for (auto &v : vertices) {
    //     v.position += offset;
    // }
    // return std::make_unique<FrgModel>(device, vertices);
    return nullptr;
}

void FirstApp::loadGameObjects() {
    std::string file_path = "../resources/models/stanford_bunny.obj";
    auto g_obj = FrgGameObject::createGameObject();
    g_obj.model = std::make_shared<FrgModel>(frgDevice, file_path);
    g_obj.transform.translation = {0.f, 0.f, 2.5f};
    gameObjects.emplace_back(std::move(g_obj));
}
} // namespace frg
