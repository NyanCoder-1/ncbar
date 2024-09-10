#include "core.hpp"
#include "renderer.hpp"
#include "vulkanInclude.hpp"
#include "window.hpp"
#include <argparse/argparse.hpp>
#include <chrono>
#include <iostream>


int main(int argc, char *argv[]) {
	auto parser = argparse::ArgumentParser(argc, argv).add_help(false);

	parser.add_argument("--help", "-h").action("help").help("show help and exit");
	parser.add_argument("--version", "-v").action("version").version("1.0");

	const auto args = parser.parse_args();

	auto core = Core::Create();
	if (!core) {
		std::cerr << "Failed to create wayland core" << std::endl;
		return 1;
	}
	auto window1 = Window::Create(core);
	if (!window1) {
		std::cerr << "Window1 creation failed" << std::endl;
		return 1;
	}

	auto startTime = std::chrono::high_resolution_clock::now();
	window1->SetOnPresent([appCore = core, startTime](uint32_t frameIndex, Renderer *renderer)->bool {
		auto now = std::chrono::high_resolution_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
		(void)elapsedTime;
		Window::Ptr window = renderer->GetWindow();
		if (!window)
			return false;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		VkRenderPassBeginInfo renderPassBeginInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = renderer->GetRenderPass(),
			.framebuffer = renderer->GetCurrentFrameFramebuffer(frameIndex),
			.renderArea = {
				.offset = VkOffset2D{ .x = 0, .y = 0 },
				.extent = VkExtent2D{ .width = static_cast<uint32_t>(window->GetWidth()), .height = static_cast<uint32_t>(window->GetHeight()) }
			},
			.clearValueCount = 1,
			.pClearValues = &clearColor
		};

		vkCmdBeginRenderPass(renderer->GetCurrentFrameCommandBuffer(frameIndex), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(renderer->GetCurrentFrameCommandBuffer(frameIndex));

		return true;
	});

	while (!window1->IsGoingToClose()) {
		if (!window1->Render())
			return 1;

		// wl_display_dispatch(core->GetDisplay());
		wl_display_roundtrip(core->GetDisplay());
	}

	return 0;
}