#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#define PRINT_STREAM Rcpp::Rcout
#else
#include <iostream>
#define PRINT_STREAM std::cout
#endif

#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

constexpr int NUM_FLOORS = 4;

struct Move {
  public:
    std::pair<int, std::optional<int>> cargo;
    int target_floor;
};

class State : public std::enable_shared_from_this<State> {
  private:
    int current_floor; // 0–3
    int moves_made;
    std::shared_ptr<State> parent;
    std::vector<int> item_floors; // position of each element, microchips sit on even positions,
                                  // generators on odd
    std::vector<std::string> mineral_names;

    int get_num_materials() const { return mineral_names.size(); }

  public:
    State(const std::vector<std::vector<std::string>>&);
    int get_moves_made() const { return moves_made; }
    bool is_valid() const;
    bool is_done() const;

    int score() const;

    std::string hash() const;

    std::shared_ptr<State> move(int, std::optional<int>, int) const;
    std::shared_ptr<State> move(const Move&) const;

    std::vector<Move> get_valid_moves() const;

    std::vector<std::shared_ptr<State>> get_path() const;

    friend std::ostream& operator<<(std::ostream&, const State&);
};

struct StatePtrComparator {
  public:
    bool operator()(const std::shared_ptr<State>& a, const std::shared_ptr<State>& b) const {
      return a->score() > b->score(); // higher score = better
    }
};

State::State(const std::vector<std::vector<std::string>>& floor_plan)
    : current_floor(0)
    , moves_made(0) {
  int offset, idx;
  for (size_t floor = 0; floor < floor_plan.size(); ++floor) {
    for (const auto& item : floor_plan[floor]) {
      offset = (item.find("microchip") != std::string::npos) ? 1 : 0;
      // Extract first 2 letters letter and make first letter upper case
      std::string mineral = item.substr(0, 2);
      std::transform(mineral.begin(), mineral.begin() + 1, mineral.begin(), ::toupper);
      // Is the mineral already stored?
      auto it = std::find(mineral_names.begin(), mineral_names.end(), mineral);
      if (it != mineral_names.end()) {
        idx = std::distance(mineral_names.begin(), it);
      } else {
        mineral_names.push_back(mineral);
        item_floors.insert(item_floors.end(), {-1, -1});
        idx = mineral_names.size() - 1;
      }
      item_floors[idx * 2 + offset] = floor;
    }
  }
}

bool State::is_valid() const {
  int n = get_num_materials();
  for (int floor = 0; floor < NUM_FLOORS; ++floor) {
    std::vector<bool> chips(n, false);
    std::vector<bool> generators(n, false);

    for (int i = 0; i < n; ++i) {
      if (item_floors[i * 2 + 1] == floor)
        chips[i] = true;
      if (item_floors[i * 2] == floor)
        generators[i] = true;
    }

    for (int i = 0; i < n; ++i) {
      if (chips[i] && !generators[i]) {
        // we have a unconnected chip...
        for (int j = 0; j < n; ++j) {
          if (generators[j]) {
            // ...and there is at least one other generator => BOOM
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool State::is_done() const {
  return std::all_of(
      item_floors.begin(), item_floors.end(), [](int pos) { return pos == NUM_FLOORS - 1; });
}

int State::score() const {
  int score = 0;

  for (int floor : item_floors) {
    score += (NUM_FLOORS - 1 - floor);
  }
  return moves_made + score;
}

std::string State::hash() const {
  int n = get_num_materials();
  std::string hash = std::to_string(current_floor);
  // use canonical hash to exclude states which a mere permutation of pairs
  std::vector<std::pair<int, int>> pairs(n);
  for (int i = 0; i < n; ++i) {
    pairs[i] = {item_floors[i * 2], item_floors[i * 2 + 1]};
  }
  std::sort(pairs.begin(), pairs.end());
  for (const auto& p : pairs) {
    hash += "-" + std::to_string(p.first) + "-" + std::to_string(p.second);
  }
  return hash;
}

std::shared_ptr<State> State::move(int first, std::optional<int> second, int target_floor) const {
  assert(item_floors[first] == current_floor);
  assert(!second.has_value() || item_floors[second.value()] == current_floor);
  auto new_state = std::make_shared<State>(*this);
  new_state->current_floor = target_floor;
  new_state->item_floors[first] = target_floor;
  if (second.has_value()) {
    new_state->item_floors[second.value()] = target_floor;
  }
  new_state->moves_made += abs(target_floor - current_floor);
  // set parent to be able to later identify the moves
  new_state->parent = std::const_pointer_cast<State>(shared_from_this());
  return new_state;
}

std::shared_ptr<State> State::move(const Move& move) const {
  return this->move(move.cargo.first, move.cargo.second, move.target_floor);
}

std::vector<Move> State::get_valid_moves() const {
  std::vector<Move> all_moves, valid_moves;
  std::vector<int> on_floor;
  for (size_t i = 0; i < item_floors.size(); ++i) {
    if (item_floors[i] == current_floor) {
      on_floor.push_back(i);
    }
  }
  // Generate all possible moves (1 or 2 elements)
  for (size_t i = 0; i < on_floor.size(); ++i) {
    for (int dest_floor = 0; dest_floor < NUM_FLOORS; ++dest_floor) {
      if (dest_floor != current_floor) {
        all_moves.push_back(Move {{on_floor[i], std::nullopt}, dest_floor});
        for (size_t j = i + 1; j < on_floor.size(); ++j) {
          all_moves.push_back(Move {{on_floor[i], on_floor[j]}, dest_floor});
        }
      }
    }
  }
  // Keep only valid moves
  std::copy_if(all_moves.begin(),
               all_moves.end(),
               std::back_inserter(valid_moves),
               [this](const Move& move) {
                 int final_dest = move.target_floor;
                 int step = (final_dest > this->current_floor) ? 1 : -1;
                 for (int floor = this->current_floor + step; floor != final_dest + step;
                      floor += step) {
                   auto new_state = this->move(move.cargo.first, move.cargo.second, floor);
                   if (!new_state->is_valid()) {
                     return false;
                   }
                 }
                 return true;
               });

  return valid_moves;
}

std::vector<std::shared_ptr<State>> State::get_path() const {
  std::vector<std::shared_ptr<State>> path;
  auto state = std::const_pointer_cast<State>(shared_from_this());
  while (state) {
    path.push_back(state);
    state = state->parent;
  }
  std::reverse(path.begin(), path.end());
  return path;
}

std::ostream& operator<<(std::ostream& os, const State& state) {
  std::vector<std::vector<std::string>> floors(
      NUM_FLOORS, std::vector<std::string>(2 * state.get_num_materials(), "..."));
  for (size_t i = 0; i < state.item_floors.size(); ++i) {
    bool is_chip = (i % 2 == 1);
    std::string item = state.mineral_names[i / 2] + (is_chip ? "M" : "G");
    floors[state.item_floors[i]][i] = item;
  }
  for (int floor = floors.size() - 1; floor >= 0; --floor) {
    os << "F" << (floor + 1) << (state.current_floor == floor ? " [E] " : "     ");
    for (const auto& item : floors[floor]) {
      os << item << " ";
    }
    os << std::endl;
  }
  os << "Costs: " << state.moves_made << std::endl;
  return os;
}

int count_moves(std::vector<std::vector<std::string>> floors, bool verbose = false) {
  int iteration = 0;
  auto initial_state = std::make_shared<State>(floors);
  std::shared_ptr<State> best_state = nullptr;
  std::unordered_set<std::string> visited;
  std::priority_queue<std::shared_ptr<State>,
                      std::vector<std::shared_ptr<State>>,
                      StatePtrComparator>
      all_states;
  visited.insert(initial_state->hash());
  all_states.push(initial_state);
  while (!all_states.empty()) {
    if (++iteration % 1000 == 0) {
      if (verbose) {
        PRINT_STREAM << "Iteration " << iteration << ", queue size: " << all_states.size()
                     << ", visited: " << visited.size() << std::endl;
      }
    }
    auto state = all_states.top();
    all_states.pop();
    if (state->is_done()) {
      best_state = state;
      break;
    }
    std::vector<Move> valid_moves = state->get_valid_moves();
    for (const auto& move : valid_moves) {
      auto new_state = state->move(move);
      std::string h = new_state->hash();
      if (visited.find(h) == visited.end()) {
        visited.insert(h);
        all_states.push(new_state);
      }
    }
  }

  if (verbose) {
    PRINT_STREAM << "Iteration " << iteration << ", queue size: " << all_states.size()
                 << ", visited: " << visited.size() << std::endl;
    std::vector<std::shared_ptr<State>> path = best_state->get_path();
    for (const auto& s : path) {
      PRINT_STREAM << *s << std::endl;
    }
  }
  if (best_state) {
    return best_state->get_moves_made();
  } else {
    return -1;
  }
}

#ifndef STANDALONE
// [[Rcpp::export]]
int count_moves(const List& initial_setup, bool verbose = false) {
  std::vector<std::vector<std::string>> floors;
  for (int i = 0; i < initial_setup.size(); ++i) {
    floors.push_back(as<std::vector<std::string>>(initial_setup[i]));
  }
  return count_moves(floors, verbose);
}
#else
int main() {
  std::vector<std::vector<std::string>> floors = {
      {"hydrogen microchip", "lithium microchip"}, {"hydrogen generator"}, {"lithium generator"}};
  PRINT_STREAM << count_moves(floors, true) << std::endl;
  return 0;
}
#endif