#include "renderer.h"
#include "stb_ds.h"
#include "vk-wrapper/debug-utils.h"
#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan_core.h>
#include <wayland-client-core.h>

// struct QueueFamilies {
//     uint32_t graphics;
//     uint32_t present;
//     uint32_t graphicsFound : 1;
//     uint32_t presentFound : 1;
// };

static bool initializeVulkanLoader(VkWrapper *wrapper, void *vulkanLibrary);
static bool initializeVulkanLoaderForInstance(VkWrapper *wrapper,
                                              VkInstance instance);
static bool initializeVulkanLoaderForDevice(VkWrapper *wrapper,
                                            VkDevice device);
static VkResult createInstance(VkWrapper *wrapper, VkInstance *instance);
static void destroyInstance(VkWrapper *wrapper, VkInstance *instance);
static VkResult setupDebugMessenger(VkWrapper *wrapper, VkInstance instance,
                                    VkDebugUtilsMessengerEXT *debugMessenger);
static VkResult pickPhysicalDevice(VkWrapper *wrapper, VkInstance instance,
                                   VkPhysicalDevice *physicalDevice,
                                   uint32_t *graphicsFamily,
                                   uint32_t *graphicsQueueMode,
                                   struct wl_display *wlDisplay);
static VkResult createLogicalDevice(VkWrapper *wrapper,
                                    VkPhysicalDevice physicalDevice,
                                    uint32_t graphicsFamily, VkDevice *device);
static VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkWrapper *wrapper,
                                                       VkPhysicalDevice device,
                                                       VkSurfaceKHR surface);
static VkSurfaceFormatKHR getBestSurfaceFormat(VkWrapper *wrapper,
                                               VkPhysicalDevice device,
                                               VkSurfaceKHR surface);
static VkPresentModeKHR getBestPresentMode(VkWrapper *wrapper,
                                           VkPhysicalDevice device,
                                           VkSurfaceKHR surface);
static VkExtent2D
getBestSwapchainExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities,
                       uint32_t windowWidth, uint32_t windowHeight);

static bool checkValidationLayerSupport(VkWrapper *wrapper);
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);
static bool isDeviceSuitable(VkWrapper *wrapper, VkPhysicalDevice device,
                             uint32_t *graphicsFamily,
                             uint32_t *graphicsQueueMode,
                             struct wl_display *wlDisplay);
static uint32_t findQueueFamily(VkWrapper *wrapper, VkPhysicalDevice device,
                                uint32_t *graphicsQueueMode,
                                struct wl_display *wlDisplay);
static bool checkDeviceExtensionSupport(VkWrapper *wrapper,
                                        VkPhysicalDevice device);

static const char *const vkInstanceExtensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#ifndef NDEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};
static const uint32_t vkInstanceExtensionsCount =
    sizeof(vkInstanceExtensions) / sizeof(vkInstanceExtensions[0]);
static const char *const vkDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};
static const uint32_t vkDeviceExtensionsCount =
    sizeof(vkDeviceExtensions) / sizeof(vkDeviceExtensions[0]);
static const char *const vkValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};
static const uint32_t vkValidationLayersCount =
    sizeof(vkValidationLayers) / sizeof(vkValidationLayers[0]);

Renderer Renderer_Create(struct wl_display *wlDisplay) {
    void *vulkanLibrary = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (vulkanLibrary == NULL) {
        return (Renderer){};
    }
    VkWrapper wrapper = {0};
    memset(&wrapper, 0, sizeof(VkWrapper));
    if (!initializeVulkanLoader(&wrapper, vulkanLibrary)) {
        dlclose(vulkanLibrary);
        return (Renderer){};
    }

    VkInstance instance = VK_NULL_HANDLE;
    if (createInstance(&wrapper, &instance) != VK_SUCCESS) {
        dlclose(vulkanLibrary);
        return (Renderer){};
    }
    if (!initializeVulkanLoaderForInstance(&wrapper, instance)) {
        wrapper.vkDestroyInstance(instance, NULL);
        dlclose(vulkanLibrary);
        return (Renderer){};
    }
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    if (setupDebugMessenger(&wrapper, instance, &debugMessenger) !=
        VK_SUCCESS) {
        wrapper.vkDestroyInstance(instance, NULL);
        dlclose(vulkanLibrary);
        return (Renderer){};
    }
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    uint32_t graphicsQueueMode = 0;
    if (pickPhysicalDevice(&wrapper, instance, &physicalDevice,
                           &graphicsQueueFamily, &graphicsQueueMode,
                           wlDisplay) != VK_SUCCESS) {
        DestroyDebugUtilsMessengerEXT(wrapper.vkGetInstanceProcAddr, instance,
                                      debugMessenger, NULL);
        wrapper.vkDestroyInstance(instance, NULL);
        dlclose(vulkanLibrary);
        return (Renderer){};
    }
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    if (createLogicalDevice(&wrapper, physicalDevice, graphicsQueueFamily,
                            &device) != VK_SUCCESS) {
        DestroyDebugUtilsMessengerEXT(wrapper.vkGetInstanceProcAddr, instance,
                                      debugMessenger, NULL);
        wrapper.vkDestroyInstance(instance, NULL);
        dlclose(vulkanLibrary);
        return (Renderer){};
    }
    if (!initializeVulkanLoaderForDevice(&wrapper, device)) {
        wrapper.vkDestroyDevice(device, NULL);
        DestroyDebugUtilsMessengerEXT(wrapper.vkGetInstanceProcAddr, instance,
                                      debugMessenger, NULL);
        wrapper.vkDestroyInstance(instance, NULL);
        dlclose(vulkanLibrary);
        return (Renderer){};
    }

    wrapper.vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);

    Renderer renderer = {0};
    renderer.success = 1;
    renderer.instance = instance;
    renderer.debugMessenger = debugMessenger;
    renderer.physicalDevice = physicalDevice;
    renderer.graphicsQueueFamily = graphicsQueueFamily;
    renderer.graphicsQueueMode = graphicsQueueMode;
    renderer.device = device;
    renderer.graphicsQueue = graphicsQueue;
    renderer.vkWrapper = wrapper;
    return renderer;
}
void Renderer_Destroy(Renderer *renderer) {
    if (renderer == NULL)
        return;
    if (renderer->device != VK_NULL_HANDLE) {
        renderer->vkWrapper.vkDestroyDevice(renderer->device, NULL);
    }
    if (renderer->instance != VK_NULL_HANDLE) {
        if (renderer->debugMessenger != VK_NULL_HANDLE) {
            DestroyDebugUtilsMessengerEXT(
                renderer->vkWrapper.vkGetInstanceProcAddr, renderer->instance,
                renderer->debugMessenger, NULL);
        }
        renderer->vkWrapper.vkDestroyInstance(renderer->instance, NULL);
    }
    if (renderer->vkWrapper.vulkanLibrary) {
        dlclose(renderer->vkWrapper.vulkanLibrary);
    }
    memset(renderer, 0, sizeof(Renderer));
}
bool Renderer_CreateSurface(Renderer *self, VkSurfaceKHR *vkSurface,
                            struct wl_display *wlDisplay,
                            struct wl_surface *wlSurface) {
    VkWaylandSurfaceCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = wlDisplay;
    createInfo.surface = wlSurface;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult result = self->vkWrapper.vkCreateWaylandSurfaceKHR(
        self->instance, &createInfo, NULL, &surface);
    if (result != VK_SUCCESS) {
        fprintf(stderr,
                "Error: couldn't create vulkan surface for wl_surface\n");
        return false;
    }
    *vkSurface = surface;
    return true;
}
bool Renderer_CreateSwapChain(Renderer *self, VkSwapchainKHR *vkSwapChain,
                              VkSurfaceKHR vkSurface, uint32_t width,
                              uint32_t height) {
    if (self == NULL || vkSwapChain == NULL || vkSurface == VK_NULL_HANDLE ||
        width == 0 || height == 0) {
        return false;
    }
    VkSurfaceCapabilitiesKHR surfaceCapabilities = getSurfaceCapabilities(
        &self->vkWrapper, self->physicalDevice, vkSurface);
    VkSurfaceFormatKHR bestSurfaceFormat =
        getBestSurfaceFormat(&self->vkWrapper, self->physicalDevice, vkSurface);
    VkPresentModeKHR bestPresentMode =
        getBestPresentMode(&self->vkWrapper, self->physicalDevice, vkSurface);
    VkExtent2D bestSwapchainExtent =
        getBestSwapchainExtent(surfaceCapabilities, width, height);
    uint32_t imageArrayLayers = 1;

    VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    uint32_t queueFamilyIndexCount = 0, *pQueueFamilyIndices = NULL;
    uint32_t queueFamilyIndices[] = {0, 1};
    if (self->graphicsQueueMode == 1) {
        imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        queueFamilyIndexCount = 2;
        pQueueFamilyIndices = queueFamilyIndices;
    }

    VkSwapchainKHR swapChainOld = *vkSwapChain;
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vkSurface,
        .minImageCount = surfaceCapabilities.minImageCount + 1,
        .imageFormat = bestSurfaceFormat.format,
        .imageColorSpace = bestSurfaceFormat.colorSpace,
        .imageExtent = bestSwapchainExtent,
        .imageArrayLayers = imageArrayLayers,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = imageSharingMode,
        .queueFamilyIndexCount = queueFamilyIndexCount,
        .pQueueFamilyIndices = pQueueFamilyIndices,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = surfaceCapabilities.supportedCompositeAlpha &
                                  VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
                              ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
                              : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = bestPresentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = swapChainOld};
    if (swapChainOld != VK_NULL_HANDLE) {
        arrput(self->retireQueue, swapChainOld);
    }

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    if (self->vkWrapper.vkCreateSwapchainKHR(self->device, &swapchainCreateInfo,
                                             VK_NULL_HANDLE,
                                             &swapchain) != VK_SUCCESS) {
        *vkSwapChain = VK_NULL_HANDLE;
        return false;
    }
    *vkSwapChain = swapchain;
    return true;
}

static bool initializeVulkanLoader(VkWrapper *wrapper, void *vulkanLibrary) {
    if (wrapper == NULL || vulkanLibrary == NULL)
        return false;
    VkWrapper vkWrapper = {0};
    vkWrapper.vulkanLibrary = vulkanLibrary;
    vkWrapper.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(
        vulkanLibrary, "vkGetInstanceProcAddr");
    if (vkWrapper.vkGetInstanceProcAddr == NULL) {
        return false;
    }
    vkWrapper.vkCreateInstance =
        (PFN_vkCreateInstance)vkWrapper.vkGetInstanceProcAddr(
            VK_NULL_HANDLE, "vkCreateInstance");
    vkWrapper.vkEnumerateInstanceLayerProperties =
        (PFN_vkEnumerateInstanceLayerProperties)vkWrapper.vkGetInstanceProcAddr(
            VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties");
    if (vkWrapper.vkCreateInstance == NULL ||
        vkWrapper.vkEnumerateInstanceLayerProperties == NULL) {
        return false;
    }
    *wrapper = vkWrapper;
    return true;
}
static bool initializeVulkanLoaderForInstance(VkWrapper *wrapper,
                                              VkInstance instance) {
    if (wrapper == NULL || instance == VK_NULL_HANDLE)
        return false;
    wrapper->vkCreateWaylandSurfaceKHR =
        (PFN_vkCreateWaylandSurfaceKHR)wrapper->vkGetInstanceProcAddr(
            instance, "vkCreateWaylandSurfaceKHR");
    wrapper->vkDestroyInstance =
        (PFN_vkDestroyInstance)wrapper->vkGetInstanceProcAddr(
            instance, "vkDestroyInstance");
    wrapper->vkDestroySurfaceKHR =
        (PFN_vkDestroySurfaceKHR)wrapper->vkGetInstanceProcAddr(
            instance, "vkDestroySurfaceKHR");
    wrapper->vkEnumerateDeviceExtensionProperties =
        (PFN_vkEnumerateDeviceExtensionProperties)
            wrapper->vkGetInstanceProcAddr(
                instance, "vkEnumerateDeviceExtensionProperties");
    wrapper->vkEnumeratePhysicalDevices =
        (PFN_vkEnumeratePhysicalDevices)wrapper->vkGetInstanceProcAddr(
            instance, "vkEnumeratePhysicalDevices");
    wrapper->vkGetDeviceProcAddr =
        (PFN_vkGetDeviceProcAddr)wrapper->vkGetInstanceProcAddr(
            instance, "vkGetDeviceProcAddr");
    wrapper->vkCreateDevice =
        (PFN_vkCreateDevice)wrapper->vkGetInstanceProcAddr(instance,
                                                           "vkCreateDevice");
    wrapper->vkGetPhysicalDeviceQueueFamilyProperties =
        (PFN_vkGetPhysicalDeviceQueueFamilyProperties)
            wrapper->vkGetInstanceProcAddr(
                instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    wrapper->vkGetPhysicalDeviceSurfaceCapabilitiesKHR =
        (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
            wrapper->vkGetInstanceProcAddr(
                instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    wrapper->vkGetPhysicalDeviceSurfaceFormatsKHR =
        (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)
            wrapper->vkGetInstanceProcAddr(
                instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    wrapper->vkGetPhysicalDeviceSurfacePresentModesKHR =
        (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)
            wrapper->vkGetInstanceProcAddr(
                instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    wrapper->vkGetPhysicalDeviceWaylandPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)
            wrapper->vkGetInstanceProcAddr(
                instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
    if (wrapper->vkCreateWaylandSurfaceKHR == NULL ||
        wrapper->vkDestroyInstance == NULL ||
        wrapper->vkDestroySurfaceKHR == NULL ||
        wrapper->vkEnumerateDeviceExtensionProperties == NULL ||
        wrapper->vkEnumeratePhysicalDevices == NULL ||
        wrapper->vkGetDeviceProcAddr == NULL ||
        wrapper->vkCreateDevice == NULL ||
        wrapper->vkGetPhysicalDeviceQueueFamilyProperties == NULL ||
        wrapper->vkGetPhysicalDeviceSurfaceCapabilitiesKHR == NULL ||
        wrapper->vkGetPhysicalDeviceSurfaceFormatsKHR == NULL ||
        wrapper->vkGetPhysicalDeviceSurfacePresentModesKHR == NULL ||
        wrapper->vkGetPhysicalDeviceWaylandPresentationSupportKHR == NULL) {
        return false;
    }
    return true;
}
static bool initializeVulkanLoaderForDevice(VkWrapper *wrapper,
                                            VkDevice device) {
    if (wrapper == NULL || device == VK_NULL_HANDLE)
        return false;
    wrapper->vkCreateSwapchainKHR =
        (PFN_vkCreateSwapchainKHR)wrapper->vkGetDeviceProcAddr(
            device, "vkCreateSwapchainKHR");
    wrapper->vkDestroyDevice =
        (PFN_vkDestroyDevice)wrapper->vkGetDeviceProcAddr(device,
                                                          "vkDestroyDevice");
    wrapper->vkDestroySwapchainKHR =
        (PFN_vkDestroySwapchainKHR)wrapper->vkGetDeviceProcAddr(
            device, "vkDestroySwapchainKHR");
    wrapper->vkDeviceWaitIdle =
        (PFN_vkDeviceWaitIdle)wrapper->vkGetDeviceProcAddr(device,
                                                           "vkDeviceWaitIdle");
    wrapper->vkGetDeviceQueue =
        (PFN_vkGetDeviceQueue)wrapper->vkGetDeviceProcAddr(device,
                                                           "vkGetDeviceQueue");
    if (wrapper->vkCreateSwapchainKHR == NULL ||
        wrapper->vkDestroyDevice == NULL ||
        wrapper->vkDestroySwapchainKHR == NULL ||
        wrapper->vkDeviceWaitIdle == NULL ||
        wrapper->vkGetDeviceQueue == NULL) {
        return false;
    }
    return true;
}
static VkResult createInstance(VkWrapper *wrapper, VkInstance *instance) {
#ifndef NDEBUG
    if (!checkValidationLayerSupport(wrapper)) {
        fprintf(stderr,
                "Error: validation layers requested, but not available!\n");
        return VK_RESULT_MAX_ENUM;
    }
#endif
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ncbar";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {0};
#ifndef NDEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
    debugCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    debugCreateInfo.pUserData = NULL;
    createInfo.pNext = &debugCreateInfo;
#else
    createInfo.pNext = NULL;
#endif
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.flags = 0U;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = vkInstanceExtensionsCount;
    createInfo.ppEnabledExtensionNames = vkInstanceExtensions;
#ifndef NDEBUG
    createInfo.enabledLayerCount = vkValidationLayersCount;
    createInfo.ppEnabledLayerNames = vkValidationLayers;
#else
    createInfo.enabledLayerCount = 0;
#endif

    return wrapper->vkCreateInstance(&createInfo, NULL, instance);
}
static VkResult setupDebugMessenger(VkWrapper *wrapper, VkInstance instance,
                                    VkDebugUtilsMessengerEXT *debugMessenger) {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = NULL;
    return CreateDebugUtilsMessengerEXT(wrapper->vkGetInstanceProcAddr,
                                        instance, &createInfo, NULL,
                                        debugMessenger);
}
static VkResult pickPhysicalDevice(VkWrapper *wrapper, VkInstance instance,
                                   VkPhysicalDevice *physicalDevice,
                                   uint32_t *graphicsFamily,
                                   uint32_t *pGraphicsQueueMode,
                                   struct wl_display *wlDisplay) {
    uint32_t deviceCount = 0;
    wrapper->vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        fprintf(stderr, "Error: failed to find GPUs with Vulkan support!\n");
        return VK_RESULT_MAX_ENUM;
    }
    VkPhysicalDevice devices[deviceCount];
    wrapper->vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    VkPhysicalDevice result = VK_NULL_HANDLE;
    uint32_t graphicsFamilyIndex = 0;
    uint32_t graphicsQueueMode = 0;
    for (uint32_t deviceNum = 0; deviceNum < deviceCount; deviceNum++) {
        const VkPhysicalDevice device = devices[deviceNum];

        uint32_t graphicsFamilyIndexCurrent = 0;
        uint32_t graphicsQueueModeCurrent = 0;
        if (isDeviceSuitable(wrapper, device, &graphicsFamilyIndexCurrent,
                             &graphicsQueueModeCurrent, wlDisplay)) {
            result = device;
            graphicsFamilyIndex = graphicsFamilyIndexCurrent;
            graphicsQueueMode = graphicsQueueModeCurrent;
            break;
        }
    }

    if (result == VK_NULL_HANDLE) {
        fprintf(stderr, "Error: failed to find a suitable GPU!\n");
        return VK_RESULT_MAX_ENUM;
    }

    *physicalDevice = result;
    *graphicsFamily = graphicsFamilyIndex;
    *pGraphicsQueueMode = graphicsQueueMode;
    return VK_SUCCESS;
}
static VkResult createLogicalDevice(VkWrapper *wrapper,
                                    VkPhysicalDevice physicalDevice,
                                    uint32_t graphicsFamily, VkDevice *device) {
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamily;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {0};

    VkDeviceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = (VkDeviceQueueCreateInfo[]){queueCreateInfo};
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = vkDeviceExtensionsCount;
    createInfo.ppEnabledExtensionNames = vkDeviceExtensions;
    createInfo.enabledLayerCount = 0;
    if (wrapper->vkCreateDevice(physicalDevice, &createInfo, NULL, device) !=
        VK_SUCCESS) {
        fprintf(stderr, "Error: failed to create logical device!\n");
        return VK_RESULT_MAX_ENUM;
    }

    return VK_SUCCESS;
}
static VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkWrapper *wrapper,
                                                       VkPhysicalDevice device,
                                                       VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {0};
    wrapper->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                                       &surfaceCapabilities);
    return surfaceCapabilities;
}
static VkSurfaceFormatKHR getBestSurfaceFormat(VkWrapper *wrapper,
                                               VkPhysicalDevice device,
                                               VkSurfaceKHR surface) {
    uint32_t surfaceFormatNumber = 0;
    wrapper->vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, surface, &surfaceFormatNumber, VK_NULL_HANDLE);
    VkSurfaceFormatKHR surfaceFormats[surfaceFormatNumber] = {};
    wrapper->vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, surface, &surfaceFormatNumber, surfaceFormats);
    VkSurfaceFormatKHR bestSurfaceFormat = surfaceFormats[0];

    return bestSurfaceFormat;
}
static VkPresentModeKHR getBestPresentMode(VkWrapper *wrapper,
                                           VkPhysicalDevice device,
                                           VkSurfaceKHR surface) {
    uint32_t presentModeNumber = 0;
    wrapper->vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeNumber, VK_NULL_HANDLE);
    VkPresentModeKHR presentModes[presentModeNumber] = {};
    wrapper->vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeNumber, presentModes);
    VkPresentModeKHR bestPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < presentModeNumber; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            bestPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
    return bestPresentMode;
}
static VkExtent2D
getBestSwapchainExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities,
                       uint32_t windowWidth, uint32_t windowHeight) {
    return (VkExtent2D){
        .width = surfaceCapabilities.currentExtent.width > windowWidth
                     ? windowWidth
                     : surfaceCapabilities.currentExtent.width,
        .height = surfaceCapabilities.currentExtent.height > windowHeight
                      ? windowHeight
                      : surfaceCapabilities.currentExtent.height,
    };
}

static bool checkValidationLayerSupport(VkWrapper *wrapper) {
    uint32_t layerCount;
    wrapper->vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties availableLayers[layerCount];
    wrapper->vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (uint32_t vkValidationLayerNum = 0;
         vkValidationLayerNum < vkValidationLayersCount;
         vkValidationLayerNum++) {
        const char *const layerName = vkValidationLayers[vkValidationLayerNum];
        uint8_t layerFound = false;

        for (uint32_t availableLayersNum = 0; availableLayersNum < layerCount;
             availableLayersNum++) {
            const VkLayerProperties layerProperties =
                availableLayers[availableLayersNum];
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);
    }
    return VK_FALSE;
}
static bool isDeviceSuitable(VkWrapper *wrapper, VkPhysicalDevice device,
                             uint32_t *graphicsFamily,
                             uint32_t *graphicsQueueMode,
                             struct wl_display *wlDisplay) {
    const uint32_t queueFamilyResult =
        findQueueFamily(wrapper, device, graphicsQueueMode, wlDisplay);
    const bool extensionsSupported =
        checkDeviceExtensionSupport(wrapper, device);
    if (queueFamilyResult != UINT32_MAX && extensionsSupported) {
        *graphicsFamily = queueFamilyResult;
        return true;
    }
    return false;
}
static uint32_t findQueueFamily(VkWrapper *wrapper, VkPhysicalDevice device,
                                uint32_t *graphicsQueueMode,
                                struct wl_display *wlDisplay) {
    uint32_t queueFamilyCount = 0;
    wrapper->vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                      NULL);

    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    wrapper->vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                      queueFamilies);

    uint32_t result = UINT32_MAX;
    uint32_t queueMode = 0;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        VkBool32 presentSupport =
            wrapper->vkGetPhysicalDeviceWaylandPresentationSupportKHR(
                device, i, wlDisplay);
        const VkQueueFamilyProperties queueFamily = queueFamilies[i];
        if (presentSupport != VK_FALSE &&
            (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            result = i;
            queueMode = queueFamily.queueCount == 1  ? 0
                        : queueFamily.queueCount > 1 ? 1
                                                     : 2;
        }
    }

    *graphicsQueueMode = queueMode;
    return result;
}
static bool checkDeviceExtensionSupport(VkWrapper *wrapper,
                                        VkPhysicalDevice device) {
    uint32_t extensionCount = 0;
    wrapper->vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount,
                                                  NULL);

    VkExtensionProperties availableExtensions[extensionCount];
    wrapper->vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount,
                                                  availableExtensions);

    bool vkDeviceExtensionMet[vkDeviceExtensionsCount];
    memset(vkDeviceExtensionMet, 0, sizeof(vkDeviceExtensionMet));

    bool result = false;

    for (uint32_t i = 0; i < extensionCount && !result; i++) {
        const char *availableExtensionName =
            availableExtensions[i].extensionName;
        uint32_t extensionsMetCount = 0;
        for (uint32_t j = 0; j < vkDeviceExtensionsCount; j++) {
            const char *requiredExtension = vkDeviceExtensions[j];
            if (strcmp(availableExtensionName, requiredExtension) == 0) {
                vkDeviceExtensionMet[j] = true;
            }
            if (vkDeviceExtensionMet[j]) {
                extensionsMetCount++;
            }
        }
        result = extensionsMetCount == vkDeviceExtensionsCount;
    }

    return result;
}