#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <array>
#include <unordered_map>

inline long long encode(int x, int y) {
  return (static_cast<long long>(x) << 32) | static_cast<unsigned int>(y);
}

int burst_improved(int n, std::unordered_map<long long, int>& nodes, int x, int y) {
  std::array<int, 4> dx = {0, 1, 0, -1};
  std::array<int, 4> dy = {1, 0, -1, 0};
  int dir = 3;
  int infections = 0;

  for (int i = 0; i < n; ++i) {
    long long key = encode(x, y);
    int status = nodes.count(key) ? nodes[key] : 0;
    nodes[key] = (status + 1) & 3; // modulo 4 via bitmask
    infections += (status == 1);
    dir = (dir + ((status == 0) ? -1 : (status == 2) ? 1 : (status == 3) ? 2 : 0)) & 3;
    x += dx[dir];
    y += dy[dir];
  }
  return infections;
}

#ifndef STANDALONE
// [[Rcpp::export]]
int burst_improved(int n, const LogicalMatrix& node_map) {
  std::unordered_map<long long, int> nodes;
  int nrow = node_map.nrow();
  int ncol = node_map.ncol();

  for (int i = 0; i < nrow; ++i) {
    for (int j = 0; j < ncol; ++j) {
      if (node_map(i, j)) {
        nodes[encode(i + 1, j + 1)] = 2;
      }
    }
  }
  return burst_improved(n, nodes, (nrow + 1) / 2, (ncol + 1) / 2);
}

#else
int main() {
  std::unordered_map<long long, int> node_map = {{encode(1, 3), 2}, {encode(2, 1), 2}};
  std::cout << burst_improved(100, node_map, 2, 2) << std::endl;
  return 0;
}
#endif
