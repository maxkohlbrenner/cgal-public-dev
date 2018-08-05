// Author: Keyu Chen.
// We test speed of maximum entropy coordinates on a set of automatically generated
// points inside a regular polygon with 100 vertices. We use inexact kernel.

#include <CGAL/Real_timer.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Barycentric_coordinates_2/Maximum_entropy_2.h>
#include <CGAL/Barycentric_coordinates_2/Maximum_entropy_2/Maximum_entropy_parameters.h>
#include <CGAL/Barycentric_coordinates_2/Maximum_entropy_2/Maximum_entropy_solver.h>
#include <CGAL/Barycentric_coordinates_2/Maximum_entropy_2/Maximum_entropy_prior_function_type_one.h>
#include <CGAL/Barycentric_coordinates_2/Generalized_barycentric_coordinates_2.h>
#include <CGAL/property_map.h>

typedef CGAL::Real_timer Timer;

typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;

typedef Kernel::FT      Scalar;
typedef Kernel::Point_2 Point;

typedef std::vector<Scalar> Scalar_vector;
typedef std::vector<Point>  Point_vector;

typedef std::pair<Point, bool> Point_with_property;
typedef CGAL::First_of_pair_property_map<Point_with_property> Point_map;
typedef std::vector<Point_with_property> Input_range;

typedef Scalar_vector::iterator Overwrite_iterator;

typedef CGAL::Barycentric_coordinates::Maximum_entropy_parameters<Kernel> MEC_parameters;
typedef CGAL::Barycentric_coordinates::Maximum_entropy_newton_solver<Kernel> MEC_newton_solver;
typedef CGAL::Barycentric_coordinates::Maximum_entropy_prior_function_type_one<Kernel> MEC1_prior;

typedef CGAL::Barycentric_coordinates::Maximum_entropy_2<Kernel, MEC1_prior, MEC_newton_solver, MEC_parameters> Maximum_entropy;
typedef CGAL::Barycentric_coordinates::Generalized_barycentric_coordinates_2<Maximum_entropy, Input_range, Point_map, Kernel> Maximum_entropy_coordinates;

using std::cout; using std::endl; using std::string;

void generate_regular_polygon(const int number_of_vertices, const double polygon_radius, Point_vector &vertices)
{
    const int n = number_of_vertices;
    const double r =  polygon_radius;
    const double number_pi = 3.14159;

    vertices.resize(n);

    for(int i = 0; i < n; ++i)
        vertices[i] = Point(Scalar(r*sin((number_pi / n) + ((i * 2.0 * number_pi) / n))), Scalar(-r*cos((number_pi / n) + ((i * 2.0 * number_pi) / n))));
}

int main()
{
    const int number_of_x_coordinates = 1000;
    const int number_of_y_coordinates = 1000;
    const int number_of_runs          = 1;

    const Scalar one  = Scalar(1);
    const Scalar x_step = one / Scalar(number_of_x_coordinates);
    const Scalar y_step = one / Scalar(number_of_y_coordinates);

    Point_vector vertices;

    const int number_of_vertices = 100;
    const double  polygon_radius = 2;

    generate_regular_polygon(number_of_vertices, polygon_radius, vertices);

    Input_range point_range(number_of_vertices);
    for(size_t i = 0; i < number_of_vertices; ++i)
    {
        point_range[i]=Point_with_property(vertices[i],false);
    }

    Maximum_entropy_coordinates maximum_entropy_coordinates(point_range, Point_map());

    Scalar_vector coordinates(number_of_vertices);
    Overwrite_iterator it = coordinates.begin();

    Timer time_to_compute;

    double time = 0.0;
    for(int i = 0; i < number_of_runs; ++i) {

        time_to_compute.start();
        for(Scalar x = -one; x <= one; x += x_step) {
            for(Scalar y = -one; y <= one; y += y_step)
                maximum_entropy_coordinates.compute(Point(x, y), it, CGAL::Barycentric_coordinates::ON_BOUNDED_SIDE);
        }
        time_to_compute.stop();

        time += time_to_compute.time();

        time_to_compute.reset();
    }
    const double mean_time = time / number_of_runs;

    cout.precision(10);
    cout << endl << "CPU time to compute Maximum Entropy coordinates (100 vertices) = " << mean_time << " seconds." << endl << endl;

    return EXIT_SUCCESS;
}
