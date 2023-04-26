#include <iostream>
#include <CGAL/array.h>
#include <CGAL/use.h>

struct B {};

struct A // move-only class, move-constructible from B
{
  A(B&&) {}

  A(const A&) = delete;
  A(A&&) = default;
};

int main()
{
  // this test requires C++17 mandatory return-value optimization (RVO)
  std::array<A, 1> a = CGAL::fwd_make_array<A>(B());
  std::array<double, 1> b = CGAL::fwd_make_array<double>(1u);
  CGAL_USE(a);
  CGAL_USE(b);
  return 0;
}
