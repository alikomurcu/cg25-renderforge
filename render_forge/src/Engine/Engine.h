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
#include <set>
#include <algorithm>
#include <array>

#include "QueueFamilyIndicies.h"
#include "SwapChainSupportDetails.h"


const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
const int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

class Engine {
public:
    Engine(
        int width, int height, const char* title, 
        const std::vector<const char*>& instance_extensions = std::vector < const char* > (),
        const std::vector<const char*>& additional_device_extensions = std::vector<const char*>());

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    static VkResult create_debug_utils_messengerEXT(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
        const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger);
    static void destroy_debug_utils_messengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
        const VkAllocationCallbacks* p_allocator);
    static std::vector<char> read_file(const std::string& filename);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    void setup_vulkan();
    ~Engine();
private:
    // ============== Window variables ==============
	GLFWwindow* m_window;
	const char* m_title;
	const int m_width;
	const int m_height;
    bool m_framebuffer_resized = false;

    // ============== Vulkan variables ==============
    VkInstance m_instance = VK_NULL_HANDLE;
    //Vector to add custom extensions on top of the required ones, can be set in constructor
    std::vector<const char*> m_extensions;
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkSampleCountFlagBits m_msaa_samples = VK_SAMPLE_COUNT_1_BIT;
    std::vector<const char*> m_device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;
    VkQueue compute_queue = VK_NULL_HANDLE;
    VkSwapchainKHR m_swap_chain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swap_chain_images;
    VkFormat m_swap_chain_image_format;
    VkExtent2D m_swap_chain_extent;
    std::vector<VkImageView> m_swap_chain_image_views;
    VkRenderPass m_render_pass = VK_NULL_HANDLE;

	Engine() = delete;
    void create_instance();
    bool check_validation_layer_support() const;
    std::vector<const char*> get_required_extensions() const;
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);
    void setup_debug_messenger();
    void create_surface();
    void pick_physical_device();
    bool is_device_suitable(VkPhysicalDevice device) const;
    QueueFamilyIndicies find_queue_families(VkPhysicalDevice device) const;
    VkSampleCountFlagBits get_max_usable_sample_count() const;
    bool check_device_extension_support(VkPhysicalDevice device) const;
    SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device) const;
    void create_logical_device();
    void create_swap_chain();
    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) const;
    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) const;
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    void cleanup_swap_chain();
    void create_image_views();
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) const;
    void create_render_pass();
    VkFormat find_depth_format() const;
    VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
};