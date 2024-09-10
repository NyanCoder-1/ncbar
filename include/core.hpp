#pragma once

#include "vulkanInclude.hpp"
#include "wlr-layer-shell-unstable-v1-wrapper.hpp"
#include <wayland-client.h>
#include <xdg-shell.h>
#include <cstdint>
#include <functional>
#include <memory>

class Core : public std::enable_shared_from_this<Core>
{
	friend class Window;
	friend class Renderer;
	struct Private { explicit Private() = default; };
public:
	typedef std::shared_ptr<Core> Ptr;
	struct WlRegistryListenerWrapper {
		std::function<void(wl_registry *registry, uint32_t name, const char* interface, uint32_t version)> onGlobal;
		~WlRegistryListenerWrapper() {
			onGlobal = nullptr;
		}
	};
	struct XdgWmBaseListenerWrapper {
		std::function<void(xdg_wm_base *shell, uint32_t serial)> onPing;
		~XdgWmBaseListenerWrapper() {
			onPing = nullptr;
		}
	};

	Core() = delete;
	Core(const Core::Private&);
	~Core();
	static Core::Ptr Create()
	{
		auto ptr = std::make_shared<Core>(Private());
		if (!ptr->Init())
			return nullptr;
		return ptr;
	}

	wl_display* GetDisplay() { return display; }
	wl_compositor* GetCompositor() { return compositor; }
	zwlr_layer_shell_v1* GetLayerShell() { return layerShell; }
	xdg_wm_base* GetXdgWmBase() { return shell; }

	VkInstance GetInstance() const { return instance; }
	VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
	VkDevice GetDevice() const { return device; }
	uint32_t GetQueueFamilyIndex() const { return queueFamilyIndex; }

	bool IsVulkanInitialized() const { return vulkanInitialized; }

private:
	bool Init();

	// Wayland
	wl_display *display = nullptr;
	wl_registry *registry = nullptr;
	wl_compositor *compositor = nullptr;
	zwlr_layer_shell_v1 *layerShell = nullptr;
	xdg_wm_base *shell = nullptr;
	std::unique_ptr<WlRegistryListenerWrapper> wlRegistryListenerWrapper;
	std::unique_ptr<XdgWmBaseListenerWrapper> xdgWmBaseListenerWrapper;

	void TryInitVulkan();
	bool InitVkInstance();
	bool InitVkMessenger();
	bool InitVkDevice();

	// Vulkan
	VkInstance instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	uint32_t queueFamilyIndex = 0;
	bool vulkanInitialized = false;
};
