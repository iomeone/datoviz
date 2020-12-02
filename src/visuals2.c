#include "../include/visky/visuals2.h"
#include "../include/visky/canvas.h"
#include "../include/visky/graphics.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _default_visual_fill(VklVisual* visual, VklVisualFillEvent ev)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    VklCommands* cmds = ev.cmds;
    uint32_t idx = ev.cmd_idx;
    VkViewport viewport = ev.viewport.viewport;

    ASSERT(viewport.width > 0);
    ASSERT(viewport.height > 0);
    ASSERT(is_obj_created(&visual->graphics[0]->obj));
    ASSERT(is_obj_created(&visual->gbindings[0].obj));
    ASSERT(visual->vertex_buf.size > 0);
    ASSERT(visual->vertex_count > 0);

    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    vkl_cmd_viewport(cmds, idx, viewport);
    vkl_cmd_bind_vertex_buffer(cmds, idx, &visual->vertex_buf, 0);
    vkl_cmd_bind_graphics(cmds, idx, visual->graphics[0], &visual->gbindings[0], 0);
    vkl_cmd_draw(cmds, idx, 0, visual->vertex_count);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VklVisual vkl_visual(VklCanvas* canvas)
{
    VklVisual visual = {0};
    visual.canvas = canvas;
    // Default fill callback.
    visual.fill_callback = _default_visual_fill;
    obj_created(&visual.obj);
    return visual;
}



void vkl_visual_destroy(VklVisual* visual)
{
    ASSERT(visual != NULL);
    obj_destroyed(&visual->obj);
}



/*************************************************************************************************/
/*  Visual creation                                                                              */
/*************************************************************************************************/

static VkDeviceSize _get_dtype_size(VklDataType dtype)
{
    switch (dtype)
    {
    case VKL_DTYPE_CHAR:
        return 1;
    case VKL_DTYPE_CVEC2:
        return 1 * 2;
    case VKL_DTYPE_CVEC3:
        return 2 * 3;
    case VKL_DTYPE_CVEC4:
        return 3 * 4;


    case VKL_DTYPE_FLOAT:
    case VKL_DTYPE_UINT:
    case VKL_DTYPE_INT:
        return 4;

    case VKL_DTYPE_VEC2:
    case VKL_DTYPE_UVEC2:
    case VKL_DTYPE_IVEC2:
        return 4 * 2;

    case VKL_DTYPE_VEC3:
    case VKL_DTYPE_UVEC3:
    case VKL_DTYPE_IVEC3:
        return 4 * 3;

    case VKL_DTYPE_VEC4:
    case VKL_DTYPE_UVEC4:
    case VKL_DTYPE_IVEC4:
        return 4 * 4;


    case VKL_DTYPE_DOUBLE:
        return 8;
    case VKL_DTYPE_DVEC2:
        return 8 * 2;
    case VKL_DTYPE_DVEC3:
        return 8 * 3;
    case VKL_DTYPE_DVEC4:
        return 8 * 4;

    default:
        break;
    }

    log_error("could not find the size of dtype %d", dtype);
    return 0;
}

void vkl_visual_prop(
    VklVisual* visual, VklPropType prop, uint32_t idx, VklDataType dtype, VklPropLoc loc,
    uint32_t binding_idx, uint32_t field_idx, VkDeviceSize offset)
{
    ASSERT(visual != NULL);
    if (visual->source_count >= VKL_MAX_VISUAL_SOURCES)
    {
        log_error("maximum number of props per visual reached");
        return;
    }
    VklSource source = {0};
    source.prop = prop;
    source.prop_idx = idx;
    source.dtype = dtype;
    source.dtype_size = _get_dtype_size(dtype);
    source.loc = loc;
    source.binding_idx = binding_idx;
    source.field_idx = field_idx;
    source.offset = offset;
    visual->sources[visual->source_count++] = source;
}



void vkl_visual_graphics(VklVisual* visual, VklGraphics* graphics)
{
    ASSERT(visual != NULL);
    ASSERT(graphics != NULL);
    ASSERT(is_obj_created(&graphics->obj));
    if (visual->graphics_count >= VKL_MAX_GRAPHICS_PER_VISUAL)
    {
        log_error("maximum number of graphics per visual reached");
        return;
    }
    visual->graphics[visual->graphics_count++] = graphics;
}



void vkl_visual_compute(VklVisual* visual, VklCompute* compute)
{
    ASSERT(visual != NULL);
    ASSERT(compute != NULL);
    ASSERT(is_obj_created(&compute->obj));
    if (visual->compute_count >= VKL_MAX_COMPUTES_PER_VISUAL)
    {
        log_error("maximum number of computes per visual reached");
        return;
    }
    visual->computes[visual->compute_count++] = compute;
}



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

void vkl_visual_size(VklVisual* visual, uint32_t item_count, uint32_t group_count)
{
    ASSERT(visual != NULL);
    visual->item_count = item_count;
    visual->group_count = group_count;
}



void vkl_visual_group(VklVisual* visual, uint32_t group_idx, uint32_t size)
{
    ASSERT(visual != NULL);
    if (group_idx >= VKL_MAX_VISUAL_GROUPS)
    {
        log_error("maximum number of groups reached");
        return;
    }
    visual->group_count = MAX(visual->group_count, group_idx + 1);
    visual->group_sizes[group_idx] = size;
}



void vkl_visual_data(VklVisual* visual, VklPropType type, uint32_t idx, const void* data)
{
    ASSERT(visual != NULL);
    vkl_visual_data_partial(visual, type, idx, 0, visual->item_count, data);
}



static VklSource* _get_source(VklVisual* visual, VklPropType type, uint32_t idx)
{
    for (uint32_t i = 0; i < visual->source_count; i++)
    {
        if (visual->sources[i].prop == type && visual->sources[i].prop_idx == idx)
            return &visual->sources[i];
    }
    log_error("Data source for prop %d #%d could not be found", type, idx);
    return NULL;
}



void vkl_visual_data_partial(
    VklVisual* visual, VklPropType type, uint32_t idx, uint32_t first_item, uint32_t item_count,
    const void* data)
{
    ASSERT(visual != NULL);
    VklSource* source = _get_source(visual, type, idx);
    ASSERT(source != NULL);
    ASSERT(source->dtype_size > 0);

    source->binding = VKL_PROP_BINDING_CPU;
    source->u.a.offset = first_item * source->dtype_size;
    source->u.a.size = item_count * source->dtype_size;
    source->u.a.data = data;
}



void vkl_visual_data_buffer(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    VklSource* source = _get_source(visual, type, idx);
    ASSERT(source != NULL);
    ASSERT(source->dtype_size > 0);

    source->binding = VKL_PROP_BINDING_BUFFER;
    source->u.b.br = br;
    source->u.b.offset = offset;
    source->u.b.size = size;
}



void vkl_visual_data_texture(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    VklTexture* texture, uvec3 offset, uvec3 shape)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    VklSource* source = _get_source(visual, type, idx);
    ASSERT(source != NULL);
    ASSERT(source->dtype_size > 0);

    source->binding = VKL_PROP_BINDING_TEXTURE;
    source->u.t.texture = texture;
    for (uint32_t i = 0; i < 3; i++)
    {
        source->u.t.offset[i] = offset[i];
        source->u.t.shape[i] = shape[i];
    }
}



/*************************************************************************************************/
/*  Visual events                                                                                */
/*************************************************************************************************/

void vkl_visual_data_callback(VklVisual* visual, VklVisualDataCallback callback)
{
    ASSERT(visual != NULL);
    visual->data_callback = callback;
}



void vkl_visual_data_event(VklVisual* visual)
{
    ASSERT(visual != NULL);
    ASSERT(visual->data_callback != NULL);
    // TODO
}



void vkl_visual_fill_callback(VklVisual* visual, VklVisualFillCallback callback)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    visual->fill_callback = callback;
}



void vkl_visual_fill_event(
    VklVisual* visual, VkClearColorValue clear_color, VklCommands* cmds, uint32_t cmd_idx,
    VklViewport viewport, void* user_data)
{
    // Called in a REFILL canvas callback.

    ASSERT(visual != NULL);
    ASSERT(visual->fill_callback != NULL);

    VklVisualFillEvent ev = {0};
    ev.clear_color = clear_color;
    ev.cmds = cmds;
    ev.cmd_idx = cmd_idx;
    ev.viewport = viewport;
    ev.user_data = user_data;

    visual->fill_callback(visual, ev);
    visual->canvas->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
}