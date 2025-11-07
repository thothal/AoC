#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <algorithm>
#include <cmath>
#include <tuple>
#include <vector>

struct Ball {
    long long x, y, z, r;
};

std::pair<std::vector<int>, long long> count_nanobots(std::vector<Ball> balls,
                                                      int coarse_grid = 10,
                                                      int fine_grid = 50) {
  size_t n = balls.size();
  std::vector<long long> coords1, coords2, coords3;
  coords1.reserve(2 * n);
  coords2.reserve(2 * n);
  coords3.reserve(2 * n);
  // transform centers to u-coordinates (axes parallel)
  for (size_t i = 0; i < n; ++i) {
    long long x = balls[i].x, y = balls[i].y, z = balls[i].z, r = balls[i].r;
    long long u1 = x + y + z;
    long long u2 = x + y - z;
    long long u3 = x - y + z;
    coords1.push_back(u1 - r);
    coords1.push_back(u1 + r);
    coords2.push_back(u2 - r);
    coords2.push_back(u2 + r);
    coords3.push_back(u3 - r);
    coords3.push_back(u3 + r);
  }

  // create sorted list without duplicates
  auto compress = [](std::vector<long long>& v) {
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    return v;
  };

  // check if point inside L1-ball
  auto is_in_ball = [](long long x, long long y, long long z, const Ball& b) {
    return llabs(x - b.x) + llabs(y - b.y) + llabs(z - b.z) <= b.r;
  };

  // back transformation u->(x,y,z)
  auto inv_from_u = [](long long u1, long long u2, long long u3) {
    // x=(u2+u3)/2, y=(u1-u3)/2, z=(u1-u2)/2
    if (((u2 + u3) & 1) || ((u1 - u3) & 1) || ((u1 - u2) & 1)) {
      // x, y or z is not an integer
      return std::make_tuple(false, 0LL, 0LL, 0LL);
    }
    long long x = (u2 + u3) / 2;
    long long y = (u1 - u3) / 2;
    long long z = (u1 - u2) / 2;
    return std::make_tuple(true, x, y, z);
  };

  // update best point if needed
  auto update_best_point = [balls, is_in_ball](
                               long long x,
                               long long y,
                               long long z,
                               long long& best_count,
                               long long& best_dist,
                               std::tuple<long long, long long, long long>& best_point) {
    long long count = 0;
    for (auto& b : balls) {
      if (is_in_ball(x, y, z, b)) {
        ++count;
      }
    }
    long long dist = llabs(x) + llabs(y) + llabs(z);
    if (count > best_count || (count == best_count && dist < best_dist)) {
      best_count = count;
      best_dist = dist;
      best_point = std::make_tuple(x, y, z);
    }
  };

  coords1 = compress(coords1);
  coords2 = compress(coords2);
  coords3 = compress(coords3);

  long long best_count = -1, best_dist = LLONG_MAX;
  std::tuple<long long, long long, long long> best_point(0, 0, 0);

  // Phase 1: Coarse search, sample points from all borders
  std::vector<long long> c1, c2, c3;
  for (size_t i = 0; i < coords1.size(); i += coarse_grid) {
    c1.push_back(coords1[i]);
  }
  for (size_t i = 0; i < coords2.size(); i += coarse_grid) {
    c2.push_back(coords2[i]);
  }
  for (size_t i = 0; i < coords3.size(); i += coarse_grid) {
    c3.push_back(coords3[i]);
  }
  long long x, y, z;
  for (long long u1 : c1) {
    for (long long u2 : c2) {
      for (long long u3 : c3) {
        bool ok;
        std::tie(ok, x, y, z) = inv_from_u(u1, u2, u3);
        if (!ok) {
          continue;
        }
        update_best_point(x, y, z, best_count, best_dist, best_point);
      }
    }
  }

  // Phase 2: Refinement
  if (coarse_grid > 1) {
    auto [bx, by, bz] = best_point;
    for (int dx = -fine_grid; dx <= fine_grid; ++dx) {
      for (int dy = -fine_grid; dy <= fine_grid; ++dy) {
        for (int dz = -fine_grid; dz <= fine_grid; ++dz) {
          x = bx + dx;
          y = by + dy;
          z = bz + dz;
          update_best_point(x, y, z, best_count, best_dist, best_point);
        }
      }
    }
  }
  std::vector<int> pos(3);
  pos[0] = std::get<0>(best_point);
  pos[1] = std::get<1>(best_point);
  pos[2] = std::get<2>(best_point);
  return std::make_pair(pos, best_count);
}
#ifndef STANDALONE

// [[Rcpp::export]]
List count_nanobots(const IntegerMatrix& bots, int coarse_grid = 10, int fine_grid = 50) {
  size_t n = bots.nrow();
  std::vector<Ball> balls(n);
  for (size_t i = 0; i < n; ++i) {
    balls[i].x = bots(i, 0);
    balls[i].y = bots(i, 1);
    balls[i].z = bots(i, 2);
    balls[i].r = bots(i, 3);
  }
  auto result = count_nanobots(balls, coarse_grid, fine_grid);
  return List::create(Named("pos") = result.first, Named("count") = result.second);
}

#else
int main() {
  std::vector<Ball> balls = {{10, 12, 12, 2},
                             {12, 14, 12, 2},
                             {16, 12, 12, 4},
                             {14, 14, 14, 6},
                             {50, 50, 50, 200},
                             {10, 10, 10, 5}};
  auto result = count_nanobots(balls, 1, 1);
  for (int i = 0; i < result.first.size(); ++i) {
    std::cout << result.first[i] << " ";
  }
  return 0;
}
#endif