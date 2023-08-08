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

#include <example/vtk/interpolate.hxx>

int
t8_adapt_callback_non_empty (t8_forest_t forest,
                             t8_forest_t forest_from,
                             t8_locidx_t ltree_id,
                             t8_locidx_t lelement_id,
                             t8_eclass_scheme_c *ts,
                             const int is_family,
                             const int num_elements, t8_element_t *elements[])
{
  MeshAdapter        *user_data =
    (MeshAdapter *) t8_forest_get_user_data (forest);

  const int           level = ts->t8_element_level (elements[0]);

  /* Single Element and not family */
  if (level == user_data->GetLevelMaximum () && !is_family) {
    /* It is not possible to refine this level */
    return 0;
  }

  const t8_locidx_t   offset =
    t8_forest_get_tree_element_offset (forest_from, ltree_id);

  element_point_t    *elem_point =
    user_data->get_element_point (user_data->GetElementPoints (),
                                  offset + lelement_id);

  const int           num_points = elem_point->num_points;

  if (num_points > 1) {
    return 1;
  }
  else {
    return 0;
  }
}

void
t8_itertate_replace_pointids (t8_forest_t forest_old,
                              t8_forest_t forest_new,
                              t8_locidx_t which_tree,
                              t8_eclass_scheme_c *ts,
                              int refine,
                              int num_outgoing,
                              t8_locidx_t first_outgoing,
                              int num_incoming, t8_locidx_t first_incoming)
{
  /* Get Metadata */
  MeshAdapter        *inter =
    (MeshAdapter *) t8_forest_get_user_data (forest_old);

  T8_ASSERT (forest_old == inter->get_forest ());
  T8_ASSERT (forest_new == inter->get_forest_adapt ());

  /* ID to Data from the new forest (Data will go in there) */
  t8_locidx_t         first_incoming_data =
    first_incoming + t8_forest_get_tree_element_offset (forest_new,
                                                        which_tree);

  /* Data from the old forest (Data is going out) */
  t8_locidx_t         first_outgoing_data =
    first_outgoing + t8_forest_get_tree_element_offset (forest_old,
                                                        which_tree);

  /* Pointer to element data */
  element_point_t    *elem_point_in =
    inter->get_element_point (inter->GetElementPointsAdapt (),
                              first_incoming_data);
  element_point_t    *elem_point_out =
    inter->get_element_point (inter->GetElementPoints (),
                              first_outgoing_data);

  if (refine == 0) {
    /* point_ids array stays the same */
    T8_ASSERT (num_incoming == num_outgoing && num_incoming == 1);

    /* Allocate and copy point_ids array */
    memcpy (elem_point_in, elem_point_out, sizeof (element_point_t));
  }
  else if (refine == 1) {
    /* New offsets and new num_points for each ielem_in */

    /* Temporary array to hold the point-ids for ielem_in */
    sc_array_t        **index = T8_ALLOC (sc_array_t *, num_incoming);
    for (t8_locidx_t ielem = 0; ielem < num_incoming; ielem++) {
      index[ielem] = sc_array_new (sizeof (int));

      element_point_t    *ielem_point_in =
        inter->get_element_point (inter->GetElementPointsAdapt (),
                                  first_incoming_data + ielem);

      ielem_point_in->num_points = 0;
      ielem_point_in->offset = elem_point_out->offset;
    }
    const int           num_outgoing_points = elem_point_out->num_points;
    const int           offset_outgoing = elem_point_out->offset;

    /* Fill the array with the point_ids of elem_out.
     * we pop the id from the list as soon as it is insed of ielem_in */
    sc_array_t         *point_indices =
      sc_array_new_count (sizeof (int), num_outgoing_points);

    for (int ipoint = 0; ipoint < num_outgoing_points; ipoint++) {
      const int           ipoint_id = *((int *)
                                        t8_shmem_array_index
                                        (inter->GetPointIDs (),
                                         ipoint + offset_outgoing));

      int                *point_id =
        (int *) sc_array_index_int (point_indices, ipoint);

      *point_id = ipoint_id;
    }

    for (int ipoint = 0; ipoint < num_outgoing_points; ipoint++) {
      /* Ensures that no points is associated twice. */
      const int           ipoint_id = *((int *) sc_array_pop (point_indices));
      for (t8_locidx_t ielem = 0; ielem < num_incoming; ielem++) {
        t8_element_t       *elem =
          t8_forest_get_element_in_tree (forest_new, which_tree,
                                         first_incoming + ielem);
        element_point_t    *ielem_point_in =
          inter->get_element_point (inter->GetElementPointsAdapt (),
                                    first_incoming_data + ielem);
        double             *vtk_point =
          (double *) t8_shmem_array_index (inter->GetVTKPoints (),
                                           3 * ipoint_id);
        if (t8_forest_element_point_inside
            (forest_new, which_tree, elem, vtk_point, 0.001)) {
          int                *new_point_id =
            (int *) sc_array_push (inter->get_point_id_per_element (ielem));
          *new_point_id = ipoint_id;
          ielem_point_in->num_points++;
          break;
        }
      }
    }
    sc_array_destroy (point_indices);
#if 0
    /* Update ielem_ins offset */
    for (t8_locidx_t ielem = 1; ielem < num_incoming; ielem++) {
      element_point_t    *ielem_point_in =
        inter->get_element_point (inter->GetElementPointsAdapt (),
                                  first_incoming_data + ielem);
      element_point_t    *ielem_point_in_prev =
        inter->get_element_point (inter->GetElementPointsAdapt (),
                                  first_incoming_data + ielem - 1);
      ielem_point_in->offset =
        ielem_point_in_prev->offset + ielem_point_in_prev->num_points;
    }
    /* Update point_ids. */
    //if (t8_shmem_array_start_writing (inter->GetPointIDs ())) {
    for (t8_locidx_t ielem = 0; ielem < num_incoming; ielem++) {
      element_point_t    *ielem_point_in =
        inter->get_element_point (inter->GetElementPointsAdapt (),
                                  first_incoming_data + ielem);
      // t8_debugf("[D] write from: %i to %i\n", ielem_point_in->offset, ielem_point_in->offset + ielem_point_in->num_points);
      t8_locidx_t         element_index =
        t8_forest_get_tree_element_offset (forest_new, which_tree) + ielem;
      for (int ipoint = 0; ipoint < ielem_point_in->num_points; ipoint++) {
        sc_array_t         *points_per_elem =
          adapter->get_point_id_per_element (element_index);
        int                *new_point =
          (int *) sc_array_push (points_per_elem);
        *new_point = *((int *) sc_array_index_int (index[ielem], ipoint));
        /*int                *point_index =
           (int *) t8_shmem_array_index_for_writing (inter->GetPointIDs (),
           ipoint +
           ielem_point_in->offset);
           *point_index = *((int *) sc_array_index_int (index[ielem], ipoint)); */
      }
    }
#endif
    //t8_shmem_array_end_writing (inter->GetPointIDs ());

    for (int ielem = 0; ielem < num_incoming; ielem++) {
      sc_array_destroy (inter->get_point_id_per_element (ielem));
    }
  }
  else {
    /* point-ids array stays the same. Offset of element_in is set to the
     * offset of the first element_out. Sum over the num_points to get the
     * total of points in the coarsend array. */
    elem_point_in->offset = elem_point_out->offset;
    elem_point_in->num_points = 0;
    for (int ielem = 0; ielem < num_outgoing; ielem++) {
      element_point_t    *ielem_point_out =
        inter->get_element_point (inter->GetElementPoints (),
                                  first_outgoing_data + ielem);

      elem_point_in->num_points += ielem_point_out->num_points;
    }
  }
}

static void
t8_pipeline (vtkSmartPointer < vtkDataSet > data, sc_MPI_Comm comm)
{
  MeshAdapter         test = MeshAdapter (data, comm);
  t8_debugf ("[D] Constructed class\n");

  const char          filename[11] = "class_test";
  const char          first_data[13] = "class_test_0";

  const char         *data_name[3] = {
    "average",
    "data1",
    "data2"
  };
  t8_vtk_data_type_t  types[3];

  types[0] = T8_VTK_SCALAR;
  types[1] = T8_VTK_SCALAR;
  types[2] = T8_VTK_SCALAR;

  test.WritePVTU (first_data, data_name, types);

  for (int i = 0; i < 4; i++) {
    test.Adapt (t8_adapt_callback_non_empty, t8_itertate_replace_pointids);

    //test.partition ();
    test.SetElements ();
    char                fileprefix[12];
    snprintf (fileprefix, BUFSIZ, "%s_%i", filename, i + 1);
    test.WritePVTU (fileprefix, data_name, types);
  }
  t8_debugf ("[D] wrote vtk file\n");
  return;
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  mpiret = sc_MPI_Init (&argc, &argv);
  sc_MPI_Comm         comm = sc_MPI_COMM_WORLD;
  SC_CHECK_MPI (mpiret);
  sc_init (comm, 1, 1, NULL, SC_LP_ESSENTIAL);
  t8_init (SC_LP_DEFAULT);

  vtkSmartPointer < vtkDataSet > vtk_grid =
    t8_vtk_reader
    ("/localdata1/knap_da/projects/t8code/t8code/test/testfiles/test_vtk_tri.vtu",
     1, 0, comm, VTK_UNSTRUCTURED_FILE);
  t8_debugf ("[D] read successful\n");
  t8_pipeline (vtk_grid, comm);
  sc_finalize ();
  return 0;
}