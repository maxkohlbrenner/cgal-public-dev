// Test of Algebraic_kernel

#define CGAL_TEST_ALL_AK_VARIANTS 1

#include "Algebraic_kernel_d_1.h"

int main() {
#ifdef CGAL_HAS_CORE_ARITHMETIC_KERNEL
  std::cout << " TEST AK1 USING CORE " << std::endl;

  typedef CGAL::CORE_arithmetic_kernel AK;
  typedef AK::Integer Integer;
  typedef AK::Rational Rational;

  test_algebraic_kernel_coeff_bound<Integer, Rational>();
#if CGAL_TEST_ALL_AK_VARIANTS
  test_algebraic_kernel_coeff_bound<Rational, Rational>();
#endif
#else
  std::cout << " NOTHING TESTED " << std::endl;
#endif
  return 0;
}
