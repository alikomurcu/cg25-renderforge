#pragma once
#pragma warning(push, 0)
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>


const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
const int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif



class Engine {
public:
	Engine(int width, int height, const char* title);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    static VkResult create_debug_utils_messengerEXT(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
        const VkAllocationCallbacks* p_allocator,
        VkDebugUtilsMessengerEXT* p_debug_messenger);
    static void destory_debug_utils_messengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
        const VkAllocationCallbacks* p_allocator);
    static std::vector<char> read_file(const std::string& filename);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
private:
	GLFWwindow* m_window;
	const char* m_title;
	const int m_width;
	const int m_height;
    bool m_framebuffer_resized = false;
    VkInstance m_instance = VK_NULL_HANDLE;

	Engine() = delete;
    void create_instance();
    bool check_validation_layer_support() const;
    std::vector<const char*> get_required_extensions() const;
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);
};