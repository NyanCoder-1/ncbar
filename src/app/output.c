#include "app/output.h"

#include <stddef.h>

#include "app/app.h"

int app_render_dirty_outputs(struct app *app) {
    int status = 0;

    for (size_t i = 0; i < app->outputs_size; ++i) {
        struct app_output *output = &app->outputs[i];

        if (!output->configured || !output->dirty) {
            continue;
        }

        // if (vk_output_render(output->renderer) != 0) {
        //     status = -1;
        //     continue;
        // }

        output->dirty = false;
    }

    return status;
}