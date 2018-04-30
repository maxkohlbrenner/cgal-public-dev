// Copyright (c) 2018 GeometryFactory Sarl (France).
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
// SPDX-License-Identifier: GPL-3.0+
//
// Author(s)     : Simon Giraudot

#ifndef CGAL_CLASSIFICATION_FEATURE_CLUSTER_MEAN_FEATURE_H
#define CGAL_CLASSIFICATION_FEATURE_CLUSTER_MEAN_FEATURE_H

#include <CGAL/license/Classification.h>

#include <vector>
#include <CGAL/Classification/Feature_base.h>

namespace CGAL {

namespace Classification {

namespace Feature {

class Cluster_mean_of_feature : public CGAL::Classification::Feature_base
{
  std::vector<float> m_values;
    
public:

  template <typename ClusterRange>
  Cluster_mean_of_feature (ClusterRange& clusters,
                           Feature_handle itemwise_feature)
  {
    std::ostringstream oss;
    oss << "mean_" << itemwise_feature->name();
    this->set_name (oss.str());

    m_values.reserve (clusters.size());
    for (std::size_t i = 0; i < clusters.size(); ++ i)
    {
      double mean = 0.;

      for (std::size_t j = 0; j < clusters[i].size(); ++ j)
        mean += double(itemwise_feature->value (clusters[i].index(j)));
      mean /= clusters[i].size();
      m_values.push_back (float(mean));
    }
  }

  virtual float value (std::size_t cluster_index)
  {
    return m_values[cluster_index];
  }
    
};

} // namespace Feature

} // namespace Classification

} // namespace CGAL

#endif // CGAL_CLASSIFICATION_FEATURE_CLUSTER_MEAN_FEATURE_H
