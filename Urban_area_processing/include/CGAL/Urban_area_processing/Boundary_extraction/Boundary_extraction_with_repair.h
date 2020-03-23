// Copyright (c) 2020 SARL GeometryFactory (France).
// All rights reserved.
//
// This file is a part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0+
//
// Author(s)     : Dmitry Anisimov, Simon Giraudot, Pierre Alliez, Florent Lafarge, and Andreas Fabri

#ifndef CGAL_URBAN_AREA_PROCESSING_BOUNDARY_EXTRACTION_WITH_REPAIR_H
#define CGAL_URBAN_AREA_PROCESSING_BOUNDARY_EXTRACTION_WITH_REPAIR_H

// #include <CGAL/license/Urban_area_processing.h>

// Boost includes.
#include <boost/function_output_iterator.hpp>

// CGAL includes.
#include <CGAL/assertions.h>

// Internal includes.
#include <CGAL/Urban_area_processing/internal/utils.h>
#include <CGAL/Urban_area_processing/internal/Estimate_normals_3.h>
#include <CGAL/Urban_area_processing/internal/Generic_point_extractor.h>
#include <CGAL/Urban_area_processing/internal/Extract_vertical_points_3.h>
#include <CGAL/Urban_area_processing/internal/Boundary_from_triangulation_2.h>
#include <CGAL/Urban_area_processing/internal/Point_3_to_point_2_inserter.h>
#include <CGAL/Urban_area_processing/internal/Sphere_neighbor_query.h>

// Utils.
#include "../../../../test/Urban_area_processing/include/Saver.h"

namespace CGAL {
namespace Urban_area_processing {

  template<
  typename GeomTraits,
  typename InputRange,
  typename PointMap>
  class Boundary_extraction_with_repair {

  public:
    using Traits = GeomTraits;
    using Input_range = InputRange;
    using Point_map = PointMap;

    using FT = typename Traits::FT;
    using Point_2 = typename Traits::Point_2;
    using Vector_3 = typename Traits::Vector_3;

    using Indices = std::vector<std::size_t>;

    using Sphere_neighbor_query_3 = internal::Sphere_neighbor_query<
      Traits, Input_range, Point_map>;
    using Estimate_normals_3 = internal::Estimate_normals_3<
      Traits, Input_range, Sphere_neighbor_query_3, Point_map>;

    using Point_inserter = 
      internal::Point_3_to_point_2_inserter<Traits>;
    using Vertical_condition = 
      internal::Extract_vertical_points_3<Traits>;
    using Point_extractor = internal::Generic_point_extractor<
    Traits, Input_range, Vertical_condition, Point_map>;

    using Identity_map_2 = CGAL::Identity_property_map<Point_2>;
    using Sphere_neighbor_query_2 =
    internal::Sphere_neighbor_query<Traits, std::vector<Point_2>, Identity_map_2>;

    /*
    using Normal_estimator_2 = 
    internal::Estimate_normals_2<Traits, Points_2, Identity_map, Neighbor_query>;
    using LSLF_region = 
    internal::Least_squares_line_fit_region<Traits, Pair_range_2, First_of_pair_map, Second_of_pair_map>;
    using LSLF_sorting =
    internal::Least_squares_line_fit_sorting<Traits, Points_2, Neighbor_query, Identity_map>;
    using Region_growing_2 = 
    internal::Region_growing<Points_2, Neighbor_query, LSLF_region, typename LSLF_sorting::Seed_map>; */

    Boundary_extraction_with_repair(
      const InputRange& input_range,
      const PointMap point_map,
      const FT scale,
      const FT noise,
      const FT min_length_2,
      const FT max_angle_2,
      const FT max_angle_3) : 
    m_input_range(input_range),
    m_point_map(point_map),
    m_scale(scale),
    m_noise(noise),
    m_min_length_2(min_length_2),
    m_max_angle_2(max_angle_2),
    m_max_angle_3(max_angle_3) { 
      
      CGAL_precondition(input_range.size() > 0);
    }

    template<typename OutputIterator>
    void extract(OutputIterator boundaries) {

      std::cout << "* extracting boundary with repair... " << std::endl;
      std::vector<Point_2> boundary_points_2;
      extract_boundary_points_2(boundary_points_2);
      std::cout << "- boundary points are extracted: " << 
        boundary_points_2.size() << std::endl;

      save_boundary_points_2(boundary_points_2);
      std::cout << "- boundary points are saved" << std::endl;

      std::vector<Indices> wall_regions_2;
      extract_wall_regions_2(boundary_points_2, wall_regions_2);
      std::cout << "- wall regions are extracted: " << 
        wall_regions_2.size() << std::endl;

      std::cout << std::endl;
    }

  private:
    const Input_range& m_input_range;
    const Point_map m_point_map;
    const FT m_scale;
    const FT m_noise;
    const FT m_min_length_2;
    const FT m_max_angle_2;
    const FT m_max_angle_3;

    void extract_boundary_points_2(
      std::vector<Point_2>& boundary_points_2) const {

      std::vector<Vector_3> normals;
      estimate_normals(normals);

      boundary_points_2.clear();
      const Vertical_condition vertical_condition(
        normals, m_max_angle_3);
      const Point_extractor extractor(
        m_input_range, vertical_condition, m_point_map);

      Point_inserter inserter(boundary_points_2);
      extractor.extract(
        boost::make_function_output_iterator(inserter));
    }

    void estimate_normals(
      std::vector<Vector_3>& normals) const {

      Sphere_neighbor_query_3 neighbor_query(
        m_input_range, m_scale, m_point_map);
      Estimate_normals_3 estimator(
        m_input_range, neighbor_query, m_point_map);
      estimator.get_normals(normals);
      CGAL_assertion(normals.size() == m_input_range.size());
    }

    void save_boundary_points_2(
      const std::vector<Point_2>& boundary_points_2) const {

      Saver<Traits> saver;
      saver.export_points(boundary_points_2,  
      "/Users/monet/Documents/gf/urban-area-processing/logs/boundary_points_2");
    }

    void extract_wall_regions_2(
      const std::vector<Point_2>& boundary_points_2,
      std::vector<Indices>& wall_regions_2) const {

      wall_regions_2.clear();
      Identity_map_2 identity_map_2;
      Sphere_neighbor_query_2 neighbor_query(
        boundary_points_2, m_scale, identity_map_2);

      /*
      Vectors_2 normals;
      Normal_estimator_2 estimator(
        m_boundary_points_2, neighbor_query, identity_map);
      estimator.get_normals(normals);

      CGAL_assertion(m_boundary_points_2.size() == normals.size());
      Pair_range_2 range;
      range.reserve(m_boundary_points_2.size());
      for (std::size_t i = 0; i < m_boundary_points_2.size(); ++i)
        range.push_back(std::make_pair(m_boundary_points_2[i], normals[i]));

      First_of_pair_map point_map;
      Second_of_pair_map normal_map;
      LSLF_region region(
        range, 
        region_growing_noise_level_2,
        region_growing_angle_2,
        region_growing_min_length_2,
        point_map,
        normal_map);

      LSLF_sorting sorting(
        m_boundary_points_2, neighbor_query, identity_map);
      sorting.sort();

      Region_growing_2 region_growing(
        m_boundary_points_2,
        neighbor_query,
        region,
        sorting.seed_map());
      region_growing.detect(std::back_inserter(regions)); */
    }
  };

} // Urban_area_processing
} // CGAL

#endif // CGAL_URBAN_AREA_PROCESSING_BOUNDARY_EXTRACTION_WITH_REPAIR_H
