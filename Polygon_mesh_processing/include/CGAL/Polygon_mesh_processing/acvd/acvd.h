// Copyright (c) 2023 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Hossam Saeed
//

// #ifndef CGAL_POLYGON_MESH_PROCESSING_<>
// #define CGAL_POLYGON_MESH_PROCESSING_<>

// #include <CGAL/license/Polygon_mesh_processing/<>>

#include <CGAL/assertions.h>
#include <CGAL/IO/Color.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Named_function_parameters.h>
#include <CGAL/property_map.h>
#include <CGAL/boost/graph/named_params_helper.h>
#include <Eigen/Eigenvalues>
#include <CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>

#include <CGAL/Point_set_3/IO.h>
#include <CGAL/Point_set_3.h>

#include <numeric>
#include <vector>
#include <queue>
#include <unordered_set>
#include <iostream>

namespace CGAL {

namespace Polygon_mesh_processing {

namespace internal {

template <typename GT>
struct ClusterData {
  typename GT::Vector_3 site_sum;
  typename GT::FT weight_sum;
  typename GT::FT energy;

  ClusterData() : site_sum(0, 0, 0), weight_sum(0), energy(0) {}

  void add_vertex(const typename GT::Vector_3 vertex_position, const typename GT::FT weight)
  {
    this->site_sum += vertex_position;
    this->weight_sum += 1;
  }

  void remove_vertex(const typename GT::Vector_3 vertex_position, const typename GT::FT weight)
  {
    this->site_sum -= vertex_position;
    this->weight_sum -= 1;
  }

  typename GT::FT compute_energy()
  {
    this->energy = - (this->site_sum).squared_length() / this->weight_sum;
    return this->energy;
  }

  typename GT::Vector_3 compute_centroid()
  {
    if (this->weight_sum > 0)
      return (this->site_sum) / this->weight_sum;
    else
      return typename GT::Vector_3 (-1, -1, -1); // Change this
  }
};

template <typename PolygonMesh, /*ClusteringMetric,*/
  typename NamedParameters = parameters::Default_named_parameters>
void acvd_simplification(
    PolygonMesh& pmesh,
    const int& nb_clusters,
    const NamedParameters& np = parameters::default_values()
    // seed_randomization can be a named parameter
  )
{
  typedef typename GetGeomTraits<PolygonMesh, NamedParameters>::type GT;
  typedef typename GetVertexPointMap<PolygonMesh, NamedParameters>::const_type Vertex_position_map;
  typedef typename boost::graph_traits<PolygonMesh>::halfedge_descriptor Halfedge_descriptor;
  typedef typename boost::graph_traits<PolygonMesh>::vertex_descriptor Vertex_descriptor;
  typedef typename boost::graph_traits<PolygonMesh>::face_descriptor Face_descriptor;
  typedef typename boost::property_map<PolygonMesh, CGAL::dynamic_vertex_property_t<CGAL::IO::Color> >::type VertexColorMap;
  typedef typename boost::property_map<PolygonMesh, CGAL::dynamic_vertex_property_t<int> >::type VertexClusterMap;
  typedef typename boost::property_map<PolygonMesh, CGAL::dynamic_vertex_property_t<typename GT::FT> >::type VertexWeightMap;

  using parameters::choose_parameter;
  using parameters::get_parameter;
  using parameters::is_default_parameter;

  Vertex_position_map vpm = choose_parameter(get_parameter(np, CGAL::vertex_point),
    get_property_map(CGAL::vertex_point, pmesh));

  // initial random clusters
  // property map from vertex_descriptor to cluster index
  VertexClusterMap vertex_cluster_pmap = get(CGAL::dynamic_vertex_property_t<int>(), pmesh);
  VertexWeightMap vertex_weight_pmap = get(CGAL::dynamic_vertex_property_t<typename GT::FT>(), pmesh);
  std::vector<ClusterData<GT>> clusters(nb_clusters + 1);
  std::queue<Halfedge_descriptor> clusters_edges_active;
  std::queue<Halfedge_descriptor> clusters_edges_new;

  int nb_vertices = num_vertices(pmesh);

  typename GT::FT max_area = 0;
  typename GT::FT min_area = std::numeric_limits<typename GT::FT>::max();

  // compute vertex weights (dual area)
  for (Face_descriptor fd : faces(pmesh))
  {
    typename GT::FT weight = CGAL::Polygon_mesh_processing::face_area(fd, pmesh) / 3;

    max_area = std::max(max_area, weight * 8.0);
    min_area = std::min(min_area, weight * 3.0);

    for (Vertex_descriptor vd : vertices_around_face(halfedge(fd, pmesh), pmesh))
    {
      typename GT::FT vertex_weight = get(vertex_weight_pmap, vd);
      vertex_weight += weight;
      put(vertex_weight_pmap, vd, vertex_weight);
    }
  }

  srand(3);
  for (int ci = 0; ci < nb_clusters; ci++)
  {
    //// random index
    //int vi = rand() % num_vertices(pmesh);
    //Vertex_descriptor vd = *(vertices(pmesh).begin() + vi);
    int vi;
    Vertex_descriptor vd;
    do {
      vi = rand() % num_vertices(pmesh);
      vd = *(vertices(pmesh).begin() + vi);
    } while (get(vertex_cluster_pmap, vd));

    // TODO: check for cluster conflict at the same vertex
    put(vertex_cluster_pmap, vd, ci + 1); // TODO: should be ci but for now we start from 1 (can't set null value to -1)
    typename GT::Point_3 vp = get(vpm, vd);
    typename GT::Vector_3 vpv(vp.x(), vp.y(), vp.z());
    clusters[ci].add_vertex(vpv, get(vertex_weight_pmap, vd));

    for (Halfedge_descriptor hd : halfedges_around_source(vd, pmesh))
      clusters_edges_active.push(hd);
  }

  // frequency of each cluster
  std::vector<int> cluster_frequency (nb_clusters + 1, 0);

  for (Vertex_descriptor vd : vertices(pmesh))
  {
    int c = get(vertex_cluster_pmap, vd);
    cluster_frequency[c]++;
  }

  int nb_empty = 0;
  for (int i = 0; i < nb_clusters + 1; i++)
  {
    if (cluster_frequency[i] == 0)
    {
      nb_empty++;
    }
  }

  std::cout << "nb_empty before: " << nb_empty << std::endl;

  int nb_modifications = 0;

  do
  {
    nb_modifications = 0;

    while (clusters_edges_active.empty() == false) {
      Halfedge_descriptor hi = clusters_edges_active.front();
      clusters_edges_active.pop();

      Vertex_descriptor v1 = source(hi, pmesh);
      Vertex_descriptor v2 = target(hi, pmesh);

      int c1 = get(vertex_cluster_pmap, v1);
      int c2 = get(vertex_cluster_pmap, v2);

      if (!(c1 > 0))
      {
        // expand cluster c2 (add v1 to c2)
        put(vertex_cluster_pmap, v1, c2);
        typename GT::Point_3 vp1 = get(vpm, v1);
        typename GT::Vector_3 vpv(vp1.x(), vp1.y(), vp1.z());
        clusters[c2].add_vertex(vpv, get(vertex_weight_pmap, v1));

        // add all halfedges around v1 except hi to the queue
        for (Halfedge_descriptor hd : halfedges_around_source(v1, pmesh))
          //if (hd != hi && hd != opposite(hi, pmesh))
            clusters_edges_new.push(hd);
        nb_modifications++;
      }
      else if (!(c2 > 0))
      {
        // expand cluster c1 (add v2 to c1)
        put(vertex_cluster_pmap, v2, c1);
        typename GT::Point_3 vp2 = get(vpm, v2);
        typename GT::Vector_3 vpv(vp2.x(), vp2.y(), vp2.z());
        clusters[c1].add_vertex(vpv, get(vertex_weight_pmap, v2));

        // add all halfedges around v2 except hi to the queue
        for (Halfedge_descriptor hd : halfedges_around_source(v2, pmesh))
          //if (hd != hi && hd != opposite(hi, pmesh))
            clusters_edges_new.push(hd);
        nb_modifications++;
      }
      else if (c1 == c2)
      {
        clusters_edges_new.push(hi);
      }
      else
      {
        // compare the energy of the 3 cases
        typename GT::Point_3 vp1 = get(vpm, v1);
        typename GT::Vector_3 vpv1(vp1.x(), vp1.y(), vp1.z());
        typename GT::Point_3 vp2 = get(vpm, v2);
        typename GT::Vector_3 vpv2(vp2.x(), vp2.y(), vp2.z());
        typename GT::FT v1_weight = get(vertex_weight_pmap, v1);
        typename GT::FT v2_weight = get(vertex_weight_pmap, v2);

        typename GT::FT e_no_change = clusters[c1].compute_energy() + clusters[c2].compute_energy();

        clusters[c1].remove_vertex(vpv1, v1_weight);
        clusters[c2].add_vertex(vpv1, v1_weight);

        typename GT::FT e_v1_to_c2 = clusters[c1].compute_energy() + clusters[c2].compute_energy();

        typename GT::FT c1_weight_threshold = clusters[c1].weight_sum;

        // reset to no change
        clusters[c1].add_vertex(vpv1, v1_weight);
        clusters[c2].remove_vertex(vpv1, v1_weight);

        // The effect of the following should always be reversed after the comparison
        clusters[c2].remove_vertex(vpv2, v2_weight);
        clusters[c1].add_vertex(vpv2, v2_weight);

        typename GT::FT e_v2_to_c1 = clusters[c1].compute_energy() + clusters[c2].compute_energy();

        typename GT::FT c2_weight_threshold = clusters[c2].weight_sum;


        if (e_v2_to_c1 < e_no_change && e_v2_to_c1 < e_v1_to_c2 /*&& c2_weight_threshold > 0*/)
        {
          // move v2 to c1
          put(vertex_cluster_pmap, v2, c1);

          // cluster data is already updated

          // add all halfedges around v2 except hi to the queue
          for (Halfedge_descriptor hd : halfedges_around_source(v2, pmesh))
            //if (hd != hi && hd != opposite(hi, pmesh))
              clusters_edges_new.push(hd);
          nb_modifications++;
        }
        else if (e_v1_to_c2 < e_no_change)
        {
          // move v1 to c2
          put(vertex_cluster_pmap, v1, c2);

          // need to reset cluster data and then update
          clusters[c2].add_vertex(vpv2, v2_weight);
          clusters[c1].remove_vertex(vpv2, v2_weight);

          clusters[c1].remove_vertex(vpv1, v1_weight);
          clusters[c2].add_vertex(vpv1, v1_weight);

          // add all halfedges around v1 except hi to the queue
          for (Halfedge_descriptor hd : halfedges_around_source(halfedge(v1, pmesh), pmesh))
            //if (hd != hi && hd != opposite(hi, pmesh))
              clusters_edges_new.push(hd);
          nb_modifications++;
        }
        else
        {
            // no change but need to reset cluster data
            clusters[c2].add_vertex(vpv2, v2_weight);
            clusters[c1].remove_vertex(vpv2, v2_weight);

            clusters_edges_new.push(hi);
        }
      }
    }
    clusters_edges_active.swap(clusters_edges_new);
  } while (nb_modifications > 0);

  VertexColorMap vcm = get(CGAL::dynamic_vertex_property_t<CGAL::IO::Color>(), pmesh);

  // frequency of each cluster
  cluster_frequency = std::vector<int>(nb_clusters + 1, 0);

  for (Vertex_descriptor vd : vertices(pmesh))
  {
    int c = get(vertex_cluster_pmap, vd);
    cluster_frequency[c]++;
    //CGAL::IO::Color color((c - min_area) * 255 / (max_area - min_area), 0, 0);
    CGAL::IO::Color color(255 - (c * 255 / nb_clusters), (c * c % 7) * 255 / 7, (c * c * c % 31) * 255 / 31);
    //std::cout << vd.idx() << " " << c << " " << color << std::endl;
    put(vcm, vd, color);
  }

  nb_empty = 0;
  for (int i = 0; i < nb_clusters + 1; i++)
  {
    if (cluster_frequency[i] == 0)
    {
      nb_empty++;
    }
  }

  std::cout << "nb_empty: " << nb_empty << std::endl;

  std::cout << "kak1" << std::endl;
  std::string name = std::to_string(nb_clusters) + ".off";
  CGAL::IO::write_OFF(name, pmesh, CGAL::parameters::vertex_color_map(vcm));
  std::cout << "kak2" << std::endl;

  /// Construct new Mesh 
  std::vector<int> valid_cluster_map(nb_clusters + 1, -1);
  std::vector<typename GT::Point_3> points;
  Point_set_3<typename GT::Point_3> point_set;

  std::vector<std::vector<int> > polygons;
  PolygonMesh simplified_mesh;

  for (int i = 0; i < nb_clusters + 1; i++) //should i =1 ?
  {
    if (clusters[i].weight_sum > 0)
    {
      valid_cluster_map[i] = points.size();
      typename GT::Vector_3 center_v = clusters[i].compute_centroid();
      typename GT::Point_3 center_p(center_v.x(), center_v.y(), center_v.z());
      points.push_back(center_p);
      point_set.insert(center_p);
    }
  }

  name = std::to_string(nb_clusters) + "_points.off";
  CGAL::IO::write_point_set(name, point_set);

  for (Face_descriptor fd : faces(pmesh))
  {
    Halfedge_descriptor hd1 = halfedge(fd, pmesh);
    Vertex_descriptor v1 = source(hd1, pmesh);
    Halfedge_descriptor hd2 = next(hd1, pmesh);
    Vertex_descriptor v2 = source(hd2, pmesh);
    Halfedge_descriptor hd3 = next(hd2, pmesh);
    Vertex_descriptor v3 = source(hd3, pmesh);

    int c1 = get(vertex_cluster_pmap, v1);
    int c2 = get(vertex_cluster_pmap, v2);
    int c3 = get(vertex_cluster_pmap, v3);

    if (c1 != c2 && c1 != c3 && c2 != c3)
    {
      int c1_mapped = valid_cluster_map[c1], c2_mapped = valid_cluster_map[c2], c3_mapped = valid_cluster_map[c3];
      if (c1_mapped != -1 && c2_mapped != -1 && c3_mapped != -1)
      {
        std::vector<int> polygon = {c1_mapped, c2_mapped, c3_mapped};
        polygons.push_back(polygon);
      }
    }
  }

  std::cout << "are polygons a valid mesh ? : " << is_polygon_soup_a_polygon_mesh(polygons) << std::endl;
  polygon_soup_to_polygon_mesh(points, polygons, simplified_mesh);

  name = std::to_string(nb_clusters) + "_simped.off";
  CGAL::IO::write_OFF(name, simplified_mesh);
  std::cout << "kak3" << std::endl;

}


} // namespace internal

template <typename PolygonMesh,
  typename NamedParameters = parameters::Default_named_parameters>
void  acvd_isotropic_simplification(
    PolygonMesh& pmesh,
    const int& nb_vertices,
    const NamedParameters& np = parameters::default_values()
  )
{
  internal::acvd_simplification<PolygonMesh, /*IsotropicMetric,*/ NamedParameters>(
    pmesh,
    nb_vertices,
    np
    );
}

} // namespace Polygon_mesh_processing

} // namespace CGAL
