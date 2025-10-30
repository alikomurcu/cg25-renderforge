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


class Engine {
public:
	Engine(int width, int height, const char* title);
private:
	GLFWwindow* m_window;
	const char* m_title;
	const int m_width;
	const int m_height;
	Engine() = delete;
};