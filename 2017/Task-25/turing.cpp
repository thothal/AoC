#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <array>
#include <list>
#include <numeric>
#include <unordered_map>

struct Instruction {
    int val;
    int dir;
    char state;
};

int turing(int n,
           char start,
           const std::unordered_map<char, std::pair<Instruction, Instruction>>& ops) {
  std::list<int> bits = {0};
  auto it = bits.begin();
  char state = start;

  auto move_right = [&]() {
    ++it;
    if (it == bits.end()) {
      bits.push_back(0);
      it = std::prev(bits.end());
    }
  };

  auto move_left = [&]() {
    if (it == bits.begin()) {
      bits.push_front(0);
    }
    --it;
  };

  for (int i = 0; i < n; ++i) {
    int bit = *it;
    const auto& op = ops.at(state);
    const Instruction& ins = (bit == 0) ? op.first : op.second;

    *it = ins.val;
    if (ins.dir == 1) {
      move_right();
    } else {
      move_left();
    }
    state = ins.state;
  }

  return std::accumulate(bits.begin(), bits.end(), 0);
}

#ifndef STANDALONE
Instruction make_instruction(const List& part) {
  int val = as<int>(part[0]);
  int dir = as<int>(part[1]);
  std::string state = as<std::string>(part[2]);
  char state_c = state[0];
  return Instruction {val, dir, state_c};
}

// [[Rcpp::export]]
int turing(int n, char start, const List& lops) {
  std::unordered_map<char, std::pair<Instruction, Instruction>> ops;
  CharacterVector names = lops.names();
  for (int i = 0; i < lops.size(); ++i) {
    char name_c = as<std::string>(names[i])[0];
    List inner = lops[i];
    Instruction i1 = make_instruction(inner[0]);
    Instruction i2 = make_instruction(inner[1]);
    ops[name_c] = {i1, i2};
  }
  return turing(n, start, ops);
}

#else
int main() {
  Instruction iA1 = {1, 1, 'B'};
  Instruction iA2 = {0, -1, 'B'};
  Instruction iB1 = {1, -1, 'A'};
  Instruction iB2 = {1, 1, 'A'};
  std::unordered_map<char, std::pair<Instruction, Instruction>> ops {{'A', {iA1, iA2}},
                                                                     {'B', {iB1, iB2}}};
  std::cout << turing(6, 'A', ops) << std::endl;
  return 0;
}
#endif
