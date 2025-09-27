#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <vector>

int get_pos(int t, int n) {
  int m = t % (2 * (n - 1));
  if (m <= n - 1) {
    return m;
  } else {
    return 2 * (n - 1) - m;
  }
}

int find_offset(std::vector<std::pair<int, int>>& firewall) {
  int offset = 0;
  while (true) {
    bool caught = false;
    for (auto& p : firewall) {
      int depth = p.first;
      int range = p.second;
      if (get_pos(depth + offset, range) == 0) {
        caught = true;
        break;
      }
    }
    if (!caught) {
      return offset;
    }
    offset++;
  }
}

// [[Rcpp::export]]
int find_offset(DataFrame df) {
  std::vector<std::pair<int, int>> firewall;
  IntegerVector depth = df["depth"];
  IntegerVector range = df["range"];
  for (int i = 0; i < df.nrows(); ++i) {
    firewall.push_back({depth[i], range[i]});
  }
  return find_offset(firewall);
}

#ifdef STANDALONE
int main() {
  std::vector<std::pair<int, int>> firewall {{0, 3}, {1, 2}, {4, 4}, {6, 4}};
  std::cout << find_offset(firewall) << std::endl;
}
#endif