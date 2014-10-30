// Copyright (c) 2012 Geometry Factory. All rights reserved.
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
// Author(s)     : Andreas Fabri, Fernando Cacciola
//
#ifndef CGAL_POLYLINE_SIMPLIFICATION_2_SCALED_SQUARED_DISTANCE_COST_H
#define CGAL_POLYLINE_SIMPLIFICATION_2_SCALED_SQUARED_DISTANCE_COST_H


namespace CGAL {

namespace Polyline_simplification_2
{
/// \ingroup PkgPolylineSimplification2Classes


/// This class is a cost function which calculates the cost as a scaled variant of the square of the distance between the original and simplified polylines.
///
/// \cgalModels  `PolylineSimplificationCostFunction`
class Scaled_squared_distance_cost
{

public:

  /// Initializes the cost function.
  Scaled_squared_distance_cost() {}

  /// Returns the maximum of the square distances between each point along the original subpolyline
  /// between `p` and `r`,
  /// and the straight line segment `p->r` divided by the shortest squared distance between
  /// that segment and each of the vertices adjacent to `q`.
    template<class Tr>
    boost::optional<typename CGAL::Constrained_triangulation_plus_2<Tr>::Geom_traits::FT>
    operator()(const CGAL::Constrained_triangulation_plus_2<Tr>& pct
                                  , typename CGAL::Constrained_triangulation_plus_2<Tr>::Vertices_in_constraint_iterator p
                                  , typename CGAL::Constrained_triangulation_plus_2<Tr>::Vertices_in_constraint_iterator q
                                  , typename CGAL::Constrained_triangulation_plus_2<Tr>::Vertices_in_constraint_iterator r) const
  {
    typedef typename CGAL::Constrained_triangulation_plus_2<Tr>::Points_in_constraint_iterator Points_in_constraint_iterator;
    typedef typename CGAL::Constrained_triangulation_plus_2<Tr>::Vertex_handle Vertex_handle;
    typedef typename CGAL::Constrained_triangulation_plus_2<Tr>::Vertex_circulator Vertex_circulator;
    typedef typename CGAL::Constrained_triangulation_plus_2<Tr>::Geom_traits Geom_traits ;
    typedef typename Geom_traits::FT                                  FT;
    typedef typename Geom_traits::Compute_squared_distance_2 Compute_squared_distance;
    typedef typename Geom_traits::Construct_segment_2        Construct_segment;
    typedef typename Geom_traits::Segment_2                  Segment;
    typedef typename Geom_traits::Point_2                    Point;                   

    Compute_squared_distance compute_squared_distance = pct.geom_traits().compute_squared_distance_2_object() ;
    Construct_segment        construct_segment        = pct.geom_traits().construct_segment_2_object() ;
    
    Point const& lP = (*p)->point();
    Point const& lR = (*r)->point();
     
    Segment lP_R = construct_segment(lP, lR) ;

    FT d1 = 0.0;
    Points_in_constraint_iterator pp(p), rr(r);
    ++pp;
    
    for ( ;pp != rr; ++pp )
      d1 = (std::max)(d1, compute_squared_distance( lP_R, *pp ) ) ;

    double d2 = (std::numeric_limits<double>::max)() ;

    Vertex_circulator vc = (*q)->incident_vertices(), done(vc);
    do {
      if((vc != pct.infinite_vertex()) && (vc != *p) && (vc != *r)){
        d2 = (std::min)(d2, compute_squared_distance(vc->point(), (*q)->point()));
      }
      ++vc;
    }while(vc != done);

    return d1 / d2 ;
  }

};



} // namespace Polyline_simplification_2


} //namespace CGAL

#endif // CGAL_POLYLINE_SIMPLIFICATION_2_SCALED_SQUARED_DISTANCE_COST_H



