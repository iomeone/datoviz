#include <visky/visky.h>

static void mesh_raw(VkyPanel* panel)
{
    // Create the visual.
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_MESH_RAW, NULL, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    float x = .5;
    const uint32_t n = 6;
    vec3 positions[] = {
        {-x, -x, 0}, {+x, -x, 0}, {+x, x, 0}, {+x, x, 0}, {-x, x, 0}, {-x, -x, 0},
    };
    cvec4 colors[] = {
        {255, 0, 0, 255}, {0, 255, 0, 255},   {0, 0, 255, 255},
        {0, 0, 255, 255}, {255, 0, 255, 255}, {255, 0, 0, 255},
    };
    vky_visual_data(visual, VKY_VISUAL_PROP_POS, 0, n, positions);
    vky_visual_data(visual, VKY_VISUAL_PROP_COLOR_ALPHA, 0, n, colors);
}

static void scatter(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    VkyMarkersParams params = (VkyMarkersParams){{0, 0, 0, 1}, 1.0f, false};
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_MARKER, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    const uint32_t n0 = 11;
    const uint32_t n = n0 * n0;
    vec3* positions = calloc(n, sizeof(vec3));
    VkyColor* colors = calloc(n, sizeof(VkyColor));
    for (uint32_t i = 0; i < n; i++)
    {
        positions[i][0] = -1 + (2.0 / (n0 - 1)) * (i % n0);
        positions[i][1] = -1 + (2.0 / (n0 - 1)) * (i / n0);

        colors[i] = vky_color(VKY_CMAP_JET, i % n0, 0, n0, .5 + .5 * rand_float());
    }

    float size = 20;
    VkyMarkerType marker = VKY_MARKER_DISC;
    uint8_t angle = 0;

    vky_visual_data(visual, VKY_VISUAL_PROP_POS, 0, n, positions);
    vky_visual_data(visual, VKY_VISUAL_PROP_COLOR_ALPHA, 0, n, colors);
    vky_visual_data(visual, VKY_VISUAL_PROP_SIZE, 0, 1, &size);
    vky_visual_data(visual, VKY_VISUAL_PROP_SHAPE, 0, 1, &marker);
    vky_visual_data(visual, VKY_VISUAL_PROP_ANGLE, 0, 1, &angle);
}

static void imshow(VkyPanel* panel)
{
    vky_set_constant(VKY_AXES_GRID_COLOR_A_ID, 0);

    // Load the image.
    const uint32_t size = 100;
    VkyColor* image = calloc(size * size, sizeof(VkyColor));
    for (uint32_t i = 0; i < size; i++)
    {
        for (uint32_t j = 0; j < size; j++)
        {
            image[size * i + j] = vky_color(VKY_CMAP_VIRIDIS, rand_byte(), 0, 255, 1);
        }
    }

    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    VkyTextureParams params = vky_default_texture_params(size, size, 1); // (VkyTextureParams){
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_IMAGE, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    VkyImageData data[1] = {{
        {-1, -1, 0},
        {+1, +1, 0},
        {0, 1},
        {1, 0},
    }};
    visual->data.item_count = 1;
    visual->data.items = data;
    vky_visual_data_raw(visual);

    // Upload the texture
    vky_visual_image_upload(visual, (uint8_t*)image);
    free(image);
}

static void arrows(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_ARROW, NULL, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    const uint32_t n = 20;
    VkyArrowVertex data[20];
    double t = 0;
    double r = .25;
    double R = .75;
    for (uint32_t i = 0; i < n; i++)
    {
        t = 2 * M_PI * (float)i / n;
        data[i] = (VkyArrowVertex){
            {R * cos(t), R * sin(t), 0},
            {r * cos(t), r * sin(t), 0},
            vky_color(VKY_DEFAULT_COLORMAP, i, 0, n, 1),
            15,
            5,
            VKY_ARROW_STEALTH};
    }
    visual->data.item_count = n;
    visual->data.items = data;
    vky_visual_data_raw(visual);
}

static void paths(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    // Create the visual.
    VkyPathParams params = {20, 4., VKY_CAP_ROUND, VKY_JOIN_ROUND, VKY_DEPTH_DISABLE};
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_PATH, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    const uint32_t n = 1000;
    vec3 points[1000];
    VkyColor color[1000];
    double t = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        t = (float)i / n;

        points[i][0] = -1 + 2 * t;
        points[i][0] *= .9;
        points[i][1] = .5 * sin(8 * M_PI * t);
        points[i][2] = 0;

        if (i >= n / 2)
            points[i][1] += .25;
        else
            points[i][1] -= .25;

        color[i] = vky_color(VKY_CMAP_JET, (i >= n / 2), 0, 1, 1);
    }

    vky_visual_data_set_size(
        visual, n, 2, (uint32_t[]){n / 2, n / 2},
        (VkyPathTopology[]){VKY_PATH_OPEN, VKY_PATH_OPEN});
    vky_visual_data(visual, VKY_VISUAL_PROP_POS, 0, n, points);
    vky_visual_data(visual, VKY_VISUAL_PROP_COLOR_ALPHA, 0, n, color);
}

static void segments(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_SEGMENT, NULL, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    const uint32_t n = 20;
    VkySegmentVertex data[20];
    memset(data, 0, sizeof(data));
    float y = 0;
    float t = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        t = (float)i / (n - 1);
        y = -1 + 2 * t;
        glm_vec3_copy((vec3){0, y, 0}, data[i].P0);
        glm_vec3_copy((vec3){0.1, y, 0}, data[i].P1);
        data[i].color = vky_color(VKY_CMAP_VIRIDIS, i, 0, n, 1);
        data[i].linewidth = 20;
        data[i].cap0 = VKY_CAP_ROUND;
        data[i].cap1 = VKY_CAP_ROUND;
        glm_vec4_copy((vec4){-100 + 200 * t, 0, -100 + 200 * t, 0}, data[i].shift);
    }
    visual->data.item_count = n;
    visual->data.items = data;
    vky_visual_data_raw(visual);
}

static void text(VkyPanel* panel) { hello(panel); }

static void hist(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    VkyRectangleParams params = {0};
    params.u[0] = 1;
    params.v[1] = -1;
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_RECTANGLE, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    const uint32_t n = 100;
    VkyRectangleData data[100];
    float x = 0, dx = 2 / (float)n;
    float eps = 2e-3;
    for (uint32_t i = 0; i < n; i++)
    {
        x = -1 + 2 * (float)i / n;
        data[i] = (VkyRectangleData){
            {x + eps, 0},
            {dx - 2 * eps, .5 * cos(2 * M_2PI * x)},
            vky_color(VKY_DEFAULT_COLORMAP, i, 0, n, 1),
        };
    }
    visual->data.item_count = n;
    visual->data.items = data;
    vky_visual_data_raw(visual);
}

static void area(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    VkyAreaParams params = {0};
    params.u[0] = 1;
    params.v[1] = -1;
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_AREA, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    const uint32_t n = 1000;
    float width = 2 / (float)n;
    float height = .25;
    float x = 0, y = 0;

    vec2* pos = calloc(2 * n, sizeof(vec2));
    VkyColor* color = calloc(2 * n, sizeof(VkyColor));
    for (uint32_t i = 0; i < 2 * n; i++)
    {
        x = -1 + (i % n) * width;
        y = -.375 + .5 * cos(2 * M_2PI * x) + .5 * (i < n);
        pos[i][0] = x;
        pos[i][1] = y;
        color[i] = vky_color(VKY_DEFAULT_COLORMAP, i < n ? i : 2 * n - 1 - i, 0, n, 1);
    };

    vky_visual_data_set_size(visual, 2 * n, 2, (uint32_t[]){n, n}, NULL);
    vky_visual_data(visual, VKY_VISUAL_PROP_POS, 0, 2 * n, pos);
    vky_visual_data(visual, VKY_VISUAL_PROP_COLOR_ALPHA, 0, 2 * n, color);
    vky_visual_data(visual, VKY_VISUAL_PROP_SIZE, 0, 1, &height);

    free(pos);
    free(color);
}

static void axrect(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_RECTANGLE_AXIS, NULL, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    const uint32_t n = 10;
    VkyRectangleAxisData data[20];
    float w = .25 / (float)n;
    float x = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        x = -1 + w + (2 - 2 * w) * (float)i / (n - 1);
        for (uint32_t k = 0; k < 2; k++)
        {
            data[2 * i + k] = (VkyRectangleAxisData){
                {x - w, x + w},
                k,
                vky_color(VKY_DEFAULT_COLORMAP, i, 0, n - 1, 1),
            };
            ASSERT(2 * i + k < 2 * n);
        }
    }
    visual->data.item_count = 2 * n;
    visual->data.items = data;
    vky_visual_data_raw(visual);
}

static void raster(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    VkyMarkersRawParams params = (VkyMarkersRawParams){{5.0f, 20.0f}, VKY_SCALING_OFF};
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_MARKER_RAW, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    const uint32_t n_rows = 100;
    const uint32_t n_cols = 100;
    const uint32_t n = n_rows * n_cols;
    VkyVertex* data = calloc(2 * n, sizeof(VkyVertex));
    float x = 0, y = 0, dy = 2.0 / (n_rows - 1);
    uint32_t count = 0;
    for (uint32_t i = 0; i < n_rows; i++)
    {
        x = -1.0;
        y = -1.0 + i * dy;
        for (uint32_t j = 0; j < n_cols; j++)
        {
            x += 8 * rand_float() / (n_cols);
            if (x > 1)
                break;
            data[count] = (VkyVertex){
                {x, y, 0},
                vky_color(VKY_CMAP_AUTUMN, i, 0, n_rows - 1, 1),
            };
            count++;
        }
    }
    visual->data.item_count = count;
    visual->data.items = data;
    vky_visual_data_raw(visual);
    free(data);
}

static void graph(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);

    // Graph parameters.
    VkyGraphParams params = {0};
    params.marker_edge_width = 1.0f;
    glm_vec4_copy(GLM_VEC4_BLACK, params.marker_edge_color);

    // Create the graph visual.
    VkyVisual* graph = vky_visual_graph(panel->scene, params);
    vky_add_visual_to_panel(graph, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Create the graph.
    const uint32_t nv = 50;
    const uint32_t ne = nv * (nv - 1) / 2;
    VkyGraphNode* nodes = calloc(nv, sizeof(VkyGraphNode));
    VkyGraphEdge* edges = calloc(ne, sizeof(VkyGraphEdge));

    // Nodes.
    for (uint32_t i = 0; i < nv; i++)
    {
        float angle = M_2PI * (float)i / nv;
        nodes[i] = (VkyGraphNode){
            {.75f * cos(angle), .75f * sin(angle), 0},
            vky_color(VKY_DEFAULT_COLORMAP, i, 0, nv, 1),
            10.0f + 20.0f * rand_float(),
            VKY_MARKER_DISC};
    }

    // Edges.
    uint32_t k = 0;
    for (uint32_t i = 0; i < nv; i++)
    {
        for (uint32_t j = i + 1; j < nv; j++)
        {
            ASSERT(k < ne);
            edges[k] = (VkyGraphEdge){
                i,
                j,
                {{0, 0, 0}, 255}, // TODO: black
                1,
                VKY_CAP_ROUND,
                VKY_CAP_ROUND};
            // Hide some edges.
            if (rand_float() < .85)
            {
                edges[k].color.alpha = 0;
            }
            k++;
        }
    }
    ASSERT(k == ne);

    vky_graph_upload(graph, nv, nodes, ne, edges);

    free(nodes);
    free(edges);
}

static void image(VkyPanel* panel)
{
    // Load the image.
    char path[1024];
    snprintf(path, sizeof(path), "%s/textures/%s", DATA_DIR, "image.png");
    int w, h, b;
    stbi_uc* pixels = stbi_load(path, &w, &h, &b, STBI_rgb_alpha);
    if (pixels == NULL)
        log_error("unable to load %s", path);
    uint32_t IMAGE_SIZE = (uint32_t)w;
    log_trace("load image with size %dx%d, %d bytes per channel", w, h, b);

    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    VkyTextureParams params = (VkyTextureParams){
        IMAGE_SIZE,
        IMAGE_SIZE,
        1,
        4,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        false,
    };
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_IMAGE, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    const uint32_t n0 = 3;
    const uint32_t n = n0 * n0;
    VkyImageData* data = calloc(n, sizeof(VkyImageData));
    float x, y;
    float eps = .01;
    float size = 2.0 / (float)n0;
    for (uint32_t i = 0; i < n0; i++)
    {
        for (uint32_t j = 0; j < n0; j++)
        {
            x = -1 + size * j;
            y = -1 + size * i;
            data[n0 * i + j] = (VkyImageData){
                {x + eps, y + eps, 0},
                {x + size - eps, y + size - eps, 0},
                {0, 1},
                {1, 0},
            };
        }
    }
    visual->data.item_count = n;
    visual->data.items = data;
    vky_visual_data_raw(visual);
    free(data);

    // Upload the texture
    vky_visual_image_upload(visual, pixels);
    stbi_image_free(pixels);
}

static void add_polygon(dvec2* points, uint32_t n, double angle, vec2 offset)
{
    for (uint32_t i = 0; i < n; i++)
    {
        points[i][0] = offset[0] + .25 * cos(angle + M_2PI * (float)i / (n - 1));
        points[i][1] = offset[1] + .25 * sin(angle + M_2PI * (float)i / (n - 1));
    }
}

static void polygon(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    vky_set_panel_aspect_ratio(panel, 1);

    VkyPolygonParams params = (VkyPolygonParams){20, {{0, 0, 0}, 128}};
    VkyVisual* vb = vky_visual_polygon(panel->scene, &params);
    vky_add_visual_to_panel(vb, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    const uint32_t n0 = 4, n1 = 5, n2 = 6;
    uint32_t point_count = n0 + n1 + n2;
    ASSERT(point_count <= 1000);
    dvec2 points[1000];
    add_polygon(points, n0, M_PI / 2, (vec2){-.75, 0});
    add_polygon(points + n0, n1, M_PI / 4, (vec2){0, 0});
    add_polygon(points + n0 + n1, n2, M_PI / 2, (vec2){+.75, 0});

    const uint32_t poly_count = 3;
    uint32_t poly_lengths[3];
    poly_lengths[0] = n0;
    poly_lengths[1] = n1;
    poly_lengths[2] = n2;

    VkyColor poly_colors[3];
    poly_colors[0] = vky_color(VKY_CMAP_HSV, 0, 0, 3, 1);
    poly_colors[1] = vky_color(VKY_CMAP_HSV, 1, 0, 3, 1);
    poly_colors[2] = vky_color(VKY_CMAP_HSV, 2, 0, 3, 1);

    vky_visual_polygon_upload(
        vb,                                // visual
        point_count, (const dvec2*)points, // points
        poly_count, poly_lengths,          // polygons
        poly_colors                        // polygon colors
    );
}

static void pslg_1(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    vky_set_panel_aspect_ratio(panel, 1);

    // Points.
    const uint32_t point_count = 3 * 4;
    double a = .75, b = .5, c = .25;
    const dvec2 points[] = {
        {-a, -a}, {+a, -a}, {+a, +a}, {-a, +a}, // square 1
        {-b, -b}, {+b, -b}, {+b, +b}, {-b, +b}, // square 2
        {-c, -c}, {+c, -c}, {+c, +c}, {-c, +c}, // square 3
    };

    // Segments.
    const uint32_t segment_count = point_count;
    const uvec2 segments[] = {
        {0, 1}, {1, 2},  {2, 3},   {3, 0},  // square 1
        {4, 5}, {5, 6},  {6, 7},   {7, 4},  // square 2
        {8, 9}, {9, 10}, {10, 11}, {11, 8}, // square 3
    };

    // Regions.
    const uint32_t region_count = 3;
    const dvec2 region_coords[] = {{0, 0}, {(b + c) / 2, 0}, {(a + b) / 2, 0}};

    // Region colors.
    VkyColor region_colors[] = {
        {{128, 128, 128}, 255}, // index 0 = no region
        vky_color(VKY_CMAP_JET, 1, 0, 10, 1),
        vky_color(VKY_CMAP_JET, 4, 0, 10, 1),
        vky_color(VKY_CMAP_JET, 7, 0, 10, 1),
    };

    // PSLG visual.
    VkyPSLGParams params = (VkyPSLGParams){5, {{0, 0, 0}, 255}};
    VkyVisual* vb = vky_visual_pslg(panel->scene, &params);
    vky_add_visual_to_panel(vb, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
    VkyPSLGTriangulation tr = vky_visual_pslg_upload(
        vb,                                         // visual
        point_count, points,                        // points
        segment_count, segments,                    // segments
        region_count, region_coords, region_colors, // regions,
        "pzqAQa0.002");                             // triangle params

    // Triangulation visual.
    VkyTriangulationParams tparams =
        (VkyTriangulationParams){2, {{0, 0, 0}, 255}, {8, 8}, {{0, 0, 0}, 255}};
    VkyVisual* vbt = vky_visual_triangulation(panel->scene, &tparams);
    vky_add_visual_to_panel(vbt, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
    vky_visual_triangulation_upload(
        vbt, tr.vertex_count, sizeof(VkyVertex), tr.mesh_vertices, tr.index_count, tr.indices);
    vky_destroy_pslg_triangulation(&tr);
}

static void pslg_2(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);
    vky_set_panel_aspect_ratio(panel, 1);

    // Points and segments.
    const uint32_t N = 21;
    const uint32_t point_count = N + 4;
    const uint32_t segment_count = 4 + N - 1;
    double a = .75, b = .1;

    ASSERT(segment_count <= 1000);
    ASSERT(point_count <= 1000);
    uvec2 segments[1000];
    dvec2 points[1000];

    // Square.
    points[0][0] = -a;
    points[0][1] = -a;
    points[1][0] = +a;
    points[1][1] = -a;
    points[2][0] = +a;
    points[2][1] = +a;
    points[3][0] = -a;
    points[3][1] = +a;

    segments[0][0] = 0;
    segments[0][1] = 1;
    segments[1][0] = 1;
    segments[1][1] = 2;
    segments[2][0] = 2;
    segments[2][1] = 3;
    segments[3][0] = 3;
    segments[3][1] = 0;

    // Irregular boundary line.
    for (uint32_t i = 0; i < N; i++)
    {
        points[4 + i][0] = a * (-1.0 + 2.0 * (float)i / (N - 1));
        points[4 + i][1] = b * (-1.0 + 2.0 * (i % 2));
        // if (i == N / 2)
        //     points[4 + i][1] = 0;

        if (i < N - 1)
        {
            ASSERT(4 + i < segment_count);
            ASSERT(4 + i + 1 < point_count);
            segments[4 + i][0] = 4 + i + 0;
            segments[4 + i][1] = 4 + i + 1;
        }
    }

    // Regions.
    const uint32_t region_count = 2;
    const dvec2 region_coords[] = {{0, -.5}, {0, +.5}};

    // Region colors.
    VkyColor region_colors[] = {
        {{128, 128, 128}, 128}, // region 0 = no region
        vky_color(VKY_CMAP_JET, 1, 0, 10, .5),
        vky_color(VKY_CMAP_JET, 4, 0, 10, .5),
        {{64, 64, 64}, 255}, // DEBUG: extra triangles
    };

    // PSLG visual.
    VkyPSLGParams params = (VkyPSLGParams){5, {{0, 0, 0}, 255}};
    VkyVisual* vb = vky_visual_pslg(panel->scene, &params);
    vky_add_visual_to_panel(vb, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
    VkyPSLGTriangulation tr = vky_visual_pslg_upload(
        vb,                                         // visual
        point_count, (const dvec2*)points,          // points
        segment_count, (const uvec2*)segments,      // segments
        region_count, region_coords, region_colors, // regions,
        "pzqAQa0.002");                             // triangle params

    // Triangulation visual.
    VkyTriangulationParams tparams =
        (VkyTriangulationParams){2, {{0, 0, 0}, 255}, {8, 8}, {{0, 0, 0}, 255}};
    VkyVisual* vbt = vky_visual_triangulation(panel->scene, &tparams);
    vky_add_visual_to_panel(vbt, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
    vky_visual_triangulation_upload(
        vbt, tr.vertex_count, sizeof(VkyVertex), tr.mesh_vertices, tr.index_count, tr.indices);
    vky_destroy_pslg_triangulation(&tr);
}

static void france(VkyPanel* panel)
{
    vky_clear_color(panel->scene, VKY_CLEAR_COLOR_WHITE);

    const uint32_t point_count = 31244;
    dvec2* points = NULL;
    uint32_t poly_count = 1;

    // Load the France points.
    char path[1024];
    snprintf(path, sizeof(path), "%s/misc/departements.polypoints.bin", DATA_DIR);
    points = (dvec2*)read_file(path, NULL);

    // Load the polygon lengths.
    snprintf(path, sizeof(path), "%s/misc/departements.polylengths.bin", DATA_DIR);
    uint32_t* poly_lengths = (uint32_t*)read_file(path, NULL);
    poly_count = 131;

    // Convert longitudes/latitudes to pixels.
    vky_earth_to_pixels(point_count, points);

    // Polygon colors
    VkyColor* poly_colors = calloc(poly_count, sizeof(VkyColor));
    for (uint32_t i = 0; i < poly_count; i++)
    {
        poly_colors[i] = vky_color(VKY_CPAL256_GLASBEY, i % 256, 0, poly_count, 1);
    }

    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);
    vky_set_panel_aspect_ratio(panel, 1);

    // Create the polygon visual.
    VkyPolygonParams params = (VkyPolygonParams){2, {{0, 0, 0}, 255}};
    VkyVisual* vb = vky_visual_polygon(panel->scene, &params);
    vky_add_visual_to_panel(vb, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    VkyPolygonTriangulation tr = vky_visual_polygon_upload(
        vb,                                // visual
        point_count, (const dvec2*)points, // points
        poly_count, poly_lengths,          // polygons
        poly_colors                        // polygon colors
    );


    // Triangulation visual.
    if (0) // NOTE: disable for now
    {
        VkyTriangulationParams tparams =
            (VkyTriangulationParams){2, {{0, 0, 0}, 255}, {6, 6}, {{0, 0, 0}, 255}};
        VkyVisual* vbt = vky_visual_triangulation(panel->scene, &tparams);
        vky_add_visual_to_panel(vbt, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
        vky_visual_triangulation_upload(
            vbt, point_count, sizeof(VkyVertex), tr.mesh_vertices, tr.index_count, tr.indices);
        vky_destroy_polygon_triangulation(&tr);
    }

    // Cleanup
    free(points);
    free(poly_lengths);
    free(poly_colors);
}
