#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <vector>

int find_longest_path(std::vector<std::vector<int>>& adj_list, int start, int goal) {
  // Implement DFS to find the longest path from start to goal
  const size_t n = adj_list.size();
  std::vector<bool> visited(n, false);
  visited[start] = true;
  int longest_path = 0, act_path = 0;

  // use vector instead of stack as it is faster
  std::vector<std::pair<int, int>> stack; // node id and next neighbor
  stack.reserve(n - 1);
  stack.emplace_back(start, 0);

  auto prune = [&](int u) {
    visited[u] = false;
    act_path--;
    stack.pop_back();
  };
  
  while (!stack.empty()) {
    auto& top = stack.back();
    int u = top.first;
    int idx = top.second;

    if (u == goal) {
      if (act_path > longest_path) {
        longest_path = act_path;
      }
      prune(u);
      continue;
    }

    const auto& neighbors = adj_list[u];
    if (idx >= static_cast<int> (neighbors.size())) {
      prune(u);
      continue;
    }

    int v = neighbors[idx];
    top.second++;

    if (!visited[v]) {
      visited[v] = true;
      ++act_path;
      stack.emplace_back(v, 0);
    }
  }
  return longest_path;
}

#ifndef STANDALONE
// [[Rcpp::export]]
int find_longest_path(List adj_list, int start, int goal) {
  std::vector<std::vector<int>> adj(adj_list.size());
  for (int i = 0; i < adj_list.size(); ++i) {
    IntegerVector neighbors = adj_list[i];
    adj[i] = std::vector<int>(neighbors.begin(), neighbors.end());
  }
  return find_longest_path(adj, start, goal);
}
#else
int main() {
  return 0;
}
#endif