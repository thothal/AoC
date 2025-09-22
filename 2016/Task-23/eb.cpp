#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

bool is_register(const std::string& value) {
  return (value[0] >= 'a' && value[0] <= 'd');
}

int resolve(const std::string& value, std::map<char, int>& registers) {
  if (is_register(value))
    return registers[value[0]];
  else
    return std::stoi(value);
}

class Command {
  public:
    std::vector<std::string> instruction;

    Command(const std::string& s) { boost::split(instruction, s, boost::is_any_of(" ")); }

    void toggle() {
      if (instruction.size() == 2) {
        if (instruction[0] == "inc")
          instruction[0] = "dec";
        else
          instruction[0] = "inc";
      } else {
        if (instruction[0] == "jnz")
          instruction[0] = "cpy";
        else
          instruction[0] = "jnz";
      }
    }

    int execute(const size_t& current,
                std::map<char, int>& registers,
                std::vector<Command>& commands) const {
      int result(1);
      if (instruction[0] == "cpy") {
        if (is_register(instruction[2]))
          registers[instruction[2][0]] = resolve(instruction[1], registers);
      } else if (instruction[0] == "inc") {
        ++registers[instruction[1][0]];
      } else if (instruction[0] == "dec") {
        --registers[instruction[1][0]];
      } else if (instruction[0] == "jnz") {
        if (resolve(instruction[1], registers) != 0) {
          result = resolve(instruction[2], registers);
        }
      } else if (instruction[0] == "tgl") {
        int offset = resolve(instruction[1], registers);
        if (current + offset >= 0 && current + offset < commands.size())
          commands[current + offset].toggle();
      }
      return result;
    }
};

int main() {
  std::map<char, int> registers({{'a', 12}, {'b', 0}, {'c', 0}, {'d', 0}});
  std::ifstream input("input23");
  std::vector<Command> commands;
  std::string line;
  std::getline(input, line);
  while (input) {
    commands.emplace_back(line);
    std::getline(input, line);
  }

  int current = 0;
  int i = 0;
  while (current < commands.size()) {
    ++i;
    if (i % 10000000 == 0) {
      std::cout << "Iteration: " << i << std::endl;
    }
    current += commands[current].execute(current, registers, commands);
  }
  std::cout << registers['a'] << "\t in " << i << " runs\n";
}