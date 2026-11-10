#ifndef GRAPHICS_VULKAN_LOADER_H
#define GRAPHICS_VULKAN_LOADER_H

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>

struct vulkan_loader_global {
    void *vulkan_library;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

    // global commands
    PFN_vkCreateInstance vkCreateInstance;
    PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
};
struct vulkan_loader_instance {
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT; // optional
    PFN_vkCreateDevice vkCreateDevice;
    PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT; // optional
    PFN_vkDestroyInstance vkDestroyInstance;
    PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR;
};
struct vulkan_loader_device {
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
    PFN_vkDestroyDevice vkDestroyDevice;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
    PFN_vkGetDeviceQueue vkGetDeviceQueue;
};

#endif