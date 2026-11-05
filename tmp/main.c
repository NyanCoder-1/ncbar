#include "renderer.h"
#include "wayland-common.h"
#include "window.h"
#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

int main(void) {
    WaylandCommon wlCommon = WaylandCommon_Create();
    if (wlCommon.success == 0) {
        fprintf(stdout, "Wayland init bad\n");
    } else {
        fprintf(stdout, "Wayland init good\n");
    }
    Renderer renderer = Renderer_Create(wlCommon.display);
    if (renderer.success == 0) {
        fprintf(stdout, "Vulkan init bad\n");
    } else {
        fprintf(stdout, "Vulkan init good\n");
    }

    Window wnd = {0};
    bool windowCreated = Window_Create(&wnd, &wlCommon, &renderer);
    if (!windowCreated || !wnd.configured) {
        fprintf(stdout, "Window init bad\n");
    } else {
        fprintf(stdout, "Window init good\n");
    }

    Window_Destroy(&wnd);
    WaylandCommon_Destroy(&wlCommon);
    Renderer_Destroy(&renderer);

    return 0;
}