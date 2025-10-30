#include "Engine.h"

Engine::Engine(
	int width, int height, const char* title, const std::vector<const char*>& instance_extensions, 
	const std::vector<const char*>& additional_device_extensions
):
	m_height(height), m_width(width), m_title(title), m_extensions(instance_extensions)
{
	glfwInit();

	// We stop glfw from creating an OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(m_width, m_height, m_title, nullptr, nullptr);
	
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

	m_device_extensions.insert(
		std::end(m_device_extensions), std::begin(additional_device_extensions),
		std::end(additional_device_extensions)
	);
}

VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, 
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData
){
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

VkResult Engine::create_debug_utils_messengerEXT(
	VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
	const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger
){
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, p_create_info, p_allocator, p_debug_messenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Engine::destroy_debug_utils_messengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
	const VkAllocationCallbacks* p_allocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) { func(instance, debug_messenger, p_allocator); }
}

std::vector<char> Engine::read_file(const std::string& filename){
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to find file " + filename);
	}
	else std::cout << "opened file:" + filename << std::endl;

	size_t file_size = (size_t)file.tellg();
	std::vector<char> buffer(file_size);
	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	return buffer;
}

void Engine::framebufferResizeCallback(GLFWwindow* window, int width, int height){
	auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
	app->m_framebuffer_resized = true;
}

void Engine::setup_vulkan() {
	create_instance();
	setup_debug_messenger();
	create_surface();
	pick_physical_device();
	create_logical_device();
	create_swap_chain();
	create_render_pass();
}

Engine::~Engine() {
	// We release all the resources that we allocated during runtime
	//Vulkan
	cleanup_swap_chain();
	vkDestroyRenderPass(m_device, m_render_pass, nullptr);
	vkDestroyDevice(m_device, nullptr);
	if (enable_validation_layers) destroy_debug_utils_messengerEXT(m_instance, debug_messenger, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
	//GLFW
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Engine::create_instance() {
	if (enable_validation_layers && !check_validation_layer_support()) {
		throw std::runtime_error("validation layers requested, but they are not available!");
	}

	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Render_Forge";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> extensions = get_required_extensions();

	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
	if (enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();

		populate_debug_messenger_create_info(debug_create_info);
		create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
	}
	else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&create_info, nullptr, &m_instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vulkan instance!");
	}
}

bool Engine::check_validation_layer_support() const{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char* layer_name : validation_layers) {
		bool layer_found = false;

		for (const auto& layer_properties : available_layers) {
			if (strcmp(layer_name, layer_properties.layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found) return false;
	}

	return true;
}

std::vector<const char*> Engine::get_required_extensions() const {
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char*> extensions(glfw_extensions, glfw_extension_count + glfw_extensions);
	if (enable_validation_layers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }
	extensions.insert(std::end(extensions), std::begin(m_extensions), std::end(m_extensions));

	return extensions;
}

void Engine::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
	create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debugCallback;
	create_info.pUserData = nullptr;
}

void Engine::setup_debug_messenger() {
	if (!enable_validation_layers) return;
	VkDebugUtilsMessengerCreateInfoEXT create_info;
	populate_debug_messenger_create_info(create_info);

	if (create_debug_utils_messengerEXT(m_instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to setup debug messenger!");
	}
}

void Engine::create_surface() {
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void Engine::pick_physical_device() {
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
	if (device_count == 0) {
		throw std::runtime_error("failed to find suitable GPUs that support Vulkan!");
	}
	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

	for (const auto& device : devices) {
		if (is_device_suitable(device)) {
			m_physical_device = device;
			m_msaa_samples = get_max_usable_sample_count();
			break;
		}
	}

	if (m_physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

bool Engine::is_device_suitable(VkPhysicalDevice device) const {
	VkPhysicalDeviceProperties device_properties{};
	vkGetPhysicalDeviceProperties(device, &device_properties);

	QueueFamilyIndicies indices = find_queue_families(device);
	bool extension_supported = check_device_extension_support(device);

	bool swap_chain_adequate = false;
	if (extension_supported) {
		SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);
		swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
	}

	VkPhysicalDeviceFeatures features_supported;
	vkGetPhysicalDeviceFeatures(device, &features_supported);

	//Always pick dedicated GPU
	return 
		device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && 
		indices.is_complete() && 
		extension_supported && swap_chain_adequate &&
		features_supported.samplerAnisotropy;
}

QueueFamilyIndicies Engine::find_queue_families(VkPhysicalDevice device) const {
	QueueFamilyIndicies indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const auto& queue_family : queue_families) {
		if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
			indices.graphics_and_compute_families = i;
		}

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present_support);
		if (present_support) {
			indices.present_family = i;
		}

		if (indices.is_complete()) break;

		i++;
	}

	return indices;
}

bool Engine::check_device_extension_support(VkPhysicalDevice device) const {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(m_device_extensions.begin(), m_device_extensions.end());

	for (const auto& extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}

SwapChainSupportDetails Engine::query_swap_chain_support(VkPhysicalDevice device) const {
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);

	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);

	if (present_mode_count != 0) {
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, details.present_modes.data());
	}

	return details;
}

void Engine::create_logical_device() {
	QueueFamilyIndicies indices = find_queue_families(m_physical_device);

	float queue_priority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = { 
		indices.graphics_and_compute_families.value(), indices.present_family.value()
	};

	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features{};
	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(m_device_extensions.size());
	create_info.ppEnabledExtensionNames = m_device_extensions.data();
	device_features.samplerAnisotropy = VK_TRUE;

	if (enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	}
	else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_physical_device, &create_info, nullptr, &m_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to instantiate logical device!");
	}

	vkGetDeviceQueue(m_device, indices.graphics_and_compute_families.value(), 0, &graphics_queue);
	vkGetDeviceQueue(m_device, indices.present_family.value(), 0, &present_queue);
	vkGetDeviceQueue(m_device, indices.graphics_and_compute_families.value(), 0, &compute_queue);
}

void Engine::create_swap_chain() {
	SwapChainSupportDetails swap_chain_support = query_swap_chain_support(m_physical_device);

	VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
	VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
	VkExtent2D extent = choose_swap_extent(swap_chain_support.capabilities);
	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
		image_count = swap_chain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = m_surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndicies indices = find_queue_families(m_physical_device);
	uint32_t queue_family_indices[] = { indices.graphics_and_compute_families.value(), indices.present_family.value() };

	if (indices.graphics_and_compute_families.value() != indices.present_family.value()) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swap_chain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, nullptr);
	m_swap_chain_images.resize(image_count);
	vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, m_swap_chain_images.data());
	m_swap_chain_image_format = surface_format.format;
	m_swap_chain_extent = extent;
}

VkSurfaceFormatKHR Engine::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) const {
	for (const auto& format : available_formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return available_formats[0];
}

VkPresentModeKHR Engine::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) const {
	for (const auto& mode : available_present_modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Engine::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) const {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	VkExtent2D actual_extent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actual_extent;
}

void Engine::cleanup_swap_chain() {
	for (auto image_view : m_swap_chain_image_views) {
		vkDestroyImageView(m_device, image_view, nullptr);
	}

	vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
}

void Engine::create_image_views() {
	m_swap_chain_image_views.resize(m_swap_chain_images.size());
	for (size_t i = 0; i < m_swap_chain_images.size(); ++i) {
		m_swap_chain_image_views[i] = create_image_view(m_swap_chain_images[i], m_swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

VkImageView Engine::create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) const {
	VkImageViewCreateInfo view_info{};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = format;
	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = mip_levels;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	VkImageView image_view;
	if (vkCreateImageView(m_device, &view_info, nullptr, &image_view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return image_view;
}

void Engine::create_render_pass() {
	VkAttachmentDescription color_attachment{};
	color_attachment.format = m_swap_chain_image_format;
	color_attachment.samples = m_msaa_samples;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref{};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	VkAttachmentDescription depth_attachment{};
	depth_attachment.format = find_depth_format();
	depth_attachment.samples = m_msaa_samples;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref{};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription color_attachment_resolve{};
	color_attachment_resolve.format = m_swap_chain_image_format;
	color_attachment_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment_resolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment_resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_resolve_ref{};
	color_attachment_resolve_ref.attachment = 2;
	color_attachment_resolve_ref.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;
	subpass.pResolveAttachments = &color_attachment_resolve_ref;

	std::array<VkAttachmentDescription, 3> attachments = { color_attachment, depth_attachment, color_attachment_resolve };
	VkRenderPassCreateInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	render_pass_info.pAttachments = attachments.data();
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;


	if (vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

VkFormat Engine::find_depth_format() const {
	return find_supported_format({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat Engine::find_supported_format(
	const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features
) const {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkSampleCountFlagBits Engine::get_max_usable_sample_count() const {
	VkPhysicalDeviceProperties physical_device_properties;
	vkGetPhysicalDeviceProperties(m_physical_device, &physical_device_properties);

	VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts & physical_device_properties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}
