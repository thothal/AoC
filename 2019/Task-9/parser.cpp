#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <stdexcept>
#include <vector>

using Int = long long;

class IntcodeComputer {
  public:
    explicit IntcodeComputer(std::vector<Int> program)
        : memory(std::move(program)) { }

    void add_input(Int value) { input.push_back(value); }

    bool has_output() const { return !output.empty(); }

    Int get_output() {
      Int val = output.front();
      output.erase(output.begin());
      return val;
    }

    bool step();
    void run();

  private:
    enum class Mode { Position = 0, Immediate = 1, Relative = 2 };

    enum class OpCode {
      Add = 1,
      Mul = 2,
      Input = 3,
      Output = 4,
      JumpIfTrue = 5,
      JumpIfFalse = 6,
      LessThan = 7,
      Equals = 8,
      AdjustRelBase = 9,
      Halt = 99
    };

    std::vector<Int> memory;
    std::vector<Int> input;
    std::vector<Int> output;
    Int ip = 0;
    Int rel_base = 0;

    void ensure_size(size_t idx) {
      if (idx >= memory.size())
        memory.resize(idx + 1, 0);
    }

    Int get_value(Int param, Mode mode) const {
      switch (mode) {
      case Mode::Position:
        return (param >= 0 && static_cast<size_t>(param) < memory.size()) ? memory[param] : 0;
      case Mode::Immediate:
        return param;
      case Mode::Relative: {
        Int addr = rel_base + param;
        return (addr >= 0 && static_cast<size_t>(addr) < memory.size()) ? memory[addr] : 0;
      }
      default:
        throw std::runtime_error("unknown parameter mode");
      }
    }

    size_t get_address(Int param, Mode mode) const {
      switch (mode) {
      case Mode::Position:
        return static_cast<size_t>(param);
      case Mode::Relative:
        return static_cast<size_t>(rel_base + param);
      default:
        throw std::runtime_error("invalid write mode");
      }
    }

    static Mode decode_mode(Int instr) { return static_cast<Mode>(instr % 10); }

    static OpCode decode_opcode(Int instr) { return static_cast<OpCode>(instr % 100); }

    std::vector<Mode> get_parameter_modes(Int instr, int num_params) const {
      std::vector<Mode> modes;
      instr /= 100; // Remove opcode
      for (int i = 0; i < num_params; ++i) {
        modes.push_back(decode_mode(instr));
        instr /= 10;
      }
      return modes;
    }
};

bool IntcodeComputer::step() {
  Int instr = memory.at(ip);
  OpCode op = decode_opcode(instr);

  std::vector<Mode> modes;
  int num_params = 0;

  switch (op) {
  case OpCode::Add:
  case OpCode::Mul:
  case OpCode::LessThan:
  case OpCode::Equals:
    num_params = 3;
    break;
  case OpCode::JumpIfTrue:
  case OpCode::JumpIfFalse:
    num_params = 2;
    break;
  case OpCode::Input:
  case OpCode::Output:
  case OpCode::AdjustRelBase:
    num_params = 1;
    break;
  case OpCode::Halt:
    return false;
  default:
    throw std::runtime_error("unknown opcode");
  }

  modes = get_parameter_modes(instr, num_params);

  switch (op) {
  case OpCode::Add: {
    Int a = get_value(memory[ip + 1], modes[0]);
    Int b = get_value(memory[ip + 2], modes[1]);
    size_t dest = get_address(memory[ip + 3], modes[2]);
    ensure_size(dest);
    memory[dest] = a + b;
    ip += 4;
    break;
  }
  case OpCode::Mul: {
    Int a = get_value(memory[ip + 1], modes[0]);
    Int b = get_value(memory[ip + 2], modes[1]);
    size_t dest = get_address(memory[ip + 3], modes[2]);
    ensure_size(dest);
    memory[dest] = a * b;
    ip += 4;
    break;
  }
  case OpCode::Input: {
    if (input.empty())
      return false;
    size_t dest = get_address(memory[ip + 1], modes[0]);
    ensure_size(dest);
    memory[dest] = input.front();
    input.erase(input.begin());
    ip += 2;
    break;
  }
  case OpCode::Output: {
    Int val = get_value(memory[ip + 1], modes[0]);
    output.push_back(val);
    ip += 2;
    break;
  }
  case OpCode::JumpIfTrue: {
    Int a = get_value(memory[ip + 1], modes[0]);
    Int b = get_value(memory[ip + 2], modes[1]);
    ip = (a != 0) ? b : ip + 3;
    break;
  }
  case OpCode::JumpIfFalse: {
    Int a = get_value(memory[ip + 1], modes[0]);
    Int b = get_value(memory[ip + 2], modes[1]);
    ip = (a == 0) ? b : ip + 3;
    break;
  }
  case OpCode::LessThan: {
    Int a = get_value(memory[ip + 1], modes[0]);
    Int b = get_value(memory[ip + 2], modes[1]);
    size_t dest = get_address(memory[ip + 3], modes[2]);
    ensure_size(dest);
    memory[dest] = (a < b) ? 1 : 0;
    ip += 4;
    break;
  }
  case OpCode::Equals: {
    Int a = get_value(memory[ip + 1], modes[0]);
    Int b = get_value(memory[ip + 2], modes[1]);
    size_t dest = get_address(memory[ip + 3], modes[2]);
    ensure_size(dest);
    memory[dest] = (a == b) ? 1 : 0;
    ip += 4;
    break;
  }
  case OpCode::AdjustRelBase: {
    Int a = get_value(memory[ip + 1], modes[0]);
    rel_base += a;
    ip += 2;
    break;
  }
  case OpCode::Halt:
    return false;
  }

  return true;
}

void IntcodeComputer::run() {
  while (step()) {
    // runs until input block or HALT
  }
}

#ifndef STANDALONE
// [[Rcpp::export]]
IntegerVector run_intcode(IntegerVector program, IntegerVector input) {
  IntcodeComputer vm(as<std::vector<Int>>(program));
  for (Int val : input) {
    vm.add_input(val);
  }
  vm.run();
  std::vector<Int> output;
  while (vm.has_output()) {
    output.push_back(vm.get_output());
  }
  return wrap(output);
}
#else
int main() {
  std::vector<Int> program = {
      109, 1, 204, -1, 1001, 100, 1, 100, 1008, 100, 16, 101, 1006, 101, 0, 99};
  IntcodeComputer vm(program);
  vm.add_input(2);
  vm.run();
  while (vm.has_output()) {
    std::cout << vm.get_output() << " ";
  }
  std::cout << "\n";
  return 0;
}
#endif
