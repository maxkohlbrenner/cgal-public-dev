#ifndef CGAL_SHAPE_REGULARIZATION_ANGLE_REGULARIZATION_2
#define CGAL_SHAPE_REGULARIZATION_ANGLE_REGULARIZATION_2

// #include <CGAL/license/Shape_regularization.h>

#include <map>
#include <utility>
#include <vector>

#include <CGAL/Shape_regularization/internal/utils.h>
#include <CGAL/Shape_regularization/internal/Segment_data_2.h>
#include <CGAL/Shape_regularization/internal/Grouping_segments_2.h>


namespace CGAL {
namespace Regularization {

  template<
    typename GeomTraits, 
    typename InputRange,
    typename SegmentMap>
  class Angle_regularization_2 {
  public:
    using Traits = GeomTraits;
    using Input_range = InputRange;
    using Segment_map = SegmentMap;
    using FT = typename GeomTraits::FT;
    using Segment = typename GeomTraits::Segment_2;
    using Point = typename GeomTraits::Point_2;
    using Segment_data = typename internal::Segment_data_2<Traits>;
    using Grouping = internal::Grouping_segments_2<Traits>;
    using Vector  = typename GeomTraits::Vector_2;

    Angle_regularization_2 (
      InputRange& input_range, 
      const SegmentMap segment_map = SegmentMap()) :
    m_input_range(input_range),
    m_segment_map(segment_map),
    m_mu_ij(FT(4) / FT(5)) {

      CGAL_precondition(input_range.size() > 0);
      for (std::size_t i = 0; i < m_input_range.size(); ++i) {
        const Segment& seg = get(m_segment_map, *(m_input_range.begin() + i));
        const Segment_data seg_data(seg, i);
        m_segments.push_back(seg_data);
      }
      m_grouping_ptr = std::make_shared<Grouping>(m_segments);

    }

    FT target_value(const std::size_t i, const std::size_t j) {
 
      //compute_orientation
      const FT mes_ij = m_segments[i].m_orientation - m_segments[j].m_orientation;
      const double mes90 = std::floor(CGAL::to_double(mes_ij / FT(90)));

      const FT to_lower = FT(90) *  static_cast<FT>(mes90)          - mes_ij;
      const FT to_upper = FT(90) * (static_cast<FT>(mes90) + FT(1)) - mes_ij;

      const FT  t_ij = CGAL::abs(to_lower) < CGAL::abs(to_upper) ? to_lower : to_upper;

      if (CGAL::abs(t_ij) < bound(i) + bound(j)) {
        m_t_ijs[std::make_pair(i, j)] = t_ij;

        // we will need r_ij in update();  
        int      r_ij;
        if (CGAL::abs(to_lower) < CGAL::abs(to_upper))
            r_ij = ((90 * static_cast<int>(mes90)) % 180 == 0 ? 0 : 1);
        else
            r_ij = ((90 * static_cast<int>(mes90 + 1.0)) % 180 == 0 ? 0 : 1);
        
        m_r_ijs[std::make_pair(i, j)] = r_ij;
      }
  
      return t_ij;
    }

    FT bound(const std::size_t i) {
      FT theta_max;
      m_input_range.size() > 3 ? theta_max = FT(25) : theta_max = FT(10);
      return theta_max;
    }

    std::map<FT, std::vector<std::size_t>> parallel_groups_angle_map() {

      CGAL_precondition(m_parallel_groups_angle_map.size() > 0);
      return m_parallel_groups_angle_map;

    }


    void update(std::vector<FT> & result) {

      m_parallel_groups_angle_map.clear();
      m_grouping_ptr->make_groups(m_t_ijs, m_r_ijs, m_mu_ij, result, m_parallel_groups_angle_map);

      for (auto it_ps = m_parallel_groups_angle_map.begin(); it_ps != m_parallel_groups_angle_map.end(); ++it_ps) {
        const FT theta = it_ps->first;
        const std::vector<std::size_t> &group = it_ps->second;

        // Each group of parallel segments has a normal vector that we compute with alpha.
        const FT x = static_cast<FT>(cos(CGAL::to_double(theta * static_cast<FT>(CGAL_PI) / FT(180))));
        const FT y = static_cast<FT>(sin(CGAL::to_double(theta * static_cast<FT>(CGAL_PI) / FT(180))));

        Vector v_dir = Vector(x, y);
        const Vector v_ort = Vector(-v_dir.y(), v_dir.x());
        
        const FT a = v_ort.x();
        const FT b = v_ort.y();

        // Rotate segments with precision.
        for (std::size_t i = 0; i < group.size(); ++i) {
          std::size_t seg_index = group[i];

          // Compute equation of the supporting line of the rotated segment.
          const Point &barycentre = m_segments[seg_index].m_barycentre;
          const FT c = -a * barycentre.x() - b * barycentre.y();

          set_orientation(seg_index, theta - m_segments[seg_index].m_orientation, a, b, c, v_dir);
        }

      }

    }


  private:
    // Fields.
    Input_range& m_input_range;
    const Segment_map  m_segment_map;
    std::vector<Segment_data> m_segments;
    std::map <std::pair<std::size_t, std::size_t>, FT> m_t_ijs;
    std::map <std::pair<std::size_t, std::size_t>, FT> m_r_ijs;
    const FT m_mu_ij;
    std::shared_ptr<Grouping> m_grouping_ptr;
    std::map<FT, std::vector<std::size_t>> m_parallel_groups_angle_map;

    void set_orientation(std::size_t i, const FT new_orientation, const FT a, const FT b, const FT c, const Vector &direction) {

      FT m_orientation = new_orientation;
      FT m_a = a;
      FT m_b = b;
      FT m_c = c;
      Vector m_direction = direction;
      
      if (m_direction.y() < FT(0) || (m_direction.y() == FT(0) && m_direction.x() < FT(0))) 
        m_direction = -m_direction;
      FT x1, y1, x2, y2;
      Point m_barycentre = m_segments[i].m_barycentre;
      FT m_length = m_segments[i].m_length;
      if (CGAL::abs(m_direction.x()) > CGAL::abs(m_direction.y())) { 
        x1 = m_barycentre.x() - m_length * m_direction.x() / FT(2);
        x2 = m_barycentre.x() + m_length * m_direction.x() / FT(2);

        y1 = (-m_c - m_a * x1) / m_b;
        y2 = (-m_c - m_a * x2) / m_b;
      } else {
        y1 = m_barycentre.y() - m_length * m_direction.y() / FT(2);
        y2 = m_barycentre.y() + m_length * m_direction.y() / FT(2);

        x1 = (-m_c - m_b * y1) / m_a;
        x2 = (-m_c - m_b * y2) / m_a;
      }
      const Point source = Point(x1, y1);
      const Point target = Point(x2, y2);

      m_input_range[i] = Segment(source, target);

    }

  };

} // namespace Regularization
} // namespace CGAL

#endif // CGAL_SHAPE_REGULARIZATION_ANGLE_REGULARIZATION_2
