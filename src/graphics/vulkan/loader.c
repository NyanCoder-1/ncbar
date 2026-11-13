#include "graphics/vulkan/loader.h"
#include "graphics/vulkan/context.h"
#include <dlfcn.h>
#include <string.h>

enum ncbar_vulkan_error vulkan_context_init_loader_global(struct vulkan_loader_global *loader) {
    loader->vulkan_library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (loader->vulkan_library == NULL) {
        return NCBAR_VULKAN_DLOPEN_FAILED;
    }

    *(void **)(&loader->vkGetInstanceProcAddr) = dlsym(loader->vulkan_library, "vkGetInstanceProcAddr");
    if (loader->vkGetInstanceProcAddr == NULL) {
        return NCBAR_VULKAN_LOADER_GLOBAL_INIT_FAILED;
    }
    loader->vkCreateInstance = (PFN_vkCreateInstance)loader->vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    loader->vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)loader->vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties");
    if (loader->vkCreateInstance == NULL || loader->vkEnumerateInstanceLayerProperties == NULL) {
        return NCBAR_VULKAN_LOADER_GLOBAL_INIT_FAILED;
    }

    return NCBAR_VULKAN_OK;
}
enum ncbar_vulkan_error vulkan_context_init_loader_instance(VkInstance instance, PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, struct vulkan_loader_instance *loader) {
    loader->vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"); // optional
    loader->vkCreateDevice = (PFN_vkCreateDevice)vkGetInstanceProcAddr(instance, "vkCreateDevice");
    loader->vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");
    loader->vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"); // optional
    loader->vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    loader->vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
    loader->vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
    loader->vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
    loader->vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
    loader->vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    loader->vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    loader->vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    loader->vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    loader->vkGetPhysicalDeviceWaylandPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
    if (loader->vkCreateWaylandSurfaceKHR == NULL || loader->vkDestroyInstance == NULL || loader->vkDestroySurfaceKHR == NULL ||
        loader->vkEnumerateDeviceExtensionProperties == NULL || loader->vkEnumeratePhysicalDevices == NULL || loader->vkGetDeviceProcAddr == NULL ||
        loader->vkCreateDevice == NULL || loader->vkGetPhysicalDeviceQueueFamilyProperties == NULL || loader->vkGetPhysicalDeviceSurfaceCapabilitiesKHR == NULL ||
        loader->vkGetPhysicalDeviceSurfaceFormatsKHR == NULL || loader->vkGetPhysicalDeviceSurfacePresentModesKHR == NULL ||
        loader->vkGetPhysicalDeviceWaylandPresentationSupportKHR == NULL) {
        return NCBAR_VULKAN_LOADER_INSTANCE_INIT_FAILED;
    }

    return NCBAR_VULKAN_OK;
}
enum ncbar_vulkan_error vulkan_context_init_loader_device(VkDevice device, PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, struct vulkan_loader_device *loader) {
    loader->vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr(device, "vkCreateSwapchainKHR");
    loader->vkDestroyDevice = (PFN_vkDestroyDevice)vkGetDeviceProcAddr(device, "vkDestroyDevice");
    loader->vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)vkGetDeviceProcAddr(device, "vkDestroySwapchainKHR");
    loader->vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)vkGetDeviceProcAddr(device, "vkDeviceWaitIdle");
    loader->vkGetDeviceQueue = (PFN_vkGetDeviceQueue)vkGetDeviceProcAddr(device, "vkGetDeviceQueue");
    loader->vkQueuePresentKHR = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(device, "vkQueuePresentKHR");
    loader->vkQueueSubmit = (PFN_vkQueueSubmit)vkGetDeviceProcAddr(device, "vkQueueSubmit");
    if (loader->vkCreateSwapchainKHR == NULL || loader->vkDestroyDevice == NULL || loader->vkDestroySwapchainKHR == NULL || loader->vkDeviceWaitIdle == NULL ||
        loader->vkGetDeviceQueue == NULL || loader->vkQueuePresentKHR == NULL || loader->vkQueueSubmit == NULL) {
        return NCBAR_VULKAN_LOADER_DEVICE_INIT_FAILED;
    }

    return NCBAR_VULKAN_OK;
}

void vulkan_context_destroy_loader(struct vulkan_context *self) {
    if (self == NULL || self->vk_loader_global.vulkan_library == NULL)
        return;
    dlclose(self->vk_loader_global.vulkan_library);
    memset(&self->vk_loader_device, 0, sizeof(self->vk_loader_device));
    memset(&self->vk_loader_instance, 0, sizeof(self->vk_loader_instance));
    memset(&self->vk_loader_global, 0, sizeof(self->vk_loader_global));
}