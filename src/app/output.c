#include "app/output.h"

#include <stddef.h>

#include "app/app.h"

enum ncbar_app_error app_render_dirty_outputs(struct app *app) {
    int status = NCBAR_APP_OK;

    for (size_t i = 0; i < app->outputs_size; ++i) {
        struct app_output *output = &app->outputs[i];

        if (!output->configured || !output->dirty) {
            continue;
        }

        output->dirty = false;
    }

    return status;
}