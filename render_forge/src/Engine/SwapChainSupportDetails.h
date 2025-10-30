#pragma once
#pragma warning(push, 0)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma warning(pop)

#include <vector>

struct SwapChainSupportDetails {
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
    VkSurfaceCapabilitiesKHR capabilities{};
};