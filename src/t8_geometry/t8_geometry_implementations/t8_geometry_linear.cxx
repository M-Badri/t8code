/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element classes in parallel.

  Copyright (C) 2015 the developers

  t8code is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  t8code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with t8code; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <t8_geometry/t8_geometry_implementations/t8_geometry_linear.hxx>
#include <t8_geometry/t8_geometry_implementations/t8_geometry_linear.h>
#include <t8_geometry/t8_geometry_helpers.h>
#include <t8_schemes/t8_default/t8_default.hxx>
#include <t8_types/t8_vec.hxx>

t8_geometry_linear::t8_geometry_linear (): t8_geometry_with_vertices ("t8_geom_linear")
{
}

t8_geometry_linear::~t8_geometry_linear ()
{
}

void
t8_geometry_linear::t8_geom_evaluate ([[maybe_unused]] t8_cmesh_t cmesh, [[maybe_unused]] t8_gloidx_t gtreeid,
                                      const double *ref_coords, const size_t num_coords, double *out_coords) const
{
  t8_geom_compute_linear_geometry (active_tree_class, active_tree_vertices, ref_coords, num_coords, out_coords);
}

void
t8_geometry_linear::t8_geom_evaluate_jacobian ([[maybe_unused]] t8_cmesh_t cmesh, [[maybe_unused]] t8_gloidx_t gtreeid,
                                               [[maybe_unused]] const double *ref_coords,
                                               [[maybe_unused]] const size_t num_coords,
                                               [[maybe_unused]] double *jacobian) const
{
  SC_ABORT ("Not implemented.");
}

#if T8_ENABLE_DEBUG
/* Test whether four given points in 3D are coplanar up to a given tolerance.
 */
static int
t8_four_points_coplanar (const t8_3D_vec p_0, const t8_3D_vec p_1, const t8_3D_vec p_2, const t8_3D_vec p_3,
                         const double tolerance)
{
  /* Let p0, p1, p2, p3 be the four points.
   * The four points are coplanar if the normal vectors to the triangles
   * p0, p1, p2 and p0, p2, p3 are pointing in the same direction.
   *
   * We build the vectors A = p1 - p0, B = p2 - p0 and C = p3 - p0.
   * The normal vectors to the triangles are n1 = A x B and n2 = A x C.
   * These are pointing in the same direction if their cross product is 0.
   * Hence we check if || n1 x n2 || < tolerance. */

  /* A = p1 - p0 */
  t8_3D_vec A;
  t8_axpyz (p_0, p_1, A, -1);

  /* B = p2 - p0 */
  t8_3D_vec B;
  t8_axpyz (p_0, p_2, B, -1);

  /* C = p3 - p0 */
  t8_3D_vec C;
  t8_axpyz (p_0, p_3, C, -1);

  /* n1 = A x B */
  t8_3D_vec A_cross_B;
  t8_cross_3D (A, B, A_cross_B);

  /* n2 = A x C */
  t8_3D_vec A_cross_C;
  t8_cross_3D (A, C, A_cross_C);

  /* n1 x n2 */
  t8_3D_vec n1_cross_n2;
  t8_cross_3D (A_cross_B, A_cross_C, n1_cross_n2);

  /* || n1 x n2 || */
  const double norm = t8_norm (n1_cross_n2);
  return norm < tolerance;
}
#endif

void
t8_geometry_linear::t8_geom_point_batch_inside_element (t8_forest_t forest, t8_locidx_t ltreeid,
                                                        const t8_element_t *element, const double *points,
                                                        const int num_points, int *is_inside,
                                                        const double tolerance) const
{
  const t8_eclass_t tree_class = t8_forest_get_tree_class (forest, ltreeid);
  const t8_scheme *scheme = t8_forest_get_scheme (forest);
  const t8_element_shape_t element_shape = scheme->element_get_shape (tree_class, element);
  switch (element_shape) {
  case T8_ECLASS_VERTEX: {
    /* A point is 'inside' a vertex if they have the same coordinates */
    double vertex_coords[3];
    /* Get the vertex coordinates */
    t8_forest_element_coordinate (forest, ltreeid, element, 0, vertex_coords);
    /* Check whether the point and the vertex are within tolerance distance
       * to each other */
    for (int ipoint = 0; ipoint < num_points; ipoint++) {
      is_inside[ipoint] = t8_vertex_point_inside (vertex_coords, &points[ipoint * 3], tolerance);
    }
    return;
  }
  case T8_ECLASS_LINE: {
    /* A point p is inside a line that is defined by the edge nodes
     * p_0 and p_1
     * if and only if the linear system
     * (p_1 - p_0)x = p - p_0
     * has a solution x with 0 <= x <= 1
     */
    t8_3D_vec p_0, v;

    /* Compute the vertex coordinates of the line */
    t8_forest_element_coordinate (forest, ltreeid, element, 0, p_0.data ());
    /* v = p_1 */
    t8_forest_element_coordinate (forest, ltreeid, element, 1, v.data ());
    /* v = p_1 - p_0 */
    t8_axpy (p_0, v, -1);
    for (int ipoint = 0; ipoint < num_points; ipoint++) {
      is_inside[ipoint] = t8_line_point_inside (p_0.data (), v.data (), &points[ipoint * 3], tolerance);
    }
    return;
  }
  case T8_ECLASS_QUAD: {
    /* We divide the quad in two triangles and use the triangle check. */
    t8_3D_vec p_0, p_1, p_2, p_3;
    /* Compute the vertex coordinates of the quad */
    t8_forest_element_coordinate (forest, ltreeid, element, 0, p_0.data ());
    t8_forest_element_coordinate (forest, ltreeid, element, 1, p_1.data ());
    t8_forest_element_coordinate (forest, ltreeid, element, 2, p_2.data ());
    t8_forest_element_coordinate (forest, ltreeid, element, 3, p_3.data ());

#if T8_ENABLE_DEBUG
    /* Issue a warning if the points of the quad do not lie in the same plane */
    if (!t8_four_points_coplanar (p_0, p_1, p_2, p_3, tolerance)) {
      t8_debugf ("WARNING: Testing if point is inside a quad that is not coplanar. This test will be inaccurate.\n");
    }
#endif
    t8_3D_vec v;
    t8_3D_vec w;
    /* v = v - p_0 = p_1 - p_0 */
    t8_axpyz (p_0, p_1, v, -1);
    /* w = w - p_0 = p_2 - p_0 */
    t8_axpyz (p_0, p_2, w, -1);
    /* Check whether the point is inside the first triangle. */
    for (int ipoint = 0; ipoint < num_points; ipoint++) {
      is_inside[ipoint] = t8_triangle_point_inside (p_0.data (), v.data (), w.data (), &points[ipoint * 3], tolerance);
    }
    /* If not, check whether the point is inside the second triangle. */
    /* v = v - p_0 = p_1 - p_0 */
    t8_axpyz (p_1, p_2, v, -1);
    /* w = w - p_0 = p_2 - p_0 */
    t8_axpyz (p_1, p_3, w, -1);
    for (int ipoint = 0; ipoint < num_points; ipoint++) {
      if (!is_inside[ipoint]) {
        /* point_inside is true if the point was inside the first or second triangle. Otherwise it is false. */
        is_inside[ipoint]
          = t8_triangle_point_inside (p_1.data (), v.data (), w.data (), &points[ipoint * 3], tolerance);
      }
    }
    return;
  }
  case T8_ECLASS_TRIANGLE: {
    t8_3D_vec p_0, p_1, p_2;

    /* Compute the vertex coordinates of the triangle */
    t8_forest_element_coordinate (forest, ltreeid, element, 0, p_0.data ());
    t8_forest_element_coordinate (forest, ltreeid, element, 1, p_1.data ());
    t8_forest_element_coordinate (forest, ltreeid, element, 2, p_2.data ());
    t8_3D_vec v;
    t8_3D_vec w;
    /* v = v - p_0 = p_1 - p_0 */
    t8_axpyz (p_0, p_1, v, -1);
    /* w = w - p_0 = p_2 - p_0 */
    t8_axpyz (p_0, p_2, w, -1);

    for (int ipoint = 0; ipoint < num_points; ipoint++) {
      is_inside[ipoint] = t8_triangle_point_inside (p_0.data (), v.data (), w.data (), &points[ipoint * 3], tolerance);
    }
    return;
  }
  case T8_ECLASS_TET:
  case T8_ECLASS_HEX:
  case T8_ECLASS_PRISM:
  case T8_ECLASS_PYRAMID: {
    /* For bilinearly interpolated volume elements, a point is inside an element
     * if and only if it lies on the inner side of each face.
     * The inner side is defined as the side where the outside normal vector does not
     * point to.
     * The point is on this inner side if and only if the scalar product of
     * a point on the plane minus the point with the outer normal of the face
     * is >= 0.
     *
     * In other words, let p be the point to check, n the outer normal and x a point
     * on the plane, then p is on the inner side if and only if
     *  <x - p, n> >= 0
     */

    const int num_faces = scheme->element_get_num_faces (tree_class, element);
    /* Assume that every point is inside of the element */
    for (int ipoint = 0; ipoint < num_points; ipoint++) {
      is_inside[ipoint] = 1;
    }
    for (int iface = 0; iface < num_faces; ++iface) {
      double face_normal[3];
      /* Compute the outer normal n of the face */
      t8_forest_element_face_normal (forest, ltreeid, element, iface, face_normal);
      /* Compute a point x on the face */
      const int afacecorner = scheme->element_get_face_corner (tree_class, element, iface, 0);
      double point_on_face[3];
      t8_forest_element_coordinate (forest, ltreeid, element, afacecorner, point_on_face);
      for (int ipoint = 0; ipoint < num_points; ipoint++) {
        const int is_inside_iface = t8_plane_point_inside (point_on_face, face_normal, &points[ipoint * 3]);
        if (is_inside_iface == 0) {
          /* Point is on the outside of face iface. Update is_inside */
          is_inside[ipoint] = 0;
        }
      }
    }
    return;
  }
  default:
    SC_ABORT_NOT_REACHED ();
  }
}

inline bool
t8_geometry_linear::get_tree_bounding_box ([[maybe_unused]] const t8_cmesh_t cmesh, double bounds[6]) const
{
  T8_ASSERT (cmesh != NULL);
  T8_ASSERT (active_tree_vertices != NULL);
  /* For linear geometry the bounding box is determined by the minimum/maximum occurring
   * vertex coordinates. */
  /* Set bounds to the first vertex */
  bounds[0] = active_tree_vertices[0];
  bounds[1] = active_tree_vertices[0];
  bounds[2] = active_tree_vertices[1];
  bounds[3] = active_tree_vertices[1];
  bounds[4] = active_tree_vertices[2];
  bounds[5] = active_tree_vertices[2];
  const int num_vertices = t8_eclass_num_vertices[active_tree_class];
  T8_ASSERT (num_vertices > 0);
  /* iterate over all vertices in the tree and update bounds */
  for (int ivertex = 1; ivertex < num_vertices; ++ivertex) {
    bounds[0] = std::min (bounds[0], active_tree_vertices[3 * ivertex]);
    bounds[1] = std::max (bounds[1], active_tree_vertices[3 * ivertex]);
    bounds[2] = std::min (bounds[2], active_tree_vertices[3 * ivertex + 1]);
    bounds[3] = std::max (bounds[3], active_tree_vertices[3 * ivertex + 1]);
    bounds[4] = std::min (bounds[4], active_tree_vertices[3 * ivertex + 2]);
    bounds[5] = std::max (bounds[5], active_tree_vertices[3 * ivertex + 2]);
  }
  return true;
}

T8_EXTERN_C_BEGIN ();

/* Satisfy the C interface from t8_geometry_linear.h.
 * Create a new geometry. */
t8_geometry_c *
t8_geometry_linear_new ()
{
  t8_geometry_linear *geom = new t8_geometry_linear ();
  return (t8_geometry_c *) geom;
}

void
t8_geometry_linear_destroy (t8_geometry_c **geom)
{
  T8_ASSERT (geom != NULL);
  T8_ASSERT ((*geom)->t8_geom_get_type () == T8_GEOMETRY_TYPE_LINEAR);

  delete *geom;
  *geom = NULL;
}

T8_EXTERN_C_END ();
