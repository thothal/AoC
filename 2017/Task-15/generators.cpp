#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif

// [[Rcpp::export]]
int count_matches(long long gen_a, long long gen_b, bool picky = false) {
  const long long factor_a = 16807;
  const long long factor_b = 48271;
  const long long divisor = 2147483647;
  int matches = 0;
  for (int count = 0; count < (picky ? 5000000 : 40000000); ++count) {
    gen_a = (gen_a * factor_a) % divisor;
    if (picky) {
      while (gen_a % 4 != 0) {
        gen_a = (gen_a * factor_a) % divisor;
      }
    }
    gen_b = (gen_b * factor_b) % divisor;
    if (picky) {
      while (gen_b % 8 != 0) {
        gen_b = (gen_b * factor_b) % divisor;
      }
    }
    if ((gen_a & 0xFFFF) == (gen_b & 0xFFFF)) {
      matches++;
    }
  }
  return matches;
}

#ifdef STANDALONE
int main() {
  long long gen_a = 277;
  long long gen_b = 349;
  std::cout << count_matches(gen_a, gen_b) << std::endl;
  std::cout << count_matches(gen_a, gen_b, true) << std::endl;
}
#endif
