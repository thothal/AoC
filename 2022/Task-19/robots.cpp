#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <array>
#include <cstdint>
#include <unordered_map>

struct State {
    int round;
    std::array<int, 4> robots;
    std::array<int, 4> resources;
};

namespace keying {
constexpr unsigned B_robots = 5; // robots[i] <= 31  -> 5 Bits
constexpr unsigned B_resources = 9; // resources[i] <= 496 -> 9 Bits (0..511)
constexpr unsigned B_rounds = 6;

inline uint64_t maskN(unsigned bits) {
  return (bits == 64) ? ~0ull : ((1ull << bits) - 1);
}

inline uint64_t encode64(const State& st) {
  uint64_t key = 0;
  unsigned pos = 0;

  auto push = [&](uint64_t v, unsigned bits) {
    key |= (v & maskN(bits)) << pos;
    pos += bits;
  };

  for (int i = 0; i < 4; ++i) {
    push(static_cast<uint64_t>(st.robots[i]), B_robots);
    push(static_cast<uint64_t>(st.resources[i]), B_resources);
  }
  push(static_cast<uint64_t>(st.round), B_rounds);
  return key;
}

inline State decode64(uint64_t key) {
  State st {};
  unsigned pos = 0;

  auto pop = [&](unsigned bits) -> uint64_t {
    uint64_t v = (key >> pos) & maskN(bits);
    pos += bits;
    return v;
  };

  for (int i = 0; i < 4; ++i) {
    st.robots[i] = static_cast<int>(pop(B_robots));
    st.resources[i] = static_cast<int>(pop(B_resources));
  }
  st.round = static_cast<int>(pop(B_rounds));
  return st;
}
} // namespace keying

class Blueprint {
  private:
    int nr_rounds_;
    std::array<std::array<int, 3>, 4> costs_;
    std::array<int, 3> max_costs_;
    int solve(int, std::array<int, 4>, std::array<int, 4>, int);
    int upper_bound(int, std::array<int, 4>, std::array<int, 4>);
    std::array<int, 4> cap_resources(int, std::array<int, 4>);
    std::unordered_map<uint64_t, int> memo_;

  public:
    Blueprint(int, const std::array<std::array<int, 3>, 4>&);
    int solve();
};

Blueprint::Blueprint(int nr_rounds, const std::array<std::array<int, 3>, 4>& costs)
    : nr_rounds_(nr_rounds)
    , costs_(costs) {
  for (int i = 0; i < 3; ++i) {
    int max_cost = 0;
    for (int j = 0; j < 4; ++j) {
      max_cost = std::max(max_cost, costs_[j][i]);
    }
    max_costs_[i] = max_cost;
  }
}

int Blueprint::solve(int round,
                     std::array<int, 4> robots,
                     std::array<int, 4> resources,
                     int best_so_far) {
  if (round == 0) {
    return std::max(best_so_far, resources[3]);
  }
  if (upper_bound(round, robots, resources) <= best_so_far) {
    return best_so_far;
  }
  State st {round, robots, cap_resources(round, resources)};
  uint64_t key = keying::encode64(st);
  auto it = memo_.find(key);
  if (it != memo_.end()) {
    return std::max(best_so_far, it->second);
  }
  int best_local = 0;
  std::array<int, 4> produced = resources;
  for (int i = 0; i < 4; ++i) {
    produced[i] += robots[i];
  }
  for (int robot_type = 3; robot_type >= 0; --robot_type) {
    if (robot_type != 3 && robots[robot_type] >= max_costs_[robot_type]) {
      // skip building more of this robots as we produce already the maximum needed
      continue;
    }
    bool can_build = true;
    for (int resource_type = 0; resource_type < 3; ++resource_type) {
      if (costs_[robot_type][resource_type] > resources[resource_type]) {
        can_build = false;
        break;
      }
    }
    if (can_build) {
      std::array<int, 4> new_resources = produced;
      for (int resource_type = 0; resource_type < 3; ++resource_type) {
        new_resources[resource_type] -= costs_[robot_type][resource_type];
      }
      std::array<int, 4> new_robots = robots;
      new_robots[robot_type] += 1;
      if (robot_type == 3) {
        // always build geode robot if possible
        best_local = std::max(best_local, solve(round - 1, new_robots, new_resources, best_so_far));
        memo_[key] = best_local;
        return best_local;
      }
      best_local = std::max(best_local, solve(round - 1, new_robots, new_resources, best_so_far));
    }
  }
  // produce nothing
  best_local = std::max(best_local, solve(round - 1, robots, produced, best_so_far));
  memo_[key] = best_local;
  return best_local;
}

int Blueprint::upper_bound(int round, std::array<int, 4> robots, std::array<int, 4> resources) {
  return resources[3] + robots[3] * round + (round * (round - 1)) / 2;
}

std::array<int, 4> Blueprint::cap_resources(int round, std::array<int, 4> resources) {
  std::array<int, 4> capped;
  for (int i = 0; i < 3; ++i) {
    capped[i] = std::min(resources[i], max_costs_[i] * round);
  }
  capped[3] = resources[3];
  return capped;
}

int Blueprint::solve() {
  int res = solve(nr_rounds_, {1, 0, 0, 0}, {0, 0, 0, 0}, 0);
  return res;
}

#ifndef STANDALONE
// [[Rcpp::export]]
int solve_blueprint(int nr_rounds, const List& blueprint) {
  std::array<std::array<int, 3>, 4> costs;
  for (int i = 0; i < blueprint.size(); ++i) {
    IntegerVector cost_vec = blueprint[i];
    for (int j = 0; j < 3; ++j) {
      costs[i][j] = cost_vec[j];
    }
  }
  Blueprint bp(nr_rounds, costs);
  return bp.solve();
}
#else
int main() {
  Blueprint bp(32, {{{2, 0, 0}, {3, 0, 0}, {3, 8, 0}, {3, 0, 12}}});
  int res = bp.solve();
  std::cout << "# geodes produced: " << res << std::endl;
  return 0;
}
#endif