#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <wayland-client-core.h>

#include "app/app.h"
#include "app/output.h"
#include "platform/wayland/context.h"

enum ncbar_app_error app_run(struct app *app) {
    struct wl_display *display = wayland_context_get_display(&app->wl_context);
    if (display == NULL) {
        return NCBAR_APP_WAYLAND_DISPLAY_NULL;
    }

    int display_fd = wl_display_get_fd(display);
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);

    if (epoll_fd < 0) {
        return NCBAR_APP_EPOLL_CREATE_FAILED;
    }

    struct epoll_event watched = {.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP, .data.fd = display_fd};

    struct epoll_event ready[8];

    bool read_prepared = false;
    enum ncbar_app_error status = NCBAR_APP_OK;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, display_fd, &watched) < 0) {
        status = NCBAR_APP_EPOLL_ADD_FAILED;
        goto cleanup;
    }

    while (app->running) {
        /*
         * Process pending callbacks and render dirty outputs.
         *
         * prepare_read() may fail if more events are pending,
         * so we repeat this until the queue is ready.
         */
        for (;;) {
            if (wl_display_dispatch_pending(display) < 0) {
                status = NCBAR_APP_WAYLAND_DISPATCH_FAILED;
                goto cleanup;
            }

            status = app_render_dirty_outputs(app);
            if (status != NCBAR_APP_OK) {
                goto cleanup;
            }

            if (!app->running) {
                goto cleanup;
            }

            if (wl_display_prepare_read(display) == 0) {
                read_prepared = true;
                break;
            }

            if (wl_display_get_error(display) != 0) {
                status = NCBAR_APP_WAYLAND_DISPLAY_ERROR;
                goto cleanup;
            }
        }

        /*
         * Flush outgoing Wayland requests.
         *
         * EPOLLOUT is only enabled if flush returns EAGAIN.
         * Otherwise, a writable socket would wake us constantly.
         */
        bool want_write = false;

        if (wl_display_flush(display) < 0) {
            if (errno != EAGAIN) {
                status = NCBAR_APP_WAYLAND_FLUSH_FAILED;
                goto cleanup;
            }

            want_write = true;
        }

        watched.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

        if (want_write) {
            watched.events |= EPOLLOUT;
        }

        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, display_fd, &watched) < 0) {
            status = NCBAR_APP_EPOLL_MOD_FAILED;
            goto cleanup;
        }

        /*
         * Sleep until the Wayland connection becomes ready.
         */
        int count;

        do {
            count = epoll_wait(epoll_fd, ready, 8, -1);
        } while (count < 0 && errno == EINTR);

        if (count < 0) {
            status = NCBAR_APP_EPOLL_WAIT_FAILED;
            goto cleanup;
        }

        uint32_t events = 0;

        for (int i = 0; i < count; ++i) {
            if (ready[i].data.fd == display_fd) {
                events |= ready[i].events;
            }
        }

        /*
         * Complete the prepared Wayland read.
         *
         * Every successful prepare_read() must be paired
         * with exactly one read_events() or cancel_read().
         */
        if (events & EPOLLIN) {
            read_prepared = false;

            if (wl_display_read_events(display) < 0) {
                status = NCBAR_APP_WAYLAND_READ_FAILED;
                goto cleanup;
            }
        } else {
            wl_display_cancel_read(display);
            read_prepared = false;
        }

        /*
         * The socket is writable again.
         */
        if (events & EPOLLOUT) {
            if (wl_display_flush(display) < 0 && errno != EAGAIN) {
                status = NCBAR_APP_WAYLAND_FLUSH_FAILED;
                goto cleanup;
            }
        }

        /*
         * Detect compositor disconnection.
         */
        if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
            status = NCBAR_APP_WAYLAND_DISCONNECTED;
            goto cleanup;
        }
    }

cleanup:
    if (read_prepared) {
        wl_display_cancel_read(display);
    }

    close(epoll_fd);

    return status;
}