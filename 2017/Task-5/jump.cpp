#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif

#include <vector>

int count_jumps(std::vector<int> ops, bool decrease = false) {
  int i = 0;
  int jumps = 0;
  while (i >= 0 && i <= (int)ops.size() - 1) {
    int j = i + ops[i];
    if (decrease && ops[i] >= 3) {
      ops[i] -= 1;
    } else {
      ops[i] += 1;
    }
    i = j;
    ++jumps;
  }
  return jumps;
}

#ifndef STANDALONE
// [[Rcpp::export]]
int count_jumps(const IntegerVector& ops, bool decrease = false) {
  std::vector<int> v(ops.begin(), ops.end());
  return count_jumps(v, decrease);
}
#else
int main() {
  std::vector<int> ops = {0, 3, 0, 1, -3};
  int op;
  std::cout << count_jumps(ops, false) << std::endl;
  std::cout << count_jumps(ops, true) << std::endl;
  return 0;
}
#endif
