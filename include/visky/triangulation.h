#ifndef VKY_TRIANGULATION_HEADER
#define VKY_TRIANGULATION_HEADER

#include <visky/visky.h>

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Structures                                                                                   */
/*************************************************************************************************/

typedef struct VkyVertex VkyVertex;

typedef struct triangulateio triangulateio; // forward declaration

typedef struct VkyPSLGTriangulation VkyPSLGTriangulation;
struct VkyPSLGTriangulation
{
    uint32_t vertex_count;          // vertices in the triangulation
    dvec2* vertices;                // TODO: rename into points?
    uint32_t index_count;           // indices making the triangles, 3 indices = 1 triangle
    VkyIndex* indices;              //
    double* region_idx;             // region index for each vertex
    struct triangulateio* triangle; // Triangle library struct
    VkyVertex* mesh_vertices;
};

typedef struct VkyPolygonTriangulation VkyPolygonTriangulation;
struct VkyPolygonTriangulation
{
    uint32_t index_count; // indices making the triangles, 3 indices = 1 triangle
    VkyIndex* indices;    //
    VkyVertex* mesh_vertices;
};


/*************************************************************************************************/
/*  Visual upload                                                                                */
/*************************************************************************************************/

VKY_EXPORT VkyPolygonTriangulation vky_visual_polygon_upload(
    VkyVisual* vb,                                           // visual
    const uint32_t point_count, const dvec2* points,         // points
    const uint32_t poly_count, const uint32_t* poly_lengths, // polygons
    const VkyColor* poly_colors                              // polygon colors
);


VKY_EXPORT VkyPSLGTriangulation vky_visual_pslg_upload(
    VkyVisual* vb,                //
    const uint32_t, const dvec2*, // points
    const uint32_t, const uvec2*, // segments
    const uint32_t, const dvec2*, // regions
    const VkyColor*,              // region colors
    const char*);                 // triangle params



/*************************************************************************************************/
/*  Earcut polygon triangulation                                                                 */
/*************************************************************************************************/

// Ear-clip algorithm
VKY_EXPORT void vky_triangulate_polygon(
    const uint32_t, const dvec2*, // points
    uint32_t*, uint32_t**);       // triangulation

// Triangulation each polygon with the ear-clip algorithm.
VKY_EXPORT VkyPolygonTriangulation vky_triangulate_polygons(
    const uint32_t, const dvec2*,     // points
    const uint32_t, const uint32_t*); // polygons

VKY_EXPORT void vky_destroy_polygon_triangulation(VkyPolygonTriangulation*);



/*************************************************************************************************/
/*  PSLG Delaunay triangulation with Triangle                                                    */
/*************************************************************************************************/

// Delaunay triangulation with the Triangle C library
VKY_EXPORT VkyPSLGTriangulation vky_triangulate_pslg(
    const uint32_t, const dvec2*, // points
    const uint32_t, const uvec2*, // segments
    const uint32_t, const dvec2*, // regions
    const char* triangle_params);

VKY_EXPORT void vky_destroy_pslg_triangulation(VkyPSLGTriangulation*);



#ifdef __cplusplus
}
#endif

#endif
