#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#endif

#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

constexpr unsigned int MAX_COST = std::numeric_limits<unsigned int>::max();

class State {
  public:
    State(const std::vector<std::vector<char>>&);

    std::string get_map_id() const;
    std::vector<unsigned int> get_hallbots() const;
    std::unordered_map<unsigned int, std::vector<unsigned int>> get_homebots() const;

    unsigned int move_home(unsigned int);
    unsigned int move_out(unsigned int, unsigned int);
    bool is_done() const;

  private:
    const unsigned int room_size;

    const std::array<char, 4> room_labels = {'A', 'B', 'C', 'D'};
    const std::array<unsigned int, 4> moving_costs = {1, 10, 100, 1000};
    const std::array<unsigned int, 4> room_pos = {2, 4, 6, 8};

    std::array<std::stack<char>, 4> rooms;
    std::array<char, 11> hallway;

    bool validate_layout(const std::vector<std::vector<char>>&) const;
    bool can_get_home(unsigned int) const;
    bool can_move(unsigned int, unsigned int) const;

    unsigned int get_bot_idx(char) const;
    unsigned int get_cost(unsigned int, unsigned int, unsigned int) const;

    friend std::ostream& operator<<(std::ostream&, const State&);
};

State::State(const std::vector<std::vector<char>>& initial_layout)
    : room_size(initial_layout[0].size())
    , rooms()
    , hallway() {
  if (!validate_layout(initial_layout)) {
    throw std::runtime_error("all elements of 'initial_input' must have the same length");
  }
  hallway.fill('.');
  size_t i = 0;
  for (const auto& l : initial_layout) {
    std::stack<char> room;
    std::vector<char> room_layout = l;
    for (auto j = room_layout.end() - 1; j >= room_layout.begin(); --j) {
      room.push(*j);
    }
    rooms[i++] = room;
  }
};

std::string State::get_map_id() const {
  std::ostringstream key_stream;
  std::string key;
  for (const auto s : hallway) {
    key_stream << s;
  }
  for (auto room : rooms) {
    while (room.size() != room_size) {
      room.push('.');
    }
    while (!room.empty()) {
      key_stream << room.top();
      room.pop();
    }
  }
  return key_stream.str();
}

std::vector<unsigned int> State::get_hallbots() const {
  std::vector<unsigned int> bot_idx;
  for (size_t i = 0; i < hallway.size(); ++i) {
    if (hallway[i] != '.') {
      if (can_get_home(i)) {
        bot_idx.push_back(i);
      }
    }
  }
  return bot_idx;
}

std::unordered_map<unsigned int, std::vector<unsigned int>> State::get_homebots() const {
  std::unordered_map<unsigned int, std::vector<unsigned int>> bot_idx;
  for (size_t i = 0; i < rooms.size(); ++i) {
    std::stack<char> room = rooms[i];
    std::vector<unsigned int> free_spaces;
    while (!room.empty()) {
      if (room.top() == room_labels[i]) {
        room.pop();
      } else {
        for (size_t j = 0; j < hallway.size(); ++j) {
          if (can_move(j, i) &&
              std::find(std::begin(room_pos), std::end(room_pos), j) == std::end(room_pos)) {
            free_spaces.push_back(j);
          }
        }
        if (!free_spaces.empty()) {
          bot_idx[i] = free_spaces;
        }
        break;
      }
    }
  }
  return bot_idx;
}

bool State::validate_layout(const std::vector<std::vector<char>>& initial_layout) const {
  bool result = true;
  for (const auto& l : initial_layout) {
    result = result && (l.size() == room_size);
  }
  return result;
}

bool State::can_get_home(unsigned int hall_pos) const {
  char bot = hallway[hall_pos];
  unsigned int home_idx = get_bot_idx(bot);
  // need an offset b/c we do not want to test the starting field
  int offset = (hall_pos < room_pos[home_idx]) ? 1 : -1;
  std::stack<char> room = rooms[home_idx];
  bool result = can_move(hall_pos + offset, home_idx) && room.size() != room_size;
  while (!room.empty() && result) {
    result = result && (room.top() == bot);
    room.pop();
  }
  return result;
}

bool State::can_move(unsigned int hall_pos, unsigned int room_idx) const {
  bool result = true;
  int step = hall_pos < room_pos[room_idx] ? 1 : -1;
  for (size_t i = hall_pos; i != room_pos[room_idx]; i += step) {
    result = result && (hallway[i] == '.');
    if (!result) {
      break;
    }
  }
  return result;
}

bool State::is_done() const {
  bool result;
  for (size_t i = 0; i < rooms.size(); i++) {
    std::stack<char> room = rooms[i];
    result = room.size() == room_size;
    while (!room.empty() && result) {
      result = result && (room.top() == room_labels[i]);
      room.pop();
    }
    if (!result) {
      break;
    }
  }
  return result;
}

unsigned int State::get_bot_idx(char bot) const {
  /* bot -'A' maps A => 0, B => 1, C => 2 and D => 3*/
  return bot - 'A';
}

unsigned int State::get_cost(unsigned int hall_pos,
                             unsigned int room_idx,
                             unsigned int bot_idx) const {
  unsigned int distance = abs(static_cast<int>(hall_pos) -
                              static_cast<int>(room_pos[room_idx])) + // horizontal distance
      (room_size - rooms[room_idx].size()) + // vertical distance
      // if we move out we need to count the last step
      (hallway[hall_pos] == '.' ? 1 : 0);
  return distance * moving_costs[bot_idx];
}

unsigned int State::move_home(unsigned int hall_pos) {
  char bot = hallway[hall_pos];
  unsigned int bot_idx = get_bot_idx(bot);
  unsigned int moving_costs = get_cost(hall_pos, bot_idx, bot_idx);
  hallway[hall_pos] = '.';
  rooms[bot_idx].push(bot);
  return moving_costs;
}

unsigned int State::move_out(unsigned int room_idx, unsigned int hall_pos) {
  char bot = rooms[room_idx].top();
  unsigned int moving_costs = get_cost(hall_pos, room_idx, get_bot_idx(bot));
  rooms[room_idx].pop();
  hallway[hall_pos] = bot;
  return moving_costs;
}

std::ostream& operator<<(std::ostream& stream, const State& x) {
  size_t col, row, n_col, n_row;
  std::fill_n(std::ostream_iterator<char>(stream), x.hallway.size() + 2, '#');
  stream << std::endl << '#';
  for (const auto& s : x.hallway) {
    stream << s;
  }
  stream << '#' << std::endl;
  n_col = x.rooms.size();
  n_row = x.room_size;
  char rooms_str[n_row][n_col];
  for (col = 0; col < n_col; col++) {
    std::stack<char> room = x.rooms[col];
    row = n_row - room.size();
    for (size_t i = 0; i < row; i++) {
      rooms_str[i][col] = '.';
    }
    while (!room.empty()) {
      rooms_str[row++][col] = room.top();
      room.pop();
    }
  }
  for (row = 0; row < n_row; row++) {
    char filler = row == 0 ? '#' : ' ';
    std::fill_n(std::ostream_iterator<char>(stream), 2, filler);
    stream << '#';
    for (col = 0; col < n_col; col++) {
      stream << rooms_str[row][col] << '#';
    }
    std::fill_n(std::ostream_iterator<char>(stream), 2, filler);
    stream << std::endl;
  }
  stream << "  ";
  std::fill_n(std::ostream_iterator<char>(stream), x.hallway.size() - 2, '#');
  stream << "  " << std::endl;
  return stream;
}

unsigned int solve(State state) {
  static std::unordered_map<std::string, unsigned int> hash;
  unsigned int costs, moving_costs, branch_costs;
  std::string id = state.get_map_id();
  if (state.is_done()) {
    return 0;
  }
  if (hash.find(id) != hash.end()) {
    return hash[id];
  }
  for (const auto hall_pos : state.get_hallbots()) {
    moving_costs = state.move_home(hall_pos);
    costs = solve(state);
    if (costs <= MAX_COST - moving_costs) {
      return moving_costs + costs;
    } else {
      return MAX_COST;
    }
  }
  branch_costs = MAX_COST;
  for (const auto& [home_pos, cand_pos] : state.get_homebots()) {
    for (const auto hall_pos : cand_pos) {
      State new_state(state);
      moving_costs = new_state.move_out(home_pos, hall_pos);
      costs = solve(new_state);
      if (costs <= MAX_COST - moving_costs) {
        branch_costs = std::min(branch_costs, costs + moving_costs);
      }
    }
  }
  costs = branch_costs;
  hash[id] = costs;
  return costs;
}

#ifndef STANDALONE
// [[Rcpp::export]]
unsigned int bring_bots_home(const List& initial_layout) {
  std::vector<std::vector<char>> layout;
  for (const auto& l : initial_layout) {
    CharacterVector room_layout = l;
    std::vector<char> room;
    for (auto j = room_layout.begin(); j < room_layout.end(); ++j) {
      room.push_back(as<char>(*j));
    }
    layout.push_back(room);
    room.clear();
  }
  State state(layout);
  return solve(state);
}
#else
int main() {
  std::vector<std::vector<char>> initial_layout = {{'B', 'A'}, {'C', 'D'}, {'B', 'C'}, {'D', 'A'}};
  State state(initial_layout);
  std::cout << "Least amount of Energy:" << solve(state) << std::endl;
  return 0;
}
#endif