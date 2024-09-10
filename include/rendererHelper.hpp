#pragma once

#include <cstdint>
#include <functional>
#include <memory>

class Renderer;

typedef std::function<bool(uint32_t frameIndex, Renderer *renderer)> OnPresentCallbackType;
