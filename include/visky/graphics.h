/*************************************************************************************************/
/*  Collection of builtin graphics pipelines                                                     */
/*************************************************************************************************/

#ifndef VKL_GRAPHICS_HEADER
#define VKL_GRAPHICS_HEADER

#include "array.h"
#include "canvas.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Constants and macros                                                                         */
/*************************************************************************************************/

// Number of common bindings
// NOTE: must correspond to the same constant in common.glsl
#define VKL_USER_BINDING 2

#define VKL_MAX_GLYPHS_PER_TEXT 256



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Graphics flags.
typedef enum
{
    VKL_GRAPHICS_FLAGS_DEPTH_TEST_DISABLE = 0x0000,
    VKL_GRAPHICS_FLAGS_DEPTH_TEST_ENABLE = 0x0100,
} VklGraphicsFlags;



// Marker type.
// NOTE: the numbers need to correspond to markers.glsl at the bottom.
typedef enum
{
    VKL_MARKER_DISC = 0,
    VKL_MARKER_ASTERISK = 1,
    VKL_MARKER_CHEVRON = 2,
    VKL_MARKER_CLOVER = 3,
    VKL_MARKER_CLUB = 4,
    VKL_MARKER_CROSS = 5,
    VKL_MARKER_DIAMOND = 6,
    VKL_MARKER_ARROW = 7,
    VKL_MARKER_ELLIPSE = 8,
    VKL_MARKER_HBAR = 9,
    VKL_MARKER_HEART = 10,
    VKL_MARKER_INFINITY = 11,
    VKL_MARKER_PIN = 12,
    VKL_MARKER_RING = 13,
    VKL_MARKER_SPADE = 14,
    VKL_MARKER_SQUARE = 15,
    VKL_MARKER_TAG = 16,
    VKL_MARKER_TRIANGLE = 17,
    VKL_MARKER_VBAR = 18,
} VklMarkerType;



// Cap type.
typedef enum
{
    VKL_CAP_TYPE_NONE = 0,
    VKL_CAP_ROUND = 1,
    VKL_CAP_TRIANGLE_IN = 2,
    VKL_CAP_TRIANGLE_OUT = 3,
    VKL_CAP_SQUARE = 4,
    VKL_CAP_BUTT = 5,
    VKL_CAP_COUNT,
} VklCapType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklVertex VklVertex;

typedef struct VklGraphicsPointParams VklGraphicsPointParams;

typedef struct VklGraphicsMarkerVertex VklGraphicsMarkerVertex;
typedef struct VklGraphicsMarkerParams VklGraphicsMarkerParams;

typedef struct VklGraphicsSegmentVertex VklGraphicsSegmentVertex;

typedef struct VklGraphicsImageVertex VklGraphicsImageVertex;
typedef struct VklGraphicsImageParams VklGraphicsImageParams;

typedef struct VklGraphicsVolumeSliceItem VklGraphicsVolumeSliceItem;
typedef struct VklGraphicsVolumeSliceVertex VklGraphicsVolumeSliceVertex;
typedef struct VklGraphicsVolumeSliceParams VklGraphicsVolumeSliceParams;

typedef struct VklGraphicsVolumeItem VklGraphicsVolumeItem;
typedef struct VklGraphicsVolumeVertex VklGraphicsVolumeVertex;
typedef struct VklGraphicsVolumeParams VklGraphicsVolumeParams;

typedef struct VklGraphicsMeshVertex VklGraphicsMeshVertex;
typedef struct VklGraphicsMeshParams VklGraphicsMeshParams;

typedef struct VklGraphicsTextParams VklGraphicsTextParams;
typedef struct VklGraphicsTextVertex VklGraphicsTextVertex;
typedef struct VklGraphicsTextItem VklGraphicsTextItem;

typedef struct VklGraphicsData VklGraphicsData;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklVertex
{
    vec3 pos;
    cvec4 color;
};



struct VklGraphicsData
{
    VklGraphics* graphics;
    VklArray* vertices;
    VklArray* indices;
    uint32_t item_count;
    uint32_t current_idx;
    uint32_t current_group;
    void* user_data;
};



/*************************************************************************************************/
/*  Graphics points                                                                              */
/*************************************************************************************************/

struct VklGraphicsPointParams
{
    float point_size;
};



/*************************************************************************************************/
/*  Graphics marker                                                                              */
/*************************************************************************************************/

struct VklGraphicsMarkerVertex
{
    vec3 pos;
    cvec4 color;
    float size;
    // in fact a VklMarkerType but we should control the exact data type for the GPU
    uint8_t marker;
    uint8_t angle;
    uint8_t transform;
};

struct VklGraphicsMarkerParams
{
    vec4 edge_color;
    float edge_width;
};



/*************************************************************************************************/
/*  Graphics segment                                                                             */
/*************************************************************************************************/

struct VklGraphicsSegmentVertex
{
    vec3 P0;
    vec3 P1;
    vec4 shift;
    cvec4 color;
    float linewidth;
    VklCapType cap0;
    VklCapType cap1;
    uint8_t transform;
};



/*************************************************************************************************/
/*  Graphics text                                                                                */
/*************************************************************************************************/

struct VklGraphicsTextVertex
{
    vec3 pos;
    vec2 shift;
    cvec4 color;
    vec2 glyph_size;
    vec2 anchor;
    float angle;
    usvec4 glyph; // char, char_index, str_len, str_index
    uint8_t transform;
};

struct VklGraphicsTextItem
{
    VklGraphicsTextVertex vertex;
    cvec4* glyph_colors;
    float font_size;
    const char* string;
};

struct VklGraphicsTextParams
{
    ivec2 grid_size;
    ivec2 tex_size;
};



/*************************************************************************************************/
/*  Graphics image                                                                               */
/*************************************************************************************************/

struct VklGraphicsImageVertex
{
    vec3 pos;
    vec2 uv;
};

struct VklGraphicsImageParams
{
    vec4 tex_coefs; // blending coefficients for the textures
};



/*************************************************************************************************/
/*  Graphics volume slice                                                                        */
/*************************************************************************************************/

struct VklGraphicsVolumeSliceItem
{
    // top left, top right, bottom right, bottom left
    vec3 pos0, pos1, pos2, pos3;
    vec3 uvw0, uvw1, uvw2, uvw3;
};

struct VklGraphicsVolumeSliceVertex
{
    vec3 pos;
    vec3 uvw;
};

struct VklGraphicsVolumeSliceParams
{
    vec4 x_cmap;
    vec4 y_cmap;
    vec4 x_alpha;
    vec4 y_alpha;
    int32_t cmap;
    float scale;
};



/*************************************************************************************************/
/*  Graphics volume                                                                              */
/*************************************************************************************************/

struct VklGraphicsVolumeItem
{
    // top left front, bottom right back
    vec3 pos0, pos1;
    vec3 uvw0, uvw1;
};

struct VklGraphicsVolumeVertex
{
    vec3 pos;
    vec3 uvw;
};

struct VklGraphicsVolumeParams
{
    vec4 view_pos;
    vec4 box_size;
    int32_t cmap;
};



/*************************************************************************************************/
/*  Graphics mesh                                                                                */
/*************************************************************************************************/

struct VklGraphicsMeshVertex
{
    vec3 pos;
    vec3 normal;
    vec2 uv;
    uint8_t alpha;
};

struct VklGraphicsMeshParams
{
    mat4 lights_pos_0;    // lights 0-3
    mat4 lights_params_0; // for each light, coefs for ambient, diffuse, specular
    vec4 view_pos;        // view position
    vec4 tex_coefs;       // blending coefficients for the textures
    vec4 clip_coefs;      // dot product of this vector with the vertex position < 0 => discard
};

static VklGraphicsMeshParams default_graphics_mesh_params(vec3 eye)
{
    VklGraphicsMeshParams params = {0};
    params.lights_params_0[0][0] = 0.2;
    params.lights_params_0[0][1] = 0.5;
    params.lights_params_0[0][2] = 0.3;
    params.lights_pos_0[0][0] = -1;
    params.lights_pos_0[0][1] = 1;
    params.lights_pos_0[0][2] = +10;
    params.tex_coefs[0] = 1;
    glm_vec3_copy(eye, params.view_pos);
    return params;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Set a graphics data callback.
 *
 * The callback function is called when one calls `vkl_graphics_append()` on that visual. It allows
 * one to easily add graphical elements, letting the graphics handle low-level GPU implementation
 * details (tesselation with vertices).
 *
 * Callback function signature: `void(VklGraphicsData*, uint32_t, const void*)`
 *
 * @param graphics the graphics pipeline
 * @param callback the callback function
 */
VKY_EXPORT void vkl_graphics_callback(VklGraphics* graphics, VklGraphicsCallback callback);

/**
 * Start a data collection for a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param vertices pointer to an existing array containing vertices of the right type
 * @param indices pointer to an existing array containing the indices
 * @param user_data arbitrary user-provided pointer
 * @returns the graphics data object
 */
VKY_EXPORT VklGraphicsData
vkl_graphics_data(VklGraphics* graphics, VklArray* vertices, VklArray* indices, void* user_data);

/**
 * Allocate the graphics data object with the appropriate number of elements.
 *
 * @param data the graphics data object
 * @param item_count the number of graphical items
 */
VKY_EXPORT void vkl_graphics_alloc(VklGraphicsData* data, uint32_t item_count);

/**
 * Add one graphical element after the graphics data object has been properly allocated.
 *
 * @param data the graphics data object
 * @param item a pointer to an object of the appropriate graphics item type
 */
VKY_EXPORT void vkl_graphics_append(VklGraphicsData* data, const void* item);

/**
 * Create a new graphics pipeline of a given builtin type.
 *
 * @param canvas the canvas holding the grahpics pipeline
 * @param type the graphics type
 * @param flags the creation flags for the graphics
 */
VKY_EXPORT VklGraphics* vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsType type, int flags);



/**
 * Set up a 3D camera on a Model-View-Projection (MVP) object.
 *
 * @param viewport the viewport
 * @param eye the camera position in scene coordinates
 * @param center the position the camera points to
 * @param near_far the near and far values for the perspective matrix
 * @param[out] mvp a pointer to an MVP object
 */
VKY_EXPORT void
vkl_mvp_camera(VklViewport viewport, vec3 eye, vec3 center, vec2 near_far, VklMVP* mvp);


#endif
