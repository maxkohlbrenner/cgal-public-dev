#include <CGAL/Cartesian_d.h>
#include <CGAL/Filtered_kernel_d.h>
#include <CGAL/point_generators_d.h>
#include <CGAL/Triangulation.h>
#include <CGAL/algorithm.h>
#include <CGAL/Random.h>
#include <iterator>
#include <iostream>
#include <vector>

typedef CGAL::Cartesian_d<double>  K;
typedef CGAL::Filtered_kernel_d<K> FK;
typedef CGAL::Triangulation<FK>    T;

int main()
{
    const int D = 5;   // we work in euclidean 5-space
    const int N = 10; // we will insert 100 points
    // - - - - - - - - - - - - - - - - - - - - - - - - STEP 1
    CGAL::Random rng;                             
    typedef CGAL::Random_points_in_cube_d<T::Point> Random_points_iterator;
    Random_points_iterator rand_it(D, 1.0, rng);
    std::vector<T::Point> points;
    CGAL::copy_n(rand_it, N, std::back_inserter(points));

    T t(D);                                   // create triangulation
    assert(t.empty());
    t.insert(points.begin(), points.end());   // compute triangulation
    assert( t.is_valid() );

    // - - - - - - - - - - - - - - - - - - - - - - - - STEP 2
    typedef T::Face Face;
    typedef std::vector<Face> Faces;
    Faces edges;
    std::back_insert_iterator<Faces> out(edges);
    t.incident_faces(t.infinite_vertex(), 1, out);  // collect edges
    std::cout << "There are " << edges.size() 
	      << " vertices on the convex hull."<< std::endl;
    edges.clear();
    t.clear();
    assert(t.empty());
    return 0;
}
