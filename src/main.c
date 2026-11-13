#include "app/app.h"

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    struct app app = {0};

    int status = app_init(&app);

    if (status == 0) {
        status = app_run(&app);
    }

    app_destroy(&app);

    return status;
}