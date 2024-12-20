/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element classes in parallel.

  Copyright (C) 2024 the developers

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

/** \file t8_crtp.hxx
 * This file implements a helper class for CRTP implementations.
 */

#ifndef T8_CRTP_HXX
#define T8_CRTP_HXX

/** CRTP helper class, adds static "upcasting" methods for const and non const objects. 
 * \tparam TUnderlying The CRTP derived class.
 */
template <class TUnderlying>
class t8_crtp {
 public:
  inline TUnderlying&
  underlying ()
  {
    return static_cast<TUnderlying&> (*this);
  }
  inline TUnderlying const&
  underlying () const
  {
    return static_cast<TUnderlying const&> (*this);
  }
};

#endif /* !T8_CRTP_HXX */
