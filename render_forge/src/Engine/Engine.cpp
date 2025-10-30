#include "Engine.h"

Engine::Engine(int width, int height, const char* title): m_height(height), m_width(width), m_title(title) {
	glfwInit();

	// We stop glfw from creating an OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(m_width, m_height, m_title, nullptr, nullptr);
}
