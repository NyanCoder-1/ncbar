#define __WAYLAND_CORE__
#include "core.hpp"
#include "globals.hpp"
#include "vulkanHelper.hpp"
#include <cstring>
#include <iostream>
#include <ranges>
#include <string_view>
#include <vector>

namespace {
	// Wayland
	void wlRegistryOnGlobalListener(void *data, wl_registry *registry, uint32_t name, const char* interface, uint32_t version)
	{
		std::cout << "Wayland: " << interface << " version " << version << std::endl;
		if (data) {
			if (auto onGlobal = reinterpret_cast<Core::WlRegistryListenerWrapper*>(data)->onGlobal)
				onGlobal(registry, name, interface, version);
		}
	}
	const wl_registry_listener wlRegistryListener = {
		.global = wlRegistryOnGlobalListener,
		.global_remove = nullptr
	};
	void xdgWmBasePingListener(void *data, xdg_wm_base *shell, uint32_t serial)
	{
		if (data) {
			if (auto onPing = reinterpret_cast<Core::XdgWmBaseListenerWrapper*>(data)->onPing)
				onPing(shell, serial);
		}
	};
	const xdg_wm_base_listener xdgWmBaseListener = {
		.ping = xdgWmBasePingListener
	};

	// Vulkan
	constexpr const char* const instanceExtensionNames[] = {
		"VK_EXT_debug_utils",
		"VK_KHR_surface",
		"VK_KHR_wayland_surface"
	};
	constexpr const std::size_t instanceExtensionCount = std::size(instanceExtensionNames);
	constexpr const char* const layerNames[] = {
		"VK_LAYER_KHRONOS_validation"
	};
	constexpr const std::size_t layerCount = std::size(layerNames);
	constexpr const char* const deviceExtensionNames[] = {
		"VK_KHR_swapchain"
	};
	constexpr const std::size_t deviceExtensionCount = std::size(deviceExtensionNames);
	VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void* userData)
	{
		(void)userData;
		auto typeName = [](VkDebugUtilsMessageTypeFlagsEXT type) {
			switch (type) {
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
				return "general";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
				return "validation";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
				return "performance";
			default:
				return "unknown";
			}
		};
		auto severityName = [](VkDebugUtilsMessageSeverityFlagBitsEXT severity) {
			switch (severity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				return "error";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				return "warning";
			default:
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				return "info";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				return "verbose";
			}
		};

		std::cout << "Vulkan " << typeName(type) << " (" << severityName(severity) << "): " << data->pMessage << std::endl;

		return VK_FALSE;
	}
}

Core::Core(const Core::Private&)
{
	wlRegistryListenerWrapper = std::make_unique<Core::WlRegistryListenerWrapper>();
	xdgWmBaseListenerWrapper = std::make_unique<Core::XdgWmBaseListenerWrapper>();
}

Core::~Core()
{
	if (device) {
		vkDestroyDevice(device, nullptr);
		device = nullptr;
	}
	if (instance) {
		// Destroy debug messenger
		auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXT) {
			vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
		}
		vkDestroyInstance(instance, nullptr);
		instance = nullptr;
	}
}

bool Core::Init()
{
	// ==== Wayland ====
	// Connect to the wl_display
	display = wl_display_connect(nullptr);
	if (!display) {
		std::cerr << "Wayland: Failed to connect to display" << std::endl;
		return false;
	}

	// Get the registry
	wl_registry *registry = wl_display_get_registry(display);
	// Add the listener to get the compositor and shell
	wlRegistryListenerWrapper->onGlobal = [this](wl_registry *registry, uint32_t name, const char* interface, uint32_t version) {
		(void)version;
		if (strcmp(interface, wl_compositor_interface.name) == 0) {
			this->compositor = reinterpret_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
		}
		else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
			this->shell = reinterpret_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
		}
		else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
			this->layerShell = reinterpret_cast<zwlr_layer_shell_v1*>(wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1));
		}
	};
	wl_registry_add_listener(registry, &wlRegistryListener, wlRegistryListenerWrapper.get());

	wl_display_roundtrip(display);

	if (!compositor) {
		std::cerr << "Wayland: Failed to get compositor" << std::endl;
		return false;
	}
	if (!shell) {
		std::cerr << "Wayland: Failed to get xdg_wm_base" << std::endl;
		return false;
	}
	if (!layerShell) {
		std::cerr << "Wayland: Failed to get zwlr_layer_shell_v1" << std::endl;
		return false;
	}

	// Add the ping listener to the shell
	xdgWmBaseListenerWrapper->onPing = [this](xdg_wm_base *shell, uint32_t serial) {
		xdg_wm_base_pong(shell, serial);
	};
	xdg_wm_base_add_listener(shell, &xdgWmBaseListener, xdgWmBaseListenerWrapper.get());

	// ==== Graphics ====
	TryInitVulkan();

	return true;
}
void Core::TryInitVulkan()
{
	if (!vulkanInitialized) {
		if (!InitVkInstance()) {
			std::cerr << "Vulkan: Failed to create Vulkan instance" << std::endl;
			return;
		}
		if (!InitVkMessenger()) {
			std::cerr << "Vulkan: Failed to create Vulkan debug messenger" << std::endl;
			return;
		}
		if (!InitVkDevice()) {
			std::cerr << "Failed to create Vulkan device" << std::endl;
			return;
		}
		vulkanInitialized = true;
	}
}
bool Core::InitVkInstance()
{
	VkApplicationInfo appInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = appName,
		.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
		.pEngineName = appName,
		.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
		.apiVersion = VK_API_VERSION_1_0
	};
	VkInstanceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = instanceExtensionCount,
		.ppEnabledExtensionNames = instanceExtensionNames
	};

	// Check if all required layers are available
	std::size_t foundLayers = 0;
	uint32_t instanceLayersCount = 0;
	CHECK_VK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayersCount, nullptr));
	std::vector<VkLayerProperties> instanceLayers(instanceLayersCount);
	CHECK_VK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayersCount, instanceLayers.data()));
	for (auto instanceLayer : instanceLayers) {
		for (const auto &layerName : layerNames) {
			if ((std::string_view)instanceLayer.layerName == layerName) {
				foundLayers++;
				break;
			}
		}
	}
	if (foundLayers >= layerCount) {
		createInfo.enabledLayerCount = layerCount;
		createInfo.ppEnabledLayerNames = layerNames;
	}

	// Create instance
	CHECK_VK_RESULT(vkCreateInstance(&createInfo, nullptr, &instance));
	return instance != nullptr;
}
bool Core::InitVkMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = nullptr,
		.flags = 0,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
		.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)DebugCallback,
		.pUserData = nullptr
	};
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (!func) {
		std::cerr << "Vulkan: Failed to get create debug messenger function" << std::endl;
		return false;
	}
	CHECK_VK_RESULT(func(instance, &createInfo, nullptr, &messenger));
	return messenger != nullptr;
}
bool Core::InitVkDevice()
{
	uint32_t physicalDevicesCount = 0;
	CHECK_VK_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr));
	std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
	CHECK_VK_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, physicalDevices.data()));
	if (physicalDevices.empty()) {
		std::cerr << "Vulkan: Failed to get physical devices" << std::endl;
		return false;
	}

	// Select physical device
	{
		uint32_t bestScore = 0;
		for (const auto &currentPhysicalDevice : physicalDevices) {
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(currentPhysicalDevice, &properties);

			uint32_t score;

			switch (properties.deviceType)
			{
			default:
				continue;
			case VK_PHYSICAL_DEVICE_TYPE_OTHER: score = 1; break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score = 4; break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score = 5; break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score = 3; break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU: score = 2; break;
			}

			if (score > bestScore)
			{
				physicalDevice = currentPhysicalDevice;
				bestScore = score;
			}
		}

		if (!bestScore) {
			std::cerr << "Vulkan: Failed to select physical device" << std::endl;
			return false;
		}
	}
	

	// Create logical device
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	for (const auto &currentQueueFamily : queueFamilies)
	{
		VkBool32 present = vkGetPhysicalDeviceWaylandPresentationSupportKHR(physicalDevice, i, display);
		if (present && (currentQueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			queueFamilyIndex = i;
			break;
		}
		i++;
	}
	
	float priority = 1;
	VkDeviceQueueCreateInfo queueCreateInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = queueFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};
	VkDeviceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = deviceExtensionCount,
		.ppEnabledExtensionNames = deviceExtensionNames,
		.pEnabledFeatures = nullptr
	};
	uint32_t layerPropertyCount = 0;
	CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(physicalDevice, &layerPropertyCount, nullptr));
	std::vector<VkLayerProperties> layerProperties(layerPropertyCount);
	CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(physicalDevice, &layerPropertyCount, layerProperties.data()));

	std::size_t foundLayers = 0;
	for (const auto &currentLayerProperty : layerProperties)
	{
		for (const auto &layerName : layerNames)
		{
			if ((std::string_view)currentLayerProperty.layerName == layerName)
			{
				foundLayers++;
			}
		}
	}

	if (foundLayers >= layerCount)
	{
		createInfo.enabledLayerCount = layerCount;
		createInfo.ppEnabledLayerNames = layerNames;
	}

	CHECK_VK_RESULT(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));

	return device != nullptr;
}
