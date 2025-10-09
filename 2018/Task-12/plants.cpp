#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <unordered_map>
#include <unordered_set>

long long count_plants(long long n,
                       std::string start,
                       const std::unordered_map<std::string, char>& rules) {
  std::unordered_set<std::string> hash;
  hash.insert(start);
  int offset = 0;
  for (long long i = 0; i < n; ++i) {
    std::string padded = "..." + start + "...";
    std::string next;
    for (size_t j = 2; j < padded.size() - 2; ++j) {
      std::string segment = padded.substr(j - 2, 5);
      auto it = rules.find(segment);
      if (it != rules.end()) {
        next += it->second;
      } else {
        next += '.';
      }
    }
    if (hash.find(next) != hash.end()) {
      Rcout << std::endl << "Cycle detected after " << i << " iterations." << std::endl;
      long long cycle_length = i;
      long long remaining = n - i - 1;
      long long cycles = remaining / cycle_length;
      i += cycles * cycle_length;
      offset += cycles * (first_non_dot - 2); // adjust for padding
      break;
    }
    hash.insert(next);
    start = next;
  }
  long long sum = 0;
  Rcout << "\n" << start << "\tOffset:" << offset << std::endl;
  for (size_t i = 0; i < start.size(); ++i) {
    if (start[i] == '#') {
      sum += (i + offset - 2); // adjust for padding
    }
  }
  return sum;
}

#ifndef STANDALONE
// [[Rcpp::export]]
long long count_plants(long long n, const std::string& start, const CharacterVector& rules) {
  std::unordered_map<std::string, char> rule_map;
  CharacterVector nms = rules.names();

  for (R_xlen_t i = 0; i < rules.size(); ++i) {
    std::string name = as<std::string>(nms[i]);
    std::string value = as<std::string>(rules[i]);
    rule_map[name] = value[0];
  }
  return count_plants(n, start, rule_map);
}
#else
int main() {
  std::string start = "#..#.#..##......###...###";
  std::unordered_map<std::string, char> rules = {
      {"...##", '#'}, {"..#..", '#'}, {".#...", '#'}, {".#.#.", '#'}, {".#.##", '#'},
      {".##..", '#'}, {".####", '#'}, {"#.#.#", '#'}, {"#.###", '#'}, {"##.#.", '#'},
      {"##.##", '#'}, {"###..", '#'}, {"###.#", '#'}, {"####.", '#'}, {".....", '.'},
      {"#....", '.'}, {"##...", '.'}, {"#.#..", '.'}, {"...#.", '.'}, {"#..#.", '.'},
      {"..##.", '.'}, {"#.##.", '.'}, {".###.", '.'}, {"....#", '.'}, {"#...#", '.'},
      {".#..#", '.'}, {"##..#", '.'}, {"..#.#", '.'}, {".##.#", '.'}, {"#..##", '.'},
      {"..###", '.'}, {"#####", '.'}};
  std::cout << count_plants(20, start, rules) << std::endl;
  return 1;
}
#endif