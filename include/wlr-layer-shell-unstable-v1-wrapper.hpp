#pragma once

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"
// there is function's argument that conflicts with C++'s `namespace` keyword
#define namespace __namespace
#include <wlr-layer-shell-unstable-v1.h>
#undef namespace
