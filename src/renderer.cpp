#include "globals.hpp"
#include "core.hpp"
#include "renderer.hpp"
#include "vulkanHelper.hpp"
#include "window.hpp"
#include <cstddef>
#include <iostream>
#include <vector>

Renderer::~Renderer()
{
	CHECK_VK_RESULT(vkDeviceWaitIdle(core->GetDevice()));
	DestroySwapchain();
	if (commandPool) {
		vkDestroyCommandPool(core->GetDevice(), commandPool, nullptr);
		commandPool = nullptr;
	}
	if (surface) {
		vkDestroySurfaceKHR(core->GetInstance(), surface, nullptr);
		surface = nullptr;
	}
}

bool Renderer::Init(WindowPtr window)
{
	core = window->core;
	windowWeak = window;

	if (!InitSurface(window)) {
		std::cerr << "Vulkan: Failed to get create wayland surface" << std::endl;
		return false;
	}

	InitGraphicsQueue(window);
	if (!InitCommandPool()) {
		std::cerr << "Vulkan: Failed to create command pool" << std::endl;
		return false;
	}
	if (!InitSwapchain()) {
		std::cerr << "Vulkan: Failed to create swapchain" << std::endl;
		return false;
	}

	return true;
}

void Renderer::InitGraphicsQueue(WindowPtr window)
{
	(void)window;
	vkGetDeviceQueue(core->GetDevice(), core->GetQueueFamilyIndex(), 0, &graphicsQueue);
}
bool Renderer::InitSurface(WindowPtr window)
{
	VkWaylandSurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.display = core->GetDisplay(),
		.surface = window->GetSurface()
	};
	CHECK_VK_RESULT(vkCreateWaylandSurfaceKHR(core->GetInstance(), &createInfo, nullptr, &surface));

	return surface != nullptr;
}
bool Renderer::InitCommandPool()
{
	VkCommandPoolCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = core->GetQueueFamilyIndex()
	};
	CHECK_VK_RESULT(vkCreateCommandPool(core->GetDevice(), &createInfo, nullptr, &commandPool));

	return commandPool != nullptr;
}

bool Renderer::InitSwapchain()
{
	VkFormat format = VK_FORMAT_UNDEFINED;
	int32_t width = 1920;
	int32_t height = 1080;

	{
		VkSurfaceCapabilitiesKHR capabilities;
		CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(core->GetPhysicalDevice(), surface, &capabilities));
		uint32_t formatsCount;
		CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(core->GetPhysicalDevice(), surface, &formatsCount, nullptr));
		std::vector<VkSurfaceFormatKHR> formats(formatsCount);
		CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(core->GetPhysicalDevice(), surface, &formatsCount, formats.data()));
		auto chosenFormat = formats[0];
		for (auto &currentFormat : formats) {
			if (currentFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
				chosenFormat = currentFormat;
				break;
			}
		}

		format = chosenFormat.format;

		framesCount = (capabilities.minImageCount + 1) < capabilities.maxImageCount ? capabilities.minImageCount + 1 : capabilities.maxImageCount;

		width = ~capabilities.currentExtent.width ? capabilities.currentExtent.width : capabilities.maxImageExtent.width;
		height = ~capabilities.currentExtent.height ? capabilities.currentExtent.height : capabilities.maxImageExtent.height;
		if (auto window = GetWindow(); window && window->GetWidth() && window->GetHeight()) {
			width = window->GetWidth();
			height = window->GetHeight();
		}

		VkSwapchainCreateInfoKHR createInfo = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = surface,
			.minImageCount = framesCount,
			.imageFormat = chosenFormat.format,
			.imageColorSpace = chosenFormat.colorSpace,
			.imageExtent = VkExtent2D{ .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height) },
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.preTransform = capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			.presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE
		};
		CHECK_VK_RESULT(vkCreateSwapchainKHR(core->GetDevice(), &createInfo, nullptr, &swapchain));
	}

	{
		VkAttachmentDescription attachments = {
			.flags = 0,
			.format = format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};
		VkAttachmentReference attachmentReference = {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		VkSubpassDescription subpass = {
			.flags = 0,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount = 0,
			.pInputAttachments = nullptr,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentReference,
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = nullptr,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr
		};
		VkRenderPassCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.attachmentCount = 1,
			.pAttachments = &attachments,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 0,
			.pDependencies = nullptr
		};
		CHECK_VK_RESULT(vkCreateRenderPass(core->GetDevice(), &createInfo, nullptr, &renderPass));
	}

	CHECK_VK_RESULT(vkGetSwapchainImagesKHR(core->GetDevice(), swapchain, &framesCount, nullptr));
	std::vector<VkImage> images(framesCount);
	CHECK_VK_RESULT(vkGetSwapchainImagesKHR(core->GetDevice(), swapchain, &framesCount, images.data()));
	swapchainResources.resize(framesCount);

	for (std::size_t i = 0; i < images.size(); i++) {
		auto &currentSwapchainResource = swapchainResources[i];

		VkCommandBufferAllocateInfo cbAllocInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};
		std::vector<VkCommandBuffer> commandBuffers(framesCount);
		CHECK_VK_RESULT(vkAllocateCommandBuffers(core->GetDevice(), &cbAllocInfo, &currentSwapchainResource.commandBuffer));

		currentSwapchainResource.image = images[i];

		VkImageViewCreateInfo ivCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = currentSwapchainResource.image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.components = VkComponentMapping{
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
		CHECK_VK_RESULT(vkCreateImageView(core->GetDevice(), &ivCreateInfo, nullptr, &currentSwapchainResource.imageView));

		VkFramebufferCreateInfo fbCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = renderPass,
			.attachmentCount = 1,
			.pAttachments = &currentSwapchainResource.imageView,
			.width = static_cast<uint32_t>(width),
			.height = static_cast<uint32_t>(height),
			.layers = 1
		};
		CHECK_VK_RESULT(vkCreateFramebuffer(core->GetDevice(), &fbCreateInfo, nullptr, &currentSwapchainResource.framebuffer));

		VkSemaphoreCreateInfo beginSemaphoreCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};
		CHECK_VK_RESULT(vkCreateSemaphore(core->GetDevice(), &beginSemaphoreCreateInfo, nullptr, &currentSwapchainResource.startSemaphore));
		VkSemaphoreCreateInfo endSemaphoreCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};
		CHECK_VK_RESULT(vkCreateSemaphore(core->GetDevice(), &endSemaphoreCreateInfo, nullptr, &currentSwapchainResource.endSemaphore));

		VkFenceCreateInfo fenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};
		CHECK_VK_RESULT(vkCreateFence(core->GetDevice(), &fenceCreateInfo, nullptr, &currentSwapchainResource.fence));

		currentSwapchainResource.lastFence = VK_NULL_HANDLE;
	}

	return true;
}
void Renderer::DestroySwapchain()
{
	for (auto &swapchainResource : swapchainResources) {
		if (swapchainResource.fence) {
			vkDestroyFence(core->GetDevice(), swapchainResource.fence, nullptr);
			swapchainResource.fence = nullptr;
		}
		if (swapchainResource.startSemaphore) {
			vkDestroySemaphore(core->GetDevice(), swapchainResource.startSemaphore, nullptr);
			swapchainResource.startSemaphore = nullptr;
		}
		if (swapchainResource.endSemaphore) {
			vkDestroySemaphore(core->GetDevice(), swapchainResource.endSemaphore, nullptr);
			swapchainResource.endSemaphore = nullptr;
		}
		if (swapchainResource.framebuffer) {
			vkDestroyFramebuffer(core->GetDevice(), swapchainResource.framebuffer, nullptr);
			swapchainResource.framebuffer = nullptr;
		}
		if (swapchainResource.imageView) {
			vkDestroyImageView(core->GetDevice(), swapchainResource.imageView, nullptr);
			swapchainResource.imageView = nullptr;
		}
		if (swapchainResource.commandBuffer) {
			vkFreeCommandBuffers(core->GetDevice(), commandPool, 1, &swapchainResource.commandBuffer);
			swapchainResource.commandBuffer = nullptr;
		}
	}
	swapchainResources.clear();
	if (renderPass) {
		vkDestroyRenderPass(core->GetDevice(), renderPass, nullptr);
		renderPass = nullptr;
	}
	if (swapchain) {
		vkDestroySwapchainKHR(core->GetDevice(), swapchain, nullptr);
		swapchain = nullptr;
	}
}

bool Renderer::Render()
{
	// Wait for previous frame
	auto &currentSwapchainResource = swapchainResources[currentFrame];

	CHECK_VK_RESULT(vkWaitForFences(core->GetDevice(), 1, &currentSwapchainResource.fence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
	VkResult result = vkAcquireNextImageKHR(core->GetDevice(), swapchain, std::numeric_limits<uint64_t>::max(), currentSwapchainResource.startSemaphore, VK_NULL_HANDLE, &nextFrame);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		CHECK_VK_RESULT(vkDeviceWaitIdle(core->GetDevice()));
		return OnResize();
	}
	else if (result < VK_SUCCESS) {
		CHECK_VK_RESULT(result);
	}

	auto &nextSwapchainResource = swapchainResources[nextFrame];
	if (nextSwapchainResource.lastFence) {
		CHECK_VK_RESULT(vkWaitForFences(core->GetDevice(), 1, &nextSwapchainResource.lastFence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
	}
	nextSwapchainResource.lastFence = currentSwapchainResource.fence;

	CHECK_VK_RESULT(vkResetFences(core->GetDevice(), 1, &currentSwapchainResource.fence));
	VkCommandBufferBeginInfo beginInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};
	CHECK_VK_RESULT(vkBeginCommandBuffer(nextSwapchainResource.commandBuffer, &beginInfo));

	// Prepare the current frame
	if (callbackOnPresent) {
		if (!callbackOnPresent(nextFrame, this))
			return false;
	}

	// Present the current frame
	CHECK_VK_RESULT(vkEndCommandBuffer(nextSwapchainResource.commandBuffer));
	const VkPipelineStageFlags waitStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentSwapchainResource.startSemaphore,
		.pWaitDstStageMask = &waitStageFlag,
		.commandBufferCount = 1,
		.pCommandBuffers = &nextSwapchainResource.commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &currentSwapchainResource.endSemaphore
	};
	CHECK_VK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, currentSwapchainResource.fence));
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentSwapchainResource.endSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapchain,
		.pImageIndices = &nextFrame,
		.pResults = nullptr
	};
	result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		if (!OnResize())
			return false;
	}
	else if (result < VK_SUCCESS) {
		CHECK_VK_RESULT(result);
	}

	currentFrame = (currentFrame + 1) % framesCount;

	return true;
}

bool Renderer::OnResize()
{
	DestroySwapchain();
	if (!InitSwapchain())
		return false;

	currentFrame = 0;

	return true;
}

void Renderer::SetOnPresent(OnPresentCallbackType onPresent)
{
	callbackOnPresent = onPresent;
}
