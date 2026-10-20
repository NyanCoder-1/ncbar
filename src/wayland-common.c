#include "wayland-common.h"
#include "wlr-layer-shell-unstable-v1.h"
#include <stdio.h>
#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#define WAYLANDCOMMON_SUCCESS 0
#define WAYLANDCOMMON_FAIL 1

struct WaylandCommon_RegistryData {
    struct wl_compositor *compositor;
    struct zwlr_layer_shell_v1 *layerShell;
};

static int WaylandCommon_InitDisplay(struct wl_display **display);
static int WaylandCommon_InitRegistry(struct wl_display *display,
                                      struct wl_registry **registry,
                                      struct wl_compositor **compositor,
                                      struct zwlr_layer_shell_v1 **layerShell);

WaylandCommon WaylandCommon_Create() {
    struct wl_display *display = NULL;
    if (WaylandCommon_InitDisplay(&display) != 0) {
        return (WaylandCommon){0};
    }
    struct wl_registry *registry = NULL;
    struct wl_compositor *compositor = NULL;
    struct zwlr_layer_shell_v1 *layerShell = NULL;
    if (WaylandCommon_InitRegistry(display, &registry, &compositor,
                                   &layerShell) != 0) {
        return (WaylandCommon){0};
    }
    WaylandCommon wlCommon = {0};
    wlCommon.success = 1;
    wlCommon.display = display;
    wlCommon.registry = registry;
    wlCommon.compositor = compositor;
    wlCommon.layerShell = layerShell;
    return wlCommon;
}
void WaylandCommon_Destroy(WaylandCommon *wlCommon) {
    if (wlCommon == NULL)
        return;
    wl_display_disconnect(wlCommon->display);
    memset(wlCommon, 0, sizeof(WaylandCommon));
}
WaylandWindowData WaylandCommon_CreateWindow(WaylandCommon *self,
                                             const char *layerNamespace,
                                             uint32_t layerZIndex) {
    struct wl_surface *surface = wl_compositor_create_surface(self->compositor);
    if (surface == NULL) {
        fprintf(stderr, "Wayland: failed to create wl_surface\n");
        return (WaylandWindowData){0};
    }
    struct zwlr_layer_surface_v1 *layerSurface =
        zwlr_layer_shell_v1_get_layer_surface(self->layerShell, surface, NULL,
                                              layerZIndex, layerNamespace);
    if (layerSurface == NULL) {
        fprintf(stderr, "Wayland: failed to create layer surface\n");
        wl_surface_destroy(surface);
        return (WaylandWindowData){0};
    }
    WaylandWindowData wndData = {0};
    wndData.surface = surface;
    wndData.layerSurface = layerSurface;
    return wndData;
}

static void wlRegistryOnGlobalListener(void *data, struct wl_registry *registry,
                                       uint32_t name, const char *interface,
                                       uint32_t version);
const struct wl_registry_listener wlRegistryListener = {
    .global = wlRegistryOnGlobalListener, .global_remove = NULL};

static int WaylandCommon_InitDisplay(struct wl_display **displayOutput) {
    struct wl_display *display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Wayland: Failed to connect to display\n");
        return WAYLANDCOMMON_FAIL;
    }
    *displayOutput = display;
    return WAYLANDCOMMON_SUCCESS;
}
static int
WaylandCommon_InitRegistry(struct wl_display *display,
                           struct wl_registry **registryOutput,
                           struct wl_compositor **compositorOutput,
                           struct zwlr_layer_shell_v1 **layerShellOutput) {
    struct wl_registry *registry = wl_display_get_registry(display);
    struct WaylandCommon_RegistryData registryData = {0};
    wl_registry_add_listener(registry, &wlRegistryListener, &registryData);
    wl_display_roundtrip(display);
    if (registryData.compositor == NULL) {
        fprintf(stderr, "Wayland: Failed to get compositor\n");
        return WAYLANDCOMMON_FAIL;
    }
    if (registryData.layerShell == NULL) {
        fprintf(stderr, "Wayland: Failed to get zwlr_layer_shell_v1\n");
        return WAYLANDCOMMON_FAIL;
    }
    *registryOutput = registry;
    *compositorOutput = registryData.compositor;
    *layerShellOutput = registryData.layerShell;
    return WAYLANDCOMMON_SUCCESS;
}

static void wlRegistryOnGlobalListener(void *data, struct wl_registry *registry,
                                       uint32_t name, const char *interface,
                                       uint32_t version) {
    if (data) {
        struct WaylandCommon_RegistryData *registryData =
            (struct WaylandCommon_RegistryData *)data;
        if (strcmp(interface, wl_compositor_interface.name) == 0) {
            registryData->compositor =
                wl_registry_bind(registry, name, &wl_compositor_interface, 1);
        } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
            registryData->layerShell = wl_registry_bind(
                registry, name, &zwlr_layer_shell_v1_interface, 1);
        }
    }
}