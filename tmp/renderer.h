#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

typedef struct VkWrapper {
    void *vulkanLibrary;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

    // global commands
    PFN_vkCreateInstance vkCreateInstance;
    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
    PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
    // vkInstance
    PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
    PFN_vkDestroyInstance vkDestroyInstance;
    PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
    // vkPhysicalDevice
    PFN_vkCreateDevice vkCreateDevice;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR;
    // vkDevice
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
    PFN_vkDestroyDevice vkDestroyDevice;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
    PFN_vkGetDeviceQueue vkGetDeviceQueue;
} VkWrapper;

typedef struct Renderer {
    uint8_t success;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    uint32_t graphicsQueueFamily;
    uint32_t graphicsQueueMode;
    VkQueue graphicsQueue;

    VkSwapchainKHR *retireQueue;

    VkWrapper vkWrapper;
} Renderer;

Renderer Renderer_Create(struct wl_display *wlDisplay);
void Renderer_Destroy(Renderer *renderer);
bool Renderer_CreateSurface(Renderer *self, VkSurfaceKHR *vkSurface, struct wl_display *wlDisplay, struct wl_surface *wlSurface);
bool Renderer_CreateSwapChain(Renderer *self, VkSwapchainKHR *vkSwapChain, VkSurfaceKHR vkSurface, uint32_t width, uint32_t height);

#endif