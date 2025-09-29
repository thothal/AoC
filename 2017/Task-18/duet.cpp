#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <fstream>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

enum class OpCode { SND, RCV, SET, ADD, MUL, MOD, JGZ };
enum class ParamType { REG, VAL };

class Program;

class Parameter {
  public:
    Parameter();
    Parameter(const std::string&);

    long long operator()(const Program*) const;
    char operator()() const;

    friend std::ostream& operator<<(std::ostream&, const Parameter&);

  private:
    ParamType type;
    long long value;
    char register_name;
};

class Instruction {
  public:
    Instruction(const std::string&);
    bool operator()(Program*) const;

    friend std::ostream& operator<<(std::ostream&, const Instruction&);

  private:
    OpCode opcode;
    Parameter param1;
    std::optional<Parameter> param2;
    static const std::unordered_map<std::string, OpCode> opcode_map;

    static OpCode parse_opcode(const std::string& token);
};

class Program {
  public:
    Program(int, std::vector<Instruction>&, std::queue<long long>&, std::queue<long long>&);

    Program& operator++();

    void set_register(char reg, long long);
    long long get_register(char) const;

    void send(long long);
    std::optional<long long> receive();

    void jump(long long);

    bool run();

    long long get_send_count() const;

  private:
    long long line_count;
    long long snd_count;
    std::queue<long long>& rcv_queue;
    std::queue<long long>& snd_queue;
    std::vector<Instruction>& instructions;
    std::unordered_map<char, long long> registers;
};

Parameter::Parameter()
    : type(ParamType::VAL)
    , value(0)
    , register_name('\0') { }

Parameter::Parameter(const std::string& token)
    : type(ParamType::VAL)
    , value(0)
    , register_name('\0') {
  if (std::isdigit(token[0]) || token[0] == '-') {
    value = std::stoll(token);
    type = ParamType::VAL;
  } else {
    register_name = token[0];
    type = ParamType::REG;
  }
}

long long Parameter::operator()(const Program* program) const {
  if (type == ParamType::VAL) {
    return value;
  } else {
    return program->get_register(register_name);
  }
}

char Parameter::operator()() const {
  return register_name;
}

std::ostream& operator<<(std::ostream& os, const Parameter& param) {
  if (param.type == ParamType::VAL) {
    os << param.value;
  } else {
    os << param.register_name;
  }
  return os;
}

const std::unordered_map<std::string, OpCode> Instruction::opcode_map = {{"snd", OpCode::SND},
                                                                         {"rcv", OpCode::RCV},
                                                                         {"set", OpCode::SET},
                                                                         {"add", OpCode::ADD},
                                                                         {"mul", OpCode::MUL},
                                                                         {"mod", OpCode::MOD},
                                                                         {"jgz", OpCode::JGZ}};

Instruction::Instruction(const std::string& line) {
  std::istringstream iss(line);
  std::string op_str, p1_str, p2_str;
  iss >> op_str >> p1_str;

  opcode = parse_opcode(op_str);
  param1 = Parameter(p1_str);

  if (iss >> p2_str) {
    param2 = Parameter(p2_str);
  } else {
    param2 = std::nullopt;
  }
}

bool Instruction::operator()(Program* program) const {
  bool blocked = false;
  switch (opcode) {
  case OpCode::SND:
    program->send(param1(program));
    ++(*program);
    break;
  case OpCode::RCV: {
    auto val = program->receive();
    if (val.has_value()) {
      program->set_register(param1(), val.value());
      ++(*program);
    } else {
      blocked = true;
    }
    break;
  }
  case OpCode::SET:
    program->set_register(param1(), (*param2)(program));
    ++(*program);
    break;
  case OpCode::ADD:
    program->set_register(param1(), param1(program) + (*param2)(program));
    ++(*program);
    break;
  case OpCode::MUL: {
    program->set_register(param1(), param1(program) * (*param2)(program));
    ++(*program);
    break;
  }
  case OpCode::MOD:
    program->set_register(param1(), param1(program) % (*param2)(program));
    ++(*program);
    break;
  case OpCode::JGZ:
    if (param1(program) > 0) {
      program->jump((*param2)(program));
    } else {
      ++(*program);
    }
    break;
  }
  return !blocked;
}

OpCode Instruction::parse_opcode(const std::string& token) {
  auto it = opcode_map.find(token);
  return it->second;
}

std::ostream& operator<<(std::ostream& os, const Instruction& instr) {
  std::string key = "NOP";
  for (const auto& [k, v] : Instruction::opcode_map) {
    if (v == instr.opcode) {
      key = k;
      break;
    }
  }
  os << key << " " << instr.param1;
  if (instr.param2.has_value()) {
    os << " " << *instr.param2;
  }
  return os;
}

Program::Program(int id,
                 std::vector<Instruction>& instrs,
                 std::queue<long long>& rcv_q,
                 std::queue<long long>& snd_q)
    : line_count(0)
    , snd_count(0)
    , rcv_queue(rcv_q)
    , snd_queue(snd_q)
    , instructions(instrs) {
  registers = {{'a', 0}, {'b', 0}, {'f', 0}, {'i', 0}, {'p', id}};
}

Program& Program::operator++() {
  ++line_count;
  return *this;
}

void Program::set_register(char reg, long long value) {
  registers[reg] = value;
}

long long Program::get_register(char reg) const {
  auto it = registers.find(reg);
  if (it != registers.end()) {
    return it->second;
  } else {
    return 0;
  }
}

void Program::send(long long value) {
  snd_queue.push(value);
  snd_count++;
}

std::optional<long long> Program::receive() {
  if (rcv_queue.empty()) {
    return std::nullopt;
  } else {
    long long value = rcv_queue.front();
    rcv_queue.pop();
    return value;
  }
}

void Program::jump(long long offset) {
  line_count += offset;
}

long long Program::get_send_count() const {
  return snd_count;
}

bool Program::run() {
  bool done = line_count < 0 || line_count >= instructions.size();
  bool blocked = false;
  bool has_run = false;
  while (!done && !blocked) {
    Instruction& instr = instructions[line_count];
    bool success = instr(this);
    blocked = !success;
    has_run = has_run || success;
    done = line_count < 0 || line_count >= instructions.size();
  }
  return has_run;
}

long long get_send_count(const std::vector<std::string>& instruction_lines) {
  std::vector<Instruction> instructions;
  for (const auto& line : instruction_lines) {
    instructions.emplace_back(line);
  }

  std::queue<long long> queue_0_to_1;
  std::queue<long long> queue_1_to_0;

  Program p0(0, instructions, queue_1_to_0, queue_0_to_1);
  Program p1(1, instructions, queue_0_to_1, queue_1_to_0);

  bool prog0_ran = true;
  bool prog1_ran = true;

  while (prog0_ran || prog1_ran) {
    prog0_ran = p0.run();
    prog1_ran = p1.run();
  }

  return p1.get_send_count();
}

#ifndef STANDALONE
// [[Rcpp::export]]
long long get_send_count(const CharacterVector& instruction_lines) {
  return get_send_count(as<std::vector<std::string>>(instruction_lines));
}

#else
int main() {
  // program 1 sends 6 messages
  std::vector<std::string> instructions = {"add p 1",
                                           "mod p 2",
                                           "mul p 5",
                                           "snd p",
                                           "rcv a",
                                           "jgz a 3",
                                           "rcv b",
                                           "jgz 1 -1",
                                           "snd a",
                                           "add a -1",
                                           "jgz a -2",
                                           "rcv b",
                                           "jgz 1 -2"};
  std::cout << get_send_count(instructions) << std::endl;
}
#endif
