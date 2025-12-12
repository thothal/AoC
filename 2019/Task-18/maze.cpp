#ifndef STANDALONE
#include <Rcpp.h>
#define COUT Rcpp::Rcout
using namespace Rcpp;
#else
#define COUT std::cout
#include <iostream>
#endif
#include <array>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <vector>

struct Coord {
    int x;
    int y;
    char label;
    bool operator==(const Coord& other) const { return x == other.x && y == other.y; }
    Coord operator+(const Coord& other) const { return {x + other.x, y + other.y, 0}; }
    bool is_valid(int nrow, int ncol) const { return x >= 0 && x < ncol && y >= 0 && y < nrow; }
};

struct CoordHash {
    std::size_t operator()(const Coord& c) const {
      std::size_t h1 = std::hash<int> {}(c.x);
      std::size_t h2 = std::hash<int> {}(c.y);
      return h1 ^ (h2 << 1);
    }
};

struct State {
    Coord coord;
    int key_bitmask;
    bool operator==(const State& other) const {
      return coord == other.coord && key_bitmask == other.key_bitmask;
    }
};

struct StateHash {
    std::size_t operator()(const State& s) const {
      std::size_t h1 = CoordHash {}(s.coord);
      std::size_t h2 = std::hash<int> {}(s.key_bitmask);
      std::size_t hash = h1;
      hash ^= h2 + 0x9e3779b9 + (hash << 6) + (hash >> 2);
      return hash;
    }
};

struct MultiState {
    std::array<Coord, 4> coords;
    int key_bitmask;
    bool operator==(const MultiState& other) const {
      return coords == other.coords && key_bitmask == other.key_bitmask;
    }
    Coord& operator[](std::size_t i) { return coords[i]; }
    const Coord& operator[](std::size_t i) const { return coords[i]; }
};

struct MultiStateHash {
    std::size_t operator()(const MultiState& ms) const {
      std::size_t seed = 0;
      for (const auto& c : ms.coords) {
        std::size_t h = CoordHash {}(c);
        seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      std::size_t h2 = std::hash<int> {}(ms.key_bitmask);
      seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      return seed;
    }
};

struct Field {
    State state;
    int distance;
    bool operator==(const Field& other) const {
      return state == other.state && distance == other.distance;
    }
};

using Path = std::unordered_map<char, std::vector<Field>>;

Path bfs(const std::vector<std::string>& maze, const std::vector<Coord>& pois) {
  Path result;
  int nrow = maze.size();
  int ncol = maze[0].size();
  const std::array<Coord, 4> directions = {{{0, 1, '>'}, {1, 0, 'v'}, {0, -1, '<'}, {-1, 0, '^'}}};
  for (const auto& start : pois) {
    std::queue<Field> pq;
    std::vector<std::vector<bool>> visited(nrow, std::vector<bool>(ncol, false));
    pq.push({{start, 0}, 0});
    visited[start.y][start.x] = true;
    while (!pq.empty()) {
      auto field = pq.front();
      pq.pop();

      for (const auto& dir : directions) {
        Coord nb = field.state.coord + dir;
        if (!nb.is_valid(nrow, ncol) || maze[nb.y][nb.x] == '#' || visited[nb.y][nb.x]) {
          continue;
        }
        char cell = maze[nb.y][nb.x];
        int new_key_bitmask = field.state.key_bitmask;
        if (cell >= 'A' && cell <= 'Z') {
          new_key_bitmask |= (1 << (cell - 'A'));
        } else if (cell >= 'a' && cell <= 'z') {
          nb.label = cell;
          Field new_field = {{nb, new_key_bitmask}, field.distance + 1};
          result[start.label].push_back(new_field);
        }
        visited[nb.y][nb.x] = true;
        pq.push({{nb, new_key_bitmask}, field.distance + 1});
      }
    }
  }
  return result;
}

int get_key_path_length(const Path& path, const Coord& start, int nr_keys) {
  std::unordered_map<State, int, StateHash> dp;
  auto cmp = [](const std::pair<int, State>& a, const std::pair<int, State>& b) {
    return a.first > b.first;
  };
  std::priority_queue<std::pair<int, State>, std::vector<std::pair<int, State>>, decltype(cmp)> pq(
      cmp);
  State state = {start, 0};
  int all_keys_found = (1 << nr_keys) - 1;
  pq.push({0, state});
  while (!pq.empty()) {
    auto [distance, cur_state] = pq.top();
    pq.pop();

    if (dp.count(cur_state) && distance >= dp[cur_state]) {
      continue;
    }

    dp[cur_state] = distance;

    if (cur_state.key_bitmask == all_keys_found) {
      return distance;
    }

    for (const auto& nb : path.at(cur_state.coord.label)) {
      if ((cur_state.key_bitmask & nb.state.key_bitmask) != nb.state.key_bitmask) {
        continue;
      }

      int new_key = cur_state.key_bitmask;
      new_key |= (1 << (nb.state.coord.label - 'a'));

      State new_state = {nb.state.coord, new_key};
      int new_distance = distance + nb.distance;

      if (!dp.count(new_state) || new_distance < dp[new_state]) {
        pq.push({new_distance, new_state});
      }
    }
  }
  return -1;
}

int get_key_path_length(const Path& path, const std::array<Coord, 4>& starts, int nr_keys) {
  std::unordered_map<MultiState, int, MultiStateHash> dp;
  auto cmp = [](const std::pair<int, MultiState>& a, const std::pair<int, MultiState>& b) {
    return a.first > b.first;
  };
  std::priority_queue<std::pair<int, MultiState>,
                      std::vector<std::pair<int, MultiState>>,
                      decltype(cmp)>
      pq(cmp);
  int all_keys_found = (1 << nr_keys) - 1;
  MultiState start_state = {starts, 0};
  pq.push({0, start_state});
  while (!pq.empty()) {
    auto [distance, cur_states] = pq.top();
    pq.pop();

    if (dp.count(cur_states) && distance > dp[cur_states]) {
      continue;
    }

    if (cur_states.key_bitmask == all_keys_found) {
      return distance;
    }
    int keys = cur_states.key_bitmask;

    for (std::size_t i = 0; i < cur_states.coords.size(); ++i) {
      Coord pos = cur_states[i];
      for (const auto& nb : path.at(pos.label)) {
        if ((keys & nb.state.key_bitmask) != nb.state.key_bitmask) {
          continue;
        }

        int new_keys = keys | (1 << (nb.state.coord.label - 'a'));
        MultiState new_states = cur_states;
        new_states.key_bitmask = new_keys;
        new_states[i] = nb.state.coord;

        int new_distance = distance + nb.distance;

        if (!dp.count(new_states) || new_distance < dp[new_states]) {
          dp[new_states] = new_distance;
          pq.push({new_distance, new_states});
        }
      }
    }
  }
  return -1;
}

#ifndef STANDALONE
// [[Rcpp::export]]
int get_key_path_length(const CharacterMatrix& maze) {
  std::vector<std::string> maze_vec;
  std::vector<Coord> pois;
  Coord start;
  int nrow = maze.nrow();
  int ncol = maze.ncol();
  for (int i = 0; i < nrow; ++i) {
    std::string row_str;
    for (int j = 0; j < ncol; ++j) {
      char cell = as<char>(maze(i, j));
      row_str += cell;
      if (cell == '@' || (cell >= 'a' && cell <= 'z')) {
        Coord poi = {j, i, cell};
        pois.push_back(poi);
        if (cell == '@') {
          start = poi;
        }
      }
    }
    maze_vec.push_back(row_str);
  }
  Path result = bfs(maze_vec, pois);
  return get_key_path_length(result, start, pois.size() - 1);
}

// [[Rcpp::export]]
int get_key_path_length_multi(const CharacterMatrix& maze) {
  std::vector<std::string> maze_vec;
  std::vector<Coord> pois;
  Coord orig_start = {0, 0};
  int nrow = maze.nrow();
  int ncol = maze.ncol();
  for (int i = 0; i < nrow; ++i) {
    std::string row_str;
    for (int j = 0; j < ncol; ++j) {
      char cell = as<char>(maze(i, j));
      row_str += cell;
      if (cell == '@' || (cell >= 'a' && cell <= 'z')) {
        Coord poi = {j, i, cell};
        if (cell == '@') {
          orig_start = poi;
        } else {
          pois.push_back(poi);
        }
      }
    }
    maze_vec.push_back(row_str);
  }
  int idx = 0;
  std::array<Coord, 4> starts;
  for (int j = -1; j < 2; ++j) {
    for (int i = -1; i < 2; ++i) {
      if (j == 0 || i == 0) {
        maze_vec[orig_start.y + j][orig_start.x + i] = '#';
      } else {
        char entrance = '1' + idx;
        maze_vec[orig_start.y + j][orig_start.x + i] = entrance;
        starts[idx] = {orig_start.x + i, orig_start.y + j, entrance};
        pois.push_back(starts[idx++]);
      }
    }
  }
  Path result = bfs(maze_vec, pois);
  return get_key_path_length(result, starts, pois.size() - 4);
}

#else
int main() {
  std::vector<std::string> maze_vec = {"#################",
                                       "#i.G..c...e..H.p#",
                                       "########.########",
                                       "#j.A..b...f..D.o#",
                                       "########@########",
                                       "#k.E..a...g..B.n#",
                                       "########.########",
                                       "#l.F..d...h..C.m#",
                                       "#################"};
  std::vector<Coord> pois;
  Coord start;
  int nrow = maze_vec.size();
  int ncol = maze_vec[0].size();
  for (int i = 0; i < nrow; ++i) {
    for (int j = 0; j < ncol; ++j) {
      char cell = maze_vec[i][j];
      if (cell == '@' || (cell >= 'a' && cell <= 'z')) {
        Coord poi = {j, i, cell};
        pois.push_back(poi);
        if (cell == '@') {
          start = poi;
        }
      }
    }
  }
  Path result = bfs(maze_vec, pois);
  COUT << get_key_path_length(result, start, pois.size() - 1) << std::endl;

  return 0;
}
#endif
