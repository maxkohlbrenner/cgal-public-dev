// Copyright (c) 2009 INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Mael Rouxel-Labbé, Maxime Gimeno
//
//******************************************************************************
//
//******************************************************************************

#ifndef CGAL_MDS_3_TET_SOUP_TO_C3T3_H
#define CGAL_MDS_3_TET_SOUP_TO_C3T3_H

#include <CGAL/license/MDS_3.h>

#include <CGAL/disable_warnings.h>
#include <CGAL/assertions.h>
#include <CGAL/IO/File_medit.h>

#include <array>
#include <vector>
#include <utility>
#include <map>
#include <boost/unordered_map.hpp>


namespace CGAL
{
namespace MDS_3
{
template<typename Vh>
std::array<Vh, 3> make_ordered_vertex_array(const Vh vh0, const Vh vh1, const Vh vh2)
{
  std::array<Vh, 3> ft = { {vh0, vh1, vh2} };
  if (ft[1] < ft[0]) std::swap(ft[0], ft[1]);
  if (ft[2] < ft[1]) std::swap(ft[1], ft[2]);
  if (ft[1] < ft[0]) std::swap(ft[0], ft[1]);
  return ft;
}

template<class Tr, typename PointRange>
void build_vertices(Tr& tr,
                    const PointRange& points,
                    std::vector<typename Tr::Vertex_handle>& vertex_handle_vector)
{
  typedef typename Tr::Vertex_handle            Vertex_handle;

  vertex_handle_vector[0] = tr.tds().create_vertex(); // creates the infinite vertex
  tr.set_infinite_vertex(vertex_handle_vector[0]);

  // build vertices
  for(std::size_t i=0; i<points.size(); ++i)
  {
    Vertex_handle vh = tr.tds().create_vertex();
    vertex_handle_vector[i+1] = vh;
    vh->set_point(points[i]);
  }
}

template<class Tr>
bool add_facet_to_incident_cells_map(const typename Tr::Cell_handle c, int i,
    boost::unordered_map<std::array<typename Tr::Vertex_handle, 3>,
                         std::vector<std::pair<typename Tr::Cell_handle, int> > >& incident_cells_map,
    const bool verbose)
{
  typedef typename Tr::Vertex_handle                                Vertex_handle;
  typedef typename Tr::Cell_handle                                  Cell_handle;
  typedef std::array<Vertex_handle, 3>                              Facet_vvv;
  typedef std::pair<Cell_handle, int>                               Incident_cell;
  typedef boost::unordered_map<Facet_vvv, std::vector<Incident_cell> > Incident_cells_map;

  // the opposite vertex of f in c is i
  Facet_vvv f = make_ordered_vertex_array(c->vertex((i + 1) % 4),
                                          c->vertex((i + 2) % 4),
                                          c->vertex((i + 3) % 4));
  CGAL_precondition(f[0] != f[1] && f[1] != f[2]);

  Incident_cell e = std::make_pair(c, i);
  std::vector<Incident_cell> vec;
  vec.push_back(e);
  std::pair<typename Incident_cells_map::iterator, bool> is_insert_successful =
      incident_cells_map.insert(std::make_pair(f, vec));
  if(!is_insert_successful.second) // the entry already exists in the map
  {
    // a facet must have exactly two incident cells
    if (is_insert_successful.first->second.size() != 1)
    {
      if(verbose)
        std::cout << "Error in add_facet_to_incident_cells_map" << std::endl;
      return false;
    }
    is_insert_successful.first->second.push_back(e);
  }
  return true;
}

template<class Tr, typename CellRange, typename FacetPatchMap>
bool build_finite_cells(Tr& tr,
    const CellRange& finite_cells,
    const std::vector<typename Tr::Vertex_handle>& vertex_handle_vector,
    boost::unordered_map<std::array<typename Tr::Vertex_handle, 3>,
                        std::vector<std::pair<typename Tr::Cell_handle, int> > >& incident_cells_map,
    const FacetPatchMap& border_facets,
    const bool verbose,
    bool replace_domain_0 = false)
{
  typedef std::array<int, 5>              Tet_with_ref; // 4 ids + 1 reference

  typedef typename Tr::Vertex_handle                            Vertex_handle;
  typedef typename Tr::Cell_handle                              Cell_handle;
  typedef typename Tr::Cell::Surface_patch_index                Surface_patch_index;

  CGAL_assertion_code(
    typename Tr::Geom_traits::Construct_point_3 cp =
      tr.geom_traits().construct_point_3_object();
    typename Tr::Geom_traits::Orientation_3 orientation =
      tr.geom_traits().orientation_3_object();
  )
  int max_domain = 0;
  if(replace_domain_0)
  {
    for(std::size_t i=0; i<finite_cells.size(); ++i)
    {
      const Tet_with_ref& tet = finite_cells[i];
      if(tet[4] > max_domain)
        max_domain=tet[4];
    }
  }
  // build the finite cells
  for(std::size_t i=0; i<finite_cells.size(); ++i)
  {
    const Tet_with_ref& tet = finite_cells[i];
    std::array<Vertex_handle, 4> vs;

    for(int j=0; j<4; ++j)
    {
      CGAL_precondition(static_cast<std::size_t>(tet[j]) < tr.number_of_vertices() && tet[j] >= 0);
      vs[j] = vertex_handle_vector.at(tet[j] + 1);
      CGAL_postcondition(vs[j] != Vertex_handle());
      CGAL_postcondition(!tr.is_infinite(vs[j]));
      vs[j]->set_dimension(3);
    }

    // this assertion also tests for degeneracy
    CGAL_assertion(orientation(cp(tr.point(vs[0])), cp(tr.point(vs[1])),
                               cp(tr.point(vs[2])), cp(tr.point(vs[3])))
                     == POSITIVE);

    Cell_handle c = tr.tds().create_cell(vs[0], vs[1], vs[2], vs[3]);
    c->set_subdomain_index(tet[4]); // the cell's info keeps the reference of the tetrahedron
    if(replace_domain_0 && tet[4] == 0)
    {
      c->set_subdomain_index(max_domain+1); // the cell's info keeps the reference of the tetrahedron
    }
    // assign cells to vertices
    for(int j=0; j<4; ++j)
    {
      if(vs[j]->cell() == Cell_handle())
        vs[j]->set_cell(c);
    }

    // build the map used for adjacency later
    for(int j=0; j<4; ++j)
    {
      if(!add_facet_to_incident_cells_map<Tr>(c, j, incident_cells_map, verbose))
        return false;
      if(border_facets.size() != 0)
      {
        std::array<int,3> facet;
        facet[0]=tet[(j+1) % 4];
        facet[1]=tet[(j+2) % 4];
        facet[2]=tet[(j+3) % 4];
        //find the circular permutation that puts the smallest index in the first place.
        int n0 = (std::min)((std::min)(facet[0], facet[1]), facet[2]);
        int k=0;
        std::array<int,3> f;
        do
        {
          f[0]=facet[(0+k)%3];
          f[1]=facet[(1+k)%3];
          f[2]=facet[(2+k)%3];
          ++k;
        } while(f[0] != n0);

        typename boost::unordered_map<std::array<int,3>, Surface_patch_index>::const_iterator
          it = border_facets.find(f);
        if(it != border_facets.end())
        {
          c->set_surface_patch_index(j, it->second);
        }
        else
        {
          int temp = f[2];
          f[2] = f[1];
          f[1] = temp;

          it = border_facets.find(f);
          if(it != border_facets.end())
            c->set_surface_patch_index(j, it->second);
          else
            c->set_surface_patch_index(j, Surface_patch_index());
        }
      }
    }
  }
  return true;
}

template<class Tr>
bool add_infinite_facets_to_incident_cells_map(typename Tr::Cell_handle c,
     int inf_vert_pos,
     boost::unordered_map<std::array<typename Tr::Vertex_handle, 3>,
                          std::vector<std::pair<typename Tr::Cell_handle, int> > >& incident_cells_map,
     const bool verbose)
{
  int l = (inf_vert_pos + 1) % 4;
  bool b1 = add_facet_to_incident_cells_map<Tr>(c, l, incident_cells_map, verbose);
  l = (inf_vert_pos + 2) % 4;
  bool b2 = add_facet_to_incident_cells_map<Tr>(c, l, incident_cells_map, verbose);
  l = (inf_vert_pos + 3) % 4;
  bool b3 = add_facet_to_incident_cells_map<Tr>(c, l, incident_cells_map, verbose);
  return b1 && b2 && b3;
}

template<class Tr>
bool build_infinite_cells(Tr& tr,
  boost::unordered_map<std::array<typename Tr::Vertex_handle, 3>,
                       std::vector<std::pair<typename Tr::Cell_handle, int> > >& incident_cells_map,
  const bool verbose)
{
  typedef typename Tr::Vertex_handle                               Vertex_handle;
  typedef typename Tr::Cell_handle                                 Cell_handle;
  typedef std::array<Vertex_handle, 3>                             Facet_vvv;
  typedef std::pair<Cell_handle, int>                              Incident_cell;
  typedef boost::unordered_map<Facet_vvv, std::vector<Incident_cell> > Incident_cells_map;

  std::vector<Cell_handle> infinite_cells;

  // check the incident cells map for facets who only have one incident cell
  // and build the infinite cell on the opposite side
  typename Incident_cells_map::iterator it = incident_cells_map.begin();
  typename Incident_cells_map::iterator end = incident_cells_map.end();
  for(; it != end; ++it)
  {
    if(it->second.size() == 2) // facet already has both its incident cells
      continue;
    CGAL_assertion(it->second.size() == 1);

    Cell_handle c = it->second[0].first;
    int i = it->second[0].second;

    Cell_handle opp_c;
    // the infinite cell that we are creating needs to be well oriented...
    if(i == 0 || i == 2)
      opp_c = tr.tds().create_cell(tr.infinite_vertex(),
                                   c->vertex((i + 2) % 4),
                                   c->vertex((i + 1) % 4),
                                   c->vertex((i + 3) % 4));
    else
      opp_c = tr.tds().create_cell(tr.infinite_vertex(),
                                   c->vertex((i + 3) % 4),
                                   c->vertex((i + 1) % 4),
                                   c->vertex((i + 2) % 4));

    infinite_cells.push_back(opp_c);

    // set the infinite_vertex's incident cell
    if(tr.infinite_vertex()->cell() == Cell_handle())
      tr.infinite_vertex()->set_cell(opp_c);

    // the only finite facet
    it->second.push_back(std::make_pair(opp_c, 0));
    CGAL_assertion(it->second.size() == 2);

    opp_c->set_surface_patch_index(0, c->surface_patch_index(i));
  }

#ifdef CGAL_TET_SOUP_TO_C3T3_DEBUG
  for (auto icit : incident_cells_map)
    CGAL_assertion(icit.second.size() == 2);

  std::map<Facet_vvv, int> facets;
  for (const Cell_handle c : infinite_cells)
  {
    for (int i = 1; i < 4; ++i)
    {
      std::array<Vertex_handle, 3> vs = make_ordered_vertex_array(c->vertex((i + 1) % 4),
        c->vertex((i + 2) % 4),
        c->vertex((i + 3) % 4));
      if (facets.find(vs) == facets.end())
        facets.insert(std::make_pair(vs, 1));
      else
        facets[vs]++;
    }
  }
  for (auto fp : facets)
    CGAL_assertion(fp.second == 2);
#endif

  // add the facets to the incident cells map
  for (const Cell_handle& c : infinite_cells)
    if(!add_infinite_facets_to_incident_cells_map<Tr>(c, 0, incident_cells_map, verbose))
      return false;

  return true;
}

template<class Tr>
bool assign_neighbors(Tr& tr,
  const boost::unordered_map<std::array<typename Tr::Vertex_handle, 3>,
                             std::vector<std::pair<typename Tr::Cell_handle, int> > >& incident_cells_map)
{
  typedef typename Tr::Cell_handle                                   Cell_handle;
  typedef std::pair<Cell_handle, int>                                Incident_cell;
  typedef boost::unordered_map<std::array<typename Tr::Vertex_handle, 3>,
                               std::vector<Incident_cell> >          Incident_cells_map;

  typename Incident_cells_map::const_iterator icit = incident_cells_map.begin();
  for(; icit!=incident_cells_map.end(); ++icit)
  {
    const std::vector<Incident_cell>& adjacent_cells = icit->second;
    if(adjacent_cells.size() != 2)
      return false;

    Cell_handle c0 = adjacent_cells[0].first;
    int i0 = adjacent_cells[0].second;
    Cell_handle c1 = adjacent_cells[1].first;
    int i1 = adjacent_cells[1].second;

    tr.tds().set_adjacency(c0, i0, c1, i1);
  }
  return true;
}

template<class Tr,
         typename PointRange,
         typename CellRange,
         typename FacetPatchMap>
bool build_triangulation(Tr& tr,
                         const PointRange& points,
                         const CellRange& finite_cells,
                         const FacetPatchMap& border_facets,
                         std::vector<typename Tr::Vertex_handle>& vertex_handle_vector,
                         const bool verbose = false,
                         bool replace_domain_0 = false)
{
  typedef typename Tr::Vertex_handle            Vertex_handle;
  typedef typename Tr::Cell_handle              Cell_handle;
  typedef std::array<Vertex_handle, 3>          Facet_vvv;

  // associate to a face the two (at most) incident tets and the id of the face in the cell
  typedef std::pair<Cell_handle, int>                   Incident_cell;
  typedef boost::unordered_map<Facet_vvv, std::vector<Incident_cell> >  Incident_cells_map;

  Incident_cells_map incident_cells_map;
  vertex_handle_vector.resize(points.size() + 1); // id to vertex_handle
                                        //index 0 is for infinite vertex
                                        // 1 to n for points in `points`

  CGAL_precondition(!points.empty());

  if(finite_cells.empty())
  {
    std::cout << "WARNING: No finite cells were provided. Only the points will be loaded."<<std::endl;
  }

  tr.tds().clear(); // not tr.clear() since it calls tr.init() which we don't want

  build_vertices<Tr>(tr, points, vertex_handle_vector);
  for(Vertex_handle vh : vertex_handle_vector)
  {
    vh->set_dimension(-1);
  }
  if (!finite_cells.empty())
  {
    if(!build_finite_cells<Tr>(tr, finite_cells, vertex_handle_vector, incident_cells_map,
                           border_facets, verbose, replace_domain_0))
      return false;
    if(!build_infinite_cells<Tr>(tr, incident_cells_map, verbose))
      return false;
    tr.tds().set_dimension(3);
    if (!assign_neighbors<Tr>(tr, incident_cells_map))
      return false;
    if (verbose)
    {
      std::cout << "built triangulation : " << std::endl;
      std::cout << tr.number_of_cells() << " cells" << std::endl;
    }
  }
  if(verbose)
    std::cout << tr.number_of_vertices() << " vertices" << std::endl;

  return true;// tr.tds().is_valid();
              //TDS not valid when cells do not cover the convex hull of vertices
}

template<class Tr,
         typename PointRange,
         typename CellRange,
         typename FacetPatchMap>
bool build_triangulation(Tr& tr,
                         const PointRange& points,
                         const CellRange& finite_cells,
                         const FacetPatchMap& border_facets,
                         const bool verbose = false,
                         bool replace_domain_0 = false)
{
  BOOST_STATIC_ASSERT(boost::is_same<typename Tr::Point,
                                     typename PointRange::value_type>::value);
  std::vector<typename Tr::Vertex_handle> vertex_handle_vector;

  return build_triangulation(tr, points, finite_cells, border_facets,
                             vertex_handle_vector,
                             verbose, replace_domain_0);
}

template<class Tr>
bool build_triangulation_from_file(std::istream& is,
                                   Tr& tr,
                                   const bool verbose,
                                   bool replace_domain_0)
{
  typedef typename Tr::Point                                  Point_3;

  typedef std::array<int, 3> Facet; // 3 = id
  typedef std::array<int, 5> Tet_with_ref; // first 4 = id, fifth = reference

  std::vector<Tet_with_ref> finite_cells;
  std::vector<Point_3> points;
  boost::unordered_map<Facet, typename Tr::Cell::Surface_patch_index> border_facets;

  // grab the vertices
  int dim;
  int nv, nf, ntet, ref;
  std::string word;

  is >> word >> dim; // MeshVersionFormatted 1
  is >> word >> dim; // Dimension 3

  CGAL_assertion(dim == 3);

  if(verbose)
    std::cout << "Reading .mesh file..." << std::endl;

  while(is >> word && word != "End")
  {
    if(word == "Vertices")
    {
      is >> nv;
      for(int i=0; i<nv; ++i)
      {
        double x,y,z;
        is >> x >> y >> z >> ref;
        points.push_back(Point_3(x,y,z));
      }
    }

    if(word == "Triangles")
    {
      is >> nf;
      for(int i=0; i<nf; ++i)
      {
        int n1, n2, n3;
        typename Tr::Cell::Surface_patch_index surface_patch_id;
        is >> n1 >> n2 >> n3 >> surface_patch_id;
        Facet facet;
        facet[0] = n1 - 1;
        facet[1] = n2 - 1;
        facet[2] = n3 - 1;
        //find the circular permutation that puts the smallest index in the first place.
        int n0 = (std::min)((std::min)(facet[0],facet[1]), facet[2]);
        int k=0;
        Facet f;
        do
        {
          f[0] = facet[(0+k)%3];
          f[1] = facet[(1+k)%3];
          f[2] = facet[(2+k)%3];
          ++k;
        } while(f[0] != n0);
        border_facets.insert(std::make_pair(f, surface_patch_id));
      }
    }

    if(word == "Tetrahedra")
    {
      is >> ntet;
      for(int i=0; i<ntet; ++i)
      {
        int n0, n1, n2, n3, reference;
        is >> n0 >> n1 >> n2 >> n3 >> reference;
        Tet_with_ref t;
        t[0] = n0 - 1;
        t[1] = n1 - 1;
        t[2] = n2 - 1;
        t[3] = n3 - 1;
        t[4] = reference;
        finite_cells.push_back(t);
      }
    }
  }

  if (verbose)
  {
    std::cout << points.size() << " points" << std::endl;
    std::cout << border_facets.size() << " border facets" << std::endl;
    std::cout << finite_cells.size() << " cells" << std::endl;
  }

  if(finite_cells.empty())
    return false;

  return build_triangulation(tr,
                      points, finite_cells, border_facets,
                      verbose, replace_domain_0);
}

}  // namespace MDS_3
}  // namespace CGAL

#include <CGAL/enable_warnings.h>

#endif // CGAL_MDS_3_TET_SOUP_TO_C3T3_H
