#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif

#include <string>
#include <vector>

int count_lights(const std::vector<std::vector<char>>& light_array,
                 int iterations,
                 bool consider_corners = true) {
  int n = light_array.size();
  int m = light_array[0].size();
  std::vector<std::vector<char>> state = light_array;
  std::vector<std::vector<char>> new_state = state;

  for (int k = 1; k <= iterations; k++) {
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < m; j++) {
        int cnt_on = 0;
        bool is_corner = (i == 0 && j == 0) || (i == 0 && j == m - 1) || (i == n - 1 && j == 0) ||
            (i == n - 1 && j == m - 1);
        if (consider_corners || !is_corner) {
          for (int offset_i = -1; offset_i <= 1; offset_i++) {
            for (int offset_j = -1; offset_j <= 1; offset_j++) {
              int ni = i + offset_i;
              int nj = j + offset_j;
              if (ni >= 0 && ni < n && nj >= 0 && nj < m && !(offset_i == 0 && offset_j == 0)) {
                if (state[ni][nj] == '#') {
                  cnt_on++;
                }
              }
            }
          }
          if ((state[i][j] == '#' && (cnt_on == 2 || cnt_on == 3)) ||
              (state[i][j] == '.' && cnt_on == 3)) {
            new_state[i][j] = '#';
          } else {
            new_state[i][j] = '.';
          }
        }
      }
    }
    state = new_state;
  }

  int cnt_on = 0;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      if (state[i][j] == '#') {
        cnt_on++;
      }
    }
  }
  return cnt_on;
}

#ifndef STANDALONE
// [[Rcpp::export]]
int count_lights(CharacterMatrix light_array, int iterations, bool consider_corners = true) {
  int n = light_array.rows();
  int m = light_array.cols();
  std::vector<std::vector<char>> stl_array(n, std::vector<char>(m));
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      stl_array[i][j] = Rcpp::as<std::string>(light_array(i, j))[0];
    }
  }
  return count_lights(stl_array, iterations, consider_corners);
}
#else
int main() {
  std::vector<std::vector<char>> light_array = {{'.', '#', '.', '#', '.', '#'},
                                                {'.', '.', '.', '#', '#', '.'},
                                                {'#', '.', '.', '.', '.', '#'},
                                                {'.', '.', '#', '.', '.', '.'},
                                                {'#', '.', '#', '.', '.', '#'},
                                                {'#', '#', '#', '#', '.', '.'}};
  int iterations = 4;
  bool consider_corners = true;
  int result = count_lights(light_array, iterations, consider_corners);
  std::cout << "Number of lights on after " << iterations << " iterations: " << result << std::endl;
  return 0;
}
#endif
