#include "../include/visky/visky.h"

#define USE_PROPS_API 1

int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);

    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_MESH_RAW, NULL, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

// Triangle.
#if USE_PROPS_API == 0
    VkyVertex vertices[3] = {
        {{-1, -1, 0}, {{255, 0, 0}, 255}},
        {{+1, -1, 0}, {{0, 255, 0}, 255}},
        {{0, +1, 0}, {{0, 0, 255}, 255}},
    };
    visual->data.vertex_count = 3;
    visual->data.vertices = vertices;
    vky_visual_data_raw(visual);
#else
    vec3 positions[] = {{-1, -1, 0}, {+1, -1, 0}, {0, +1, 0}};
    cvec4 colors[] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};

    vky_visual_data_set_size(visual, 3, 0, NULL, NULL);
    vky_visual_data(visual, VKY_VISUAL_PROP_POS, 0, 3, positions);
    vky_visual_data(visual, VKY_VISUAL_PROP_COLOR_ALPHA, 0, 3, colors);
#endif

    vky_run_app(app);
    vky_destroy_app(app);
    return 0;
}
