// Copyright (c) 2023 GeometryFactory (France).
//
// This file is part of CGAL (www.cgal.org)
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Jeffrey Cochran

#ifndef COLLISION_MESH_3_COLLISION_MESH_3_DO_COLLIDE_H
#define COLLISION_MESH_3_COLLISION_MESH_3_DO_COLLIDE_H

#include <CGAL/Collision_mesh_3.h>
#include <CGAL/Collision_scene_3.h>
#include <CGAL/Collision_scene_3_has_collision.h>

namespace CGAL{

/// \ingroup do_collide_grp
/// @{

/*!
    \brief Returns true if a collision occurs between any of the collision meshes provided
    \details This function efficiently computes the occurence of a collision by only considering those triangle trajectories whose bounding isocuboids intersect.
*/
template <class K>
bool do_collide(
    std::vector< Collision_mesh<K> >& meshes
){
  Collision_scene<K> scene = Collision_scene<K>(meshes);
  return has_collision(scene); //
}

/*!
    \brief Returns true if a collision occurs between any of the collision meshes provided
    \details This function efficiently computes the occurence of a collision by only considering those triangle trajectories whose bounding isocuboids intersect.
*/
template <class K>
bool do_collide(
    Collision_mesh<K>& mesh_1,
    Collision_mesh<K>& mesh_2
){
  std::vector< Collision_mesh<K> > meshes{mesh_1, mesh_2};
  Collision_scene<K> scene = Collision_scene<K>(meshes);
  return has_collision(scene); //
}

/// @}

} // end CGAL

#endif