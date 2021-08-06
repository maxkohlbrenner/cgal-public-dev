#ifndef AABB_TREE_TESTS_AABB_TRAITS_CONSTRUCT_BY_SORTING_H
#define AABB_TREE_TESTS_AABB_TRAITS_CONSTRUCT_BY_SORTING_H

#include <CGAL/AABB_traits.h>

namespace CGAL {

  // forward declaration
  template<typename AABBTraits>
  class AABB_tree;


  template<typename GeomTraits, typename AABBPrimitive, typename BboxMap = Default>
  class AABB_traits_construct_by_sorting : public AABB_traits<GeomTraits, AABBPrimitive, BboxMap> {

  public:

    class Split_primitives {
      typedef AABB_traits<GeomTraits, AABBPrimitive, BboxMap> Traits;
      const Traits &m_traits;
      mutable bool has_been_sorted = false;

    public:

      struct Get_reference_point : public std::unary_function<const Primitive &, typename Traits::Point_3> {
        const Traits &m_traits;
        typedef internal::Primitive_helper<Traits> Helper;

        Get_reference_point(const AABB_traits<GeomTraits, AABBPrimitive, BboxMap> &traits)
                : m_traits(traits) {}

        typename Traits::Point_3 operator()(const Primitive &p) const {
          return Helper::get_reference_point(p, m_traits);
        }
      };

      Split_primitives(const AABB_traits<GeomTraits, AABBPrimitive, BboxMap> &traits)
              : m_traits(traits) {}

      template<typename PrimitiveIterator>
      void operator()(PrimitiveIterator first,
                      PrimitiveIterator beyond,
                      const typename AT::Bounding_box &bbox) const {

        // If this is our first time splitting the primitives, sort them along the hilbert curve
        // This should generally put nearby primitives close together in the list
        if (!has_been_sorted) {

          // Create a property map using our Get_reference_point functor
          auto property_map = boost::make_function_property_map<Primitive, Traits::Point_3, Get_reference_point>(
                  Get_reference_point(m_traits)
          );

          // Our search traits will use that property map
          typedef CGAL::Spatial_sort_traits_adapter_3<Geom_traits, decltype(property_map)> Search_traits_3;

          // Perform our hilbert sort using the search traits type with our custom property map
          CGAL::hilbert_sort<CGAL::Parallel_if_available_tag>(first, beyond, Search_traits_3(property_map));

          // In the future, it's not necessary to re-sort the primitives (we can blindly partition them in the middle)
          has_been_sorted = true;
        }
      }
    };

    Split_primitives split_primitives_object() const { return Split_primitives(*this); }
  };
}

#endif //AABB_TREE_TESTS_AABB_TRAITS_CONSTRUCT_BY_SORTING_H
