#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <unordered_map>
#include <utility>
#include <vector>

int count_non_empty(long n, std::vector<std::string>& acre) {

  int nrow = acre.size();
  int ncol = acre[0].size();

  std::vector<std::pair<int, int>> dirs = {
      {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};

  auto count_neighbors = [&](int i, int j, const std::vector<std::string>& grid, char type) {
    int cnt = 0;
    for (auto& d : dirs) {
      int ni = i + d.first;
      int nj = j + d.second;
      if (ni >= 0 && ni < nrow && nj >= 0 && nj < ncol) {
        if (grid[ni][nj] == type)
          cnt++;
      }
    }
    return cnt;
  };

  auto make_key = [&](const std::vector<std::string>& grid) {
    std::string key;
    for (const auto& row : grid) {
      key += row;
    }
    return key;
  };

  std::unordered_map<std::string, long> seen_index;
  std::vector<std::string> states;

  std::string key = make_key(acre);
  seen_index[key] = 0;
  states.push_back(key);
  for (long minute = 1; minute <= n; ++minute) {
    std::vector<std::string> next = acre;

    for (int i = 0; i < nrow; i++) {
      for (int j = 0; j < ncol; j++) {
        char type = acre[i][j];

        if (type == '.') {
          int cnt = count_neighbors(i, j, acre, '|');
          next[i][j] = (cnt >= 3) ? '|' : '.';
        } else if (type == '|') {
          int cnt = count_neighbors(i, j, acre, '#');
          next[i][j] = (cnt >= 3) ? '#' : '|';
        } else if (type == '#') {
          int cnt_hash = count_neighbors(i, j, acre, '#');
          int cnt_tree = count_neighbors(i, j, acre, '|');
          next[i][j] = (cnt_hash >= 1 && cnt_tree >= 1) ? '#' : '.';
        }
      }
    }
    key = make_key(next);

    if (seen_index.find(key) != seen_index.end()) {
      long first_seen = seen_index[key];
      long cycle_length = minute - first_seen;

      long remaining = n - minute;
      long target_index = first_seen + ((n - first_seen) % cycle_length);

      std::string target_key = states[target_index];
      int pos = 0;
      for (int i = 0; i < nrow; i++) {
        for (int j = 0; j < ncol; j++) {
          acre[i][j] = target_key[pos++];
        }
      }
      break;
    } else {
      seen_index[key] = minute;
      states.push_back(key);
      acre = next;
    }
  }

  int cnt_tree = 0, cnt_lumber = 0;
  for (int i = 0; i < nrow; i++) {
    for (int j = 0; j < ncol; j++) {
      char type = acre[i][j];
      if (type == '|') {
        cnt_tree++;
      } else if (type == '#') {
        cnt_lumber++;
      }
    }
  }

  return cnt_tree * cnt_lumber;
}
#ifndef STANDALONE
// [[Rcpp::export]]
int count_non_empty_fast(long n, CharacterMatrix acre) {
  int nrow = acre.nrow();
  int ncol = acre.ncol();

  std::vector<std::string> acre_vec(nrow);
  for (int i = 0; i < nrow; i++) {
    std::string row_str;
    for (int j = 0; j < ncol; j++) {
      row_str += Rcpp::as<std::string>(acre(i, j));
    }
    acre_vec[i] = row_str;
  }

  return count_non_empty(n, acre_vec);
}

#else
int main() {
  std::vector<std::string> acre = {".#.#...|#.",
                                   ".....#|##|",
                                   ".|..|...#.",
                                   "..|#.....#",
                                   "#.#|||#|#|",
                                   "...#.||...",
                                   ".|....|...",
                                   "||...#|.#|",
                                   "|.||||..|.",
                                   "...#.|..|."};
  std::cout << count_non_empty(10, acre) << std::endl;
  return 0;
}
#endif
