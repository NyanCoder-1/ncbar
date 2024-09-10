#pragma once

#include "vulkanInclude.hpp"
#include <vulkan/vk_enum_string_helper.h>
#include <iostream>
#include <string>

constexpr void print_vk_result(std::string name, VkResult result)
{
	if (result != VK_SUCCESS) {
		std::cerr << "Vulkan: Failed to run command `" << name << "`: " << string_VkResult(result) << std::endl;
	}
}
#define CHECK_VK_RESULT(result) print_vk_result(#result, result)
