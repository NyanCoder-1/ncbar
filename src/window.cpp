#include "core.hpp"
#include "globals.hpp"
#include "renderer.hpp"
#include "vulkanHelper.hpp"
#include "window.hpp"
#include <iostream>

namespace {
	// Wayland
	void xdgSurfaceOnConfigureListener(void *data, xdg_surface *shellSurface, uint32_t serial)
	{
		if (data) {
			if (auto onConfigure = reinterpret_cast<Window::XdgSurfaceListenerWrapper*>(data)->onConfigure)
				onConfigure(shellSurface, serial);
		}
	}
	const xdg_surface_listener xdgSurfaceListener = {
		.configure = xdgSurfaceOnConfigureListener
	};
	void xdgToplevelOnConfigureListener(void *data, xdg_toplevel *toplevel, int32_t width, int32_t height, wl_array *states)
	{
		if (data) {
			if (auto onConfigure = reinterpret_cast<Window::XdgToplevelListenerWrapper*>(data)->onConfigure)
				onConfigure(toplevel, width, height, states);
		}
	}
	void xdgToplevelOnCloseListener(void *data, xdg_toplevel *toplevel)
	{
		if (data) {
			if (auto onClose = reinterpret_cast<Window::XdgToplevelListenerWrapper*>(data)->onClose)
				onClose(toplevel);
		}
	}
	const xdg_toplevel_listener xdgToplevelListener = {
		.configure = xdgToplevelOnConfigureListener,
		.close = xdgToplevelOnCloseListener,
		.configure_bounds = nullptr,
		.wm_capabilities = nullptr
	};
	void xdgPopupOnConfigureListener(void *data, xdg_popup *popup, int32_t x, int32_t y, int32_t width, int32_t height)
	{
		if (data) {
			if (auto onConfigure = reinterpret_cast<Window::XdgPopupListenerWrapper*>(data)->onConfigure)
				onConfigure(popup, x, y, width, height);
		}
	}
	const xdg_popup_listener xdgPopupListener = {
		.configure = xdgPopupOnConfigureListener,
		.popup_done = nullptr,
		.repositioned = nullptr
	};
	void zwlrLayerSurfaceV1OnConfigureListener(void *data, zwlr_layer_surface_v1 *layerSurface, uint32_t serial, uint32_t width, uint32_t height)
	{
		if (data) {
			if (auto onConfigure = reinterpret_cast<Window::ZwlrLayerSurfaceListenerWrapper*>(data)->onConfigure)
				onConfigure(layerSurface, serial, width, height);
		}
	}
	void zwlrLayerSurfaceV1OnClosedListener(void *data, zwlr_layer_surface_v1 *layerSurface)
	{
		if (data) {
			if (auto onClosed = reinterpret_cast<Window::ZwlrLayerSurfaceListenerWrapper*>(data)->onClosed)
				onClosed(layerSurface);
		}
	}
	const zwlr_layer_surface_v1_listener zwlrLayerSurfaceV1Listener = {
		.configure = zwlrLayerSurfaceV1OnConfigureListener,
		.closed = zwlrLayerSurfaceV1OnClosedListener
	};
}

Window::Window(const Window::Private&)
{}

Window::~Window()
{
	if (xdgToplevel) {
		xdg_toplevel_destroy(xdgToplevel);
		xdgToplevel = nullptr;
	}
	if (xdgSurface) {
		xdg_surface_destroy(xdgSurface);
		xdgSurface = nullptr;
	}
	if (layerSurface) {
		zwlr_layer_surface_v1_destroy(layerSurface);
		layerSurface = nullptr;
	}
	if (surface) {
		wl_surface_destroy(surface);
		surface = nullptr;
	}
}

bool Window::Init(CorePtr core)
{
	this->core = core;

	// ==== Wayland ====
	// Create surface
	surface = wl_compositor_create_surface(core->GetCompositor());
	if (!surface) {
		std::cerr << "Wayland: Failed to create surface" << std::endl;
		return false;
	}

	bool isBar = false;
	// Create layer surface
	if (isBar) { // Top bar
		layerSurface = zwlr_layer_shell_v1_get_layer_surface(core->GetLayerShell(), surface, nullptr, ZWLR_LAYER_SHELL_V1_LAYER_TOP, "ncbar-blur");
		if (!layerSurface) {
			std::cerr << "Wayland: Failed to create layer surface" << std::endl;
			return false;
		}
		zwlr_layer_surface_v1_set_anchor(layerSurface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
		zwlr_layer_surface_v1_set_size(layerSurface, 0, 30);
		zwlr_layer_surface_v1_set_exclusive_zone(layerSurface, 1);

		// Add listener to zwlr_layer_surface_v1
		zwlrLayerSurfaceListenerWrapper->onConfigure = [this](zwlr_layer_surface_v1 *layerSurface, uint32_t serial, uint32_t width, uint32_t height) {
			zwlr_layer_surface_v1_ack_configure(layerSurface, serial);
			if (width && height) {
				this->newWidth = width;
				this->newHeight = height;
				this->resize = true;
				this->readyToResize = true;
			}
		};
		zwlrLayerSurfaceListenerWrapper->onClosed = [this](zwlr_layer_surface_v1 *layerSurface) {
			(void)layerSurface;
			this->isGoingToClose = true;
		};
		zwlr_layer_surface_v1_add_listener(layerSurface, &zwlrLayerSurfaceV1Listener, zwlrLayerSurfaceListenerWrapper.get());
	}
	else { // Popup or regular window
		// Create xdg surface
		xdgSurface = xdg_wm_base_get_xdg_surface(core->GetXdgWmBase(), surface);
		if (!xdgSurface) {
			std::cerr << "Wayland: Failed to get xdg surface" << std::endl;
			return false;
		}
		// Add listener to xdg surface
		xdgSurfaceListenerWrapper->onConfigure = [this](xdg_surface *shellSurface, uint32_t serial) {
			xdg_surface_ack_configure(shellSurface, serial);
			if (this->resize) {
				this->readyToResize = true;
			}
		};
		xdg_surface_add_listener(xdgSurface, &xdgSurfaceListener, xdgSurfaceListenerWrapper.get());

		bool isPopup = true;
		if (isPopup) { // Popup
			xdg_surface *parentXdgSurface = nullptr;

			xdgPopupPositioner = xdg_wm_base_create_positioner(core->GetXdgWmBase());
			if (!xdgPopupPositioner) {
				std::cerr << "Wayland: Failed to create xdg positioner" << std::endl;
				return false;
			}

			xdgPopup = xdg_surface_get_popup(xdgSurface, parentXdgSurface, xdgPopupPositioner);
			if (!xdgPopup) {
				std::cerr << "Wayland: Failed to create xdg popup" << std::endl;
				return false;
			}
			xdgPopupListenerWrapper->onConfigure = [this](xdg_popup *popup, int32_t x, int32_t y, int32_t width, int32_t height) {
				(void)popup;
				(void)x;
				(void)y;
				if ((width > 0) && (height > 0)) {
					this->newWidth = width;
					this->newHeight = height;
					this->resize = true;
				}
			};
			xdg_popup_add_listener(xdgPopup, &xdgPopupListener, xdgPopupListenerWrapper.get());
		}
		else {
			// Get xdg toplevel
			xdgToplevel = xdg_surface_get_toplevel(xdgSurface);
			if (!xdgToplevel) {
				std::cerr << "Wayland: Failed to get xdg toplevel" << std::endl;
				return false;
			}
			// Add listener to xdg toplevel
			xdgToplevelListenerWrapper->onConfigure = [this](xdg_toplevel *toplevel, int32_t width, int32_t height, wl_array *states) {
				(void)toplevel;
				(void)states;
				if ((width > 0) && (height > 0)) {
					this->newWidth = width;
					this->newHeight = height;
					this->resize = true;
				}
			};
			xdgToplevelListenerWrapper->onClose = [this](xdg_toplevel *toplevel) {
				(void)toplevel;
				this->isGoingToClose = true;
			};
			xdg_toplevel_add_listener(xdgToplevel, &xdgToplevelListener, xdgToplevelListenerWrapper.get());

			// Fill info
			xdg_toplevel_set_title(xdgToplevel, windowTitle);
			xdg_toplevel_set_app_id(xdgToplevel, appId);
		}
	}

	// Commit
	wl_surface_commit(surface);
	wl_display_roundtrip(core->GetDisplay());

	if (readyToResize && resize && newWidth && newHeight) {
		width = newWidth;
		height = newHeight;
	}

	{
		renderer = Renderer::Create(shared_from_this());
		if (!renderer) {
			std::cerr << "Failed to create renderer" << std::endl;
			return false;
		}
	}

	return true;
}

bool Window::Render()
{
	if (readyToResize && resize) {
		width = newWidth;
		height = newHeight;

		CHECK_VK_RESULT(vkDeviceWaitIdle(core->GetDevice()));

		if (!renderer->OnResize()) {
			std::cerr << "Failed to resize renderer" << std::endl;
			return false;
		}

		readyToResize = false;
		resize = false;

		wl_surface_commit(surface);
	}

	if (!renderer->Render())
		return false;
	
	return true;
}

void Window::SetOnPresent(OnPresentCallbackType onPresent)
{
	renderer->SetOnPresent(onPresent);
}
