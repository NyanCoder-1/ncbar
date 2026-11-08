#include "app/signals.h"

#include <errno.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>

static void app_signals_fill_set(sigset_t *set) {
    sigemptyset(set);
    sigaddset(set, SIGINT);
    sigaddset(set, SIGTERM);
}

int app_signals_block(void) {
    sigset_t set;
    app_signals_fill_set(&set);
    return pthread_sigmask(SIG_BLOCK, &set, NULL) == 0 ? 0 : -1;
}

int app_signals_open_fd(void) {
    sigset_t set;
    app_signals_fill_set(&set);
    return signalfd(-1, &set, SFD_CLOEXEC | SFD_NONBLOCK);
}

void app_signals_drain(int fd) {
    struct signalfd_siginfo info;
    for (;;) {
        ssize_t n = read(fd, &info, sizeof(info));
        if (n == (ssize_t)sizeof(info) || (n < 0 && errno == EINTR)) {
            continue;
        }
        break; // EAGAIN means drained
    }
}