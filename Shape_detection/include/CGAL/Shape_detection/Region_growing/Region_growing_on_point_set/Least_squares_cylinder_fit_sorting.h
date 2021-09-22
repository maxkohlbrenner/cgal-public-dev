// Copyright (c) 2021 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Simon Giraudot
//

#ifndef CGAL_SHAPE_DETECTION_REGION_GROWING_POINT_SET_LEAST_SQUARES_CYLINDER_FIT_SORTING_H
#define CGAL_SHAPE_DETECTION_REGION_GROWING_POINT_SET_LEAST_SQUARES_CYLINDER_FIT_SORTING_H

#include <CGAL/license/Shape_detection.h>

// Internal includes.
#include <CGAL/Shape_detection/Region_growing/internal/property_map.h>

namespace CGAL {
namespace Shape_detection {
namespace Point_set {

  /*!
    \ingroup PkgShapeDetectionRGOnPoints

    \brief Sorting of 3D points with respect to the local cylinder fit quality.

    Indices of 3D input points are sorted with respect to the quality of the
    least squares cylinder fit applied to the neighboring points of each point.

    \tparam GeomTraits
    a model of `Kernel`

    \tparam InputRange
    a model of `ConstRange` whose iterator type is `RandomAccessIterator`

    \tparam NeighborQuery
    a model of `NeighborQuery`

    \tparam PointMap
    a model of `ReadablePropertyMap` whose key type is the value type of the input
    range and value type is `Kernel::Point_3`

    \tparam NormalMap
    a model of `ReadablePropertyMap` whose key type is the value type of the input
    range and value type is `Kernel::Vector_3`
  */
  template<
  typename GeomTraits,
  typename InputRange,
  typename NeighborQuery,
  typename PointMap,
  typename NormalMap>
  class Least_squares_cylinder_fit_sorting {

  public:
    /// \name Types
    /// @{

    /// \cond SKIP_IN_MANUAL
    using Traits = GeomTraits;
    using Input_range = InputRange;
    using Neighbor_query = NeighborQuery;
    using Point_map = PointMap;
    using Normal_map = NormalMap;
    using Seed_map = internal::Seed_property_map;
    /// \endcond

    #ifdef DOXYGEN_RUNNING
      /*!
        a model of `ReadablePropertyMap` whose key and value type is `std::size_t`.
        This map provides an access to the ordered indices of input points.
      */
      typedef unspecified_type Seed_map;
    #endif

    /// @}

  private:
    // using FT = typename Traits::FT;
    // using Compare_scores = internal::Compare_scores<FT>;

    using Local_traits = Exact_predicates_inexact_constructions_kernel;
    using Local_FT = typename Local_traits::FT;
    using Local_point_3 = typename Local_traits::Point_3;
    using Local_vector_3 = typename Local_traits::Vector_3;
    using Local_line_3 = typename Local_traits::Line_3;
    using Local_pwn = std::pair<Local_point_3, Local_vector_3>;
    using To_local_converter = Cartesian_converter<Traits, Local_traits>;
    using Compare_scores = internal::Compare_scores<Local_FT>;
    using Local_point_map = First_of_pair_property_map<Local_pwn>;
    using Local_normal_map = Second_of_pair_property_map<Local_pwn>;

  public:
    /// \name Initialization
    /// @{

    /*!
      \brief initializes all internal data structures.

      \tparam NamedParameters
      a sequence of \ref bgl_namedparameters "Named Parameters"

      \param input_range
      an instance of `InputRange` with 3D points

      \param neighbor_query
      an instance of `NeighborQuery` that is used internally to
      access point's neighbors

      \param np
      a sequence of \ref bgl_namedparameters "Named Parameters"
      among the ones listed below

      \cgalNamedParamsBegin
        \cgalParamNBegin{point_map}
          \cgalParamDescription{an instance of `PointMap` that maps an item from `input_range`
          to `Kernel::Point_3`}
          \cgalParamDefault{`PointMap()`}
        \cgalParamNEnd
        \cgalParamNBegin{normal_map}
          \cgalParamDescription{ an instance of `NormalMap` that maps an item from `input_range`
          to `Kernel::Vector_3`}
          \cgalParamDefault{`NormalMap()`}
        \cgalParamNEnd
        \cgalParamNBegin{geom_traits}
          \cgalParamDescription{an instance of `GeomTraits`}
          \cgalParamDefault{`GeomTraits()`}
        \cgalParamNEnd
      \cgalNamedParamsEnd

      \pre `input_range.size() > 0`
    */
    template<typename NamedParameters>
    Least_squares_cylinder_fit_sorting(
      const InputRange& input_range,
      NeighborQuery& neighbor_query,
      const NamedParameters& np) :
    m_input_range(input_range),
    m_neighbor_query(neighbor_query),
    m_point_map(parameters::choose_parameter(parameters::get_parameter(
      np, internal_np::point_map), PointMap())),
    m_normal_map(parameters::choose_parameter(parameters::get_parameter(
      np, internal_np::normal_map), NormalMap())),
    m_traits(parameters::choose_parameter(parameters::get_parameter(
      np, internal_np::geom_traits), GeomTraits())),
    m_to_local_converter() {

      CGAL_precondition(input_range.size() > 0);
      m_order.resize(m_input_range.size());
      std::iota(m_order.begin(), m_order.end(), 0);
      m_scores.resize(m_input_range.size());
    }

    /*!
      \brief initializes all internal data structures.

      \deprecated This constructor is deprecated since the version 5.5 of \cgal.

      \param input_range
      an instance of `InputRange` with 3D points

      \param neighbor_query
      an instance of `NeighborQuery` that is used internally to
      access point's neighbors

      \param point_map
      an instance of `PointMap` that maps an item from `input_range`
      to `Kernel::Point_3`

      \param normal_map
      an instance of `PointMap` that maps an item from `input_range`
      to `Kernel::Vector_3`

      \pre `input_range.size() > 0`
    */
    CGAL_DEPRECATED_MSG("This constructor is deprecated since the version 5.5 of CGAL!")
    Least_squares_cylinder_fit_sorting(
      const InputRange& input_range,
      NeighborQuery& neighbor_query,
      const PointMap point_map = PointMap(),
      const NormalMap normal_map = NormalMap()) :
    Least_squares_cylinder_fit_sorting(
      input_range, neighbor_query, CGAL::parameters::
    point_map(point_map).normal_map(normal_map))
    { }

    /// @}

    /// \name Sorting
    /// @{

    /*!
      \brief sorts indices of input points.
    */
    void sort() {

      compute_scores();
      CGAL_postcondition(m_scores.size() > 0);
      Compare_scores cmp(m_scores);
      std::sort(m_order.begin(), m_order.end(), cmp);
    }

    /// @}

    /// \name Access
    /// @{

    /*!
      \brief returns an instance of `Seed_map` to access the ordered indices
      of input points.
    */
    Seed_map seed_map() {
      return Seed_map(m_order);
    }

    /// @}

  private:
    const Input_range& m_input_range;
    Neighbor_query& m_neighbor_query;
    const Point_map m_point_map;
    const Normal_map m_normal_map;
    const Traits m_traits;
    std::vector<std::size_t> m_order;
    std::vector<Local_FT> m_scores;
    const To_local_converter m_to_local_converter;

    void compute_scores() {
      std::vector<std::size_t> neighbors;
      std::vector<std::pair<Local_point_3, Local_vector_3> > points;

      typename internal::Get_sqrt<Local_traits>::Sqrt sqrt;
      typename Local_traits::Compute_squared_distance_3 squared_distance_3;

      for (std::size_t i = 0; i < m_input_range.size(); ++i)
      {

        neighbors.clear();
        m_neighbor_query(i, neighbors);
        neighbors.push_back(i);

        points.clear();
        for (std::size_t j = 0; j < neighbors.size(); ++j)
        {
          CGAL_precondition(neighbors[j] < m_input_range.size());

          const auto& key = *(m_input_range.begin() + neighbors[j]);
          points.emplace_back(m_to_local_converter(get(m_point_map, key)),
                              m_to_local_converter(get(m_normal_map, key)));
        }
        CGAL_postcondition(points.size() == neighbors.size());

        Local_line_3 fitted_line;
        Local_FT fitted_radius;

        if (internal::cylinder_fit (points, Local_point_map(), Local_normal_map(),
                                    sqrt, squared_distance_3, fitted_line, fitted_radius))
        {
          // Score is min squared distance to cylinder
          m_scores[i] = Local_FT(0);
          for (const Local_pwn& pwn : points)
            m_scores[i] += abs (sqrt(squared_distance_3(pwn.first, fitted_line)) - fitted_radius);
        }
        else
          m_scores[i] = Local_FT(std::numeric_limits<double>::infinity());
      }
    }
  };

} // namespace Point_set
} // namespace Shape_detection
} // namespace CGAL

#endif // CGAL_SHAPE_DETECTION_REGION_GROWING_POINT_SET_LEAST_SQUARES_CYLINDER_FIT_SORTING_H
