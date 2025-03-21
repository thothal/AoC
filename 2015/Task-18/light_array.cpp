#include <Rcpp.h>
using namespace Rcpp;

// [[Rcpp::export]]
int count_lights(CharacterMatrix light_array, int iterations, 
                  bool consider_corners= true) {
  int n = light_array.rows();
  int m = light_array.cols();
  int cnt_on;
  CharacterMatrix new_state(clone(light_array));
  for (auto k = 1; k <= iterations; k++) {
    for (auto i = 0; i < n; i++) {
      for (auto j = 0; j < m; j++) {
        cnt_on = 0;
        bool is_corner = (i == 0 && j == 0) || 
          (i == 0 && j == m - 1) ||
          (i == n - 1 && j == 0) || 
          (i == n - 1 && j == m - 1);
        if (consider_corners || !is_corner) {
          for (auto offset_i = -1; offset_i <= 1; offset_i++) {
            for (auto offset_j = -1; offset_j <= 1; offset_j++) {
              if ((i + offset_i >= 0 && i + offset_i < n) &&
                  (j + offset_j >= 0 && j + offset_j < m) &&
                  !(offset_j == 0 && offset_i == 0)) {
                  if (light_array(j + offset_j, i + offset_i) == "#") {
                    cnt_on++;
                  }
              }
            }
          }
          new_state(j, i) = (light_array(j, i) == "#" && 
            (cnt_on == 2 || cnt_on == 3)) || 
            (light_array(j, i) == "." && cnt_on == 3) ? "#" : ".";
        }
      }
    }
    light_array = clone(new_state);
  }
  cnt_on = 0;
  for (auto i = 0; i < n; i++) {
    for (auto j = 0; j < m; j++) {
      if (light_array(j, i) == "#") {
        cnt_on++;
      }
    }
  }
  return cnt_on;
}
