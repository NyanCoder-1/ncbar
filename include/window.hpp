#pragma once

#include "rendererHelper.hpp"
#include "wlr-layer-shell-unstable-v1-wrapper.hpp"
#include <wayland-client.h>
#include <xdg-shell.h>
#include <memory>

class Core;
class Renderer;

constexpr auto windowMagicNumber = 0x000b00b5;
class Window : public std::enable_shared_from_this<Window>
{
	struct Private { explicit Private() = default; };
	friend Renderer;
	typedef std::unique_ptr<Renderer> RendererPtr;
	typedef std::shared_ptr<Core> CorePtr;
public:
	typedef std::shared_ptr<Window> Ptr;
	struct XdgSurfaceListenerWrapper {
		std::function<void(xdg_surface *shellSurface, uint32_t serial)> onConfigure;
		~XdgSurfaceListenerWrapper() {
			onConfigure = nullptr;
		}
	};
	struct XdgToplevelListenerWrapper {
		std::function<void(xdg_toplevel *toplevel, int32_t width, int32_t height, wl_array *states)> onConfigure;
		std::function<void(xdg_toplevel *toplevel)> onClose;
		~XdgToplevelListenerWrapper() {
			onConfigure = nullptr;
			onClose = nullptr;
		}
	};
	struct XdgPopupListenerWrapper {
		std::function<void(xdg_popup *popup, int32_t x, int32_t y, int32_t width, int32_t height)> onConfigure;
		~XdgPopupListenerWrapper() {
			onConfigure = nullptr;
		}
	};
	struct ZwlrLayerSurfaceListenerWrapper {
		std::function<void(zwlr_layer_surface_v1 *layerSurface, uint32_t serial, uint32_t width, uint32_t height)> onConfigure;
		std::function<void(zwlr_layer_surface_v1 *layerSurface)> onClosed;
		~ZwlrLayerSurfaceListenerWrapper() {
			onConfigure = nullptr;
			onClosed = nullptr;
		}
	};
	Window() = delete;
	Window(const Private&);
	~Window();
	static Window::Ptr Create(CorePtr core)
	{
		auto ptr = std::make_shared<Window>(Private());
		if (!ptr->Init(core))
			return nullptr;
		return ptr;
	}

	bool Render();

	void SetOnPresent(OnPresentCallbackType onPresent);

	wl_surface* GetSurface() { return surface; }
	xdg_surface* GetXdgSurface() { return xdgSurface; }
	xdg_toplevel* GetXdgToplevel() { return xdgToplevel; }
	xdg_positioner* GetXdgPopupPositioner() { return xdgPopupPositioner; }
	xdg_popup* GetXdgPopup() { return xdgPopup; }
	zwlr_layer_surface_v1* GetLayerSurface() { return layerSurface; }
	int32_t GetWidth() const { return width; }
	int32_t GetHeight() const { return height; }
	bool IsGoingToClose() const { return isGoingToClose; }

private:
	bool Init(CorePtr core);

	CorePtr core;
	// Wayland
	wl_surface *surface = nullptr;
	xdg_surface *xdgSurface = nullptr;
	xdg_toplevel *xdgToplevel = nullptr;
	xdg_positioner *xdgPopupPositioner = nullptr;
	xdg_popup *xdgPopup = nullptr;
	zwlr_layer_surface_v1 *layerSurface = nullptr;
	std::unique_ptr<XdgSurfaceListenerWrapper> xdgSurfaceListenerWrapper = std::make_unique<Window::XdgSurfaceListenerWrapper>();
	std::unique_ptr<XdgToplevelListenerWrapper> xdgToplevelListenerWrapper = std::make_unique<Window::XdgToplevelListenerWrapper>();
	std::unique_ptr<XdgPopupListenerWrapper> xdgPopupListenerWrapper = std::make_unique<Window::XdgPopupListenerWrapper>();
	std::unique_ptr<ZwlrLayerSurfaceListenerWrapper> zwlrLayerSurfaceListenerWrapper = std::make_unique<Window::ZwlrLayerSurfaceListenerWrapper>();

	uint32_t width = 1280;
	uint32_t height = 720;
	uint32_t newWidth = 0;
	uint32_t newHeight = 0;

	RendererPtr renderer;

	// bitfield
	bool resize : 1 = false;
	bool readyToResize : 1 = false;
	bool isGoingToClose : 1 = false;
};
