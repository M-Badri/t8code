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

#include <new>
#include <t8_schemes/t8_subelements/t8_subelements_cxx.hxx>
#include <t8_schemes/t8_default/t8_default_line/t8_default_line_cxx.hxx>
#include <t8_schemes/t8_default/t8_default_tri/t8_default_tri_cxx.hxx>
#include <t8_refcount.h>

/* We want to export the whole implementation to be callable from "C" */
T8_EXTERN_C_BEGIN ();

t8_scheme_cxx_t    *
t8_scheme_new_subelement_cxx (void)
{
  t8_scheme_cxx_t    *s;

  s = T8_ALLOC_ZERO (t8_scheme_cxx_t, 1);
  t8_refcount_init (&s->rc);

  s->eclass_schemes[T8_ECLASS_VERTEX] = new t8_subelement_scheme_c<T8_ECLASS_VERTEX> ();
  s->eclass_schemes[T8_ECLASS_LINE] = new t8_subelement_scheme_c<T8_ECLASS_LINE> ();
  s->eclass_schemes[T8_ECLASS_QUAD] = new t8_subelement_scheme_c<T8_ECLASS_QUAD> ();
  s->eclass_schemes[T8_ECLASS_HEX] = new t8_subelement_scheme_c<T8_ECLASS_HEX> ();
  s->eclass_schemes[T8_ECLASS_TRIANGLE] = new t8_subelement_scheme_c<T8_ECLASS_TRIANGLE> ();
  s->eclass_schemes[T8_ECLASS_TET] = new t8_subelement_scheme_c<T8_ECLASS_TET> ();
  s->eclass_schemes[T8_ECLASS_PRISM] = new t8_subelement_scheme_c<T8_ECLASS_PRISM> ();

  return s;
}

T8_EXTERN_C_END ();