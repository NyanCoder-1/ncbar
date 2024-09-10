#pragma once

#include "rendererHelper.hpp"
#include "vulkanInclude.hpp"
#include <functional>
#include <memory>
#include <vector>

class Core;
class Window;

class Renderer
{
	struct Private { explicit Private() = default; };
	typedef std::shared_ptr<Core> CorePtr;
	typedef std::shared_ptr<Window> WindowPtr;
	typedef std::weak_ptr<Window> WindowWeakPtr;

public:
	typedef std::unique_ptr<Renderer> Ptr;
	struct SwapchainResources {
		VkCommandBuffer commandBuffer = nullptr;
		VkImage image = nullptr;
		VkImageView imageView = nullptr;
		VkFramebuffer framebuffer = nullptr;
		VkSemaphore startSemaphore = nullptr;
		VkSemaphore endSemaphore = nullptr;
		VkFence fence = nullptr;
		VkFence lastFence = nullptr;
	};

	Renderer() = delete;
	Renderer(const Private&) {}
	~Renderer();
	static Renderer::Ptr Create(WindowPtr window)
	{
		if (!window)
			return nullptr;
		auto ptr = std::make_unique<Renderer>(Private());
		if (!ptr->Init(window))
			return nullptr;
		return ptr;
	}

	bool Render();

	bool OnResize();

	void SetOnPresent(OnPresentCallbackType onPresent);

	WindowPtr GetWindow() const { return windowWeak.lock(); }
	VkQueue GetGraphicsQueue() const { return graphicsQueue; }
	VkSurfaceKHR GetSurface() const { return surface; }
	VkCommandPool GetCommandPool() const { return commandPool; }
	VkSwapchainKHR GetSwapchain() const { return swapchain; }
	VkRenderPass GetRenderPass() const { return renderPass; }
	std::vector<Renderer::SwapchainResources> &GetSwapchainResources() { return swapchainResources; }
	VkCommandBuffer GetCurrentFrameCommandBuffer(uint32_t frameIndex) const { return swapchainResources[frameIndex].commandBuffer; }
	VkImage GetCurrentFrameImage(uint32_t frameIndex) const { return swapchainResources[frameIndex].image; }
	VkImageView GetCurrentFrameImageView(uint32_t frameIndex) const { return swapchainResources[frameIndex].imageView; }
	VkFramebuffer GetCurrentFrameFramebuffer(uint32_t frameIndex) const { return swapchainResources[frameIndex].framebuffer; }
	VkSemaphore GetCurrentFrameStartSemaphore(uint32_t frameIndex) const { return swapchainResources[frameIndex].startSemaphore; }
	VkSemaphore GetCurrentFrameEndSemaphore(uint32_t frameIndex) const { return swapchainResources[frameIndex].endSemaphore; }
	VkFence GetCurrentFrameFence(uint32_t frameIndex) const { return swapchainResources[frameIndex].fence; }
	VkFence GetCurrentFrameLastFence(uint32_t frameIndex) const { return swapchainResources[frameIndex].lastFence; }
	uint32_t GetFramesCount() const { return framesCount; }
	uint32_t GetCurrentFrameIndex() const { return currentFrame; }

private:
	bool Init(WindowPtr window);

	CorePtr core;
	WindowWeakPtr windowWeak;

	OnPresentCallbackType callbackOnPresent;

	void InitGraphicsQueue(WindowPtr window);
	bool InitSurface(WindowPtr window);
	bool InitCommandPool();
	bool InitSwapchain();
	void DestroySwapchain();

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	std::vector<Renderer::SwapchainResources> swapchainResources;
	uint32_t framesCount = 0;
	uint32_t currentFrame = 0;
	uint32_t nextFrame = 0;
};
