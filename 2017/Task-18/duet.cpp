#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <queue>

using namespace std;

bool execute(int &instr_num, vector<string> &instructions, vector<long long> &registers, queue<long long> &rcv, queue<long long> &snd, int &snd_cnt)
{
	if (instr_num >= instructions.size()) {
		return false;
	};

	string op, reg_id, operand_id;
	istringstream instruction(instructions[instr_num]);
	long long reg_a;
	long long reg_b;

	instruction >> op >> reg_id;

	if (op == "snd" || op == "rcv")
		operand_id = reg_id;
	else
		instruction >> operand_id;

	if (op == "jgz")
		reg_a = (reg_id[0] >= 'a' && reg_id[0] <= 'z') ? registers[reg_id[0] - 'a'] : stoi(reg_id);
	else
		reg_a = reg_id[0] - 'a';

	reg_b = (operand_id[0] >= 'a' && operand_id[0] <= 'z') ? registers[operand_id[0] - 'a'] : stoi(operand_id);

	if (op == "snd") {
		snd.push(reg_b);
		snd_cnt++;
	} else if (op == "set") {
		registers[reg_a] = reg_b;
	} else if (op == "add") {
		registers[reg_a] += reg_b;
	} else if (op == "mul") {
		registers[reg_a] *= reg_b;
	} else if (op == "mod") {
		registers[reg_a] %= reg_b;
	} else if (op == "rcv") {
		if (rcv.empty()) {
			return false;
		} else {
			registers[reg_a] = rcv.front();
			rcv.pop();
		}
	} else if (op == "jgz" && reg_a > 0) {
		instr_num += reg_b - 1;
	}

	instr_num++;

	return true;
}

int main(int argc, char const* argv[])
{
	vector<long long> prog_a_reg(26, 0);
	vector<long long> prog_b_reg(26, 0);
	vector<string> instructions;
	ifstream infile("input.txt");
	int instr_num_a = 0;
	int instr_num_b = 0;
	int snd_a_cnt = 0;
	int snd_b_cnt = 0;
	queue<long long> queue_a;
	queue<long long> queue_b;

	prog_a_reg['p' - 'a'] = 0;
	prog_b_reg['p' - 'a'] = 1;

	if (!infile.is_open()) {
		return 1;
	} else {
		string instr;
		while (getline(infile, instr))
			instructions.push_back(instr);
	}

	infile.close();

	while (true) {
		bool cont_exec_a = execute(instr_num_a, instructions, prog_a_reg, queue_a, queue_b, snd_a_cnt);
		bool cont_exec_b = execute(instr_num_b, instructions, prog_b_reg, queue_b, queue_a, snd_b_cnt);
		if (!cont_exec_a && !cont_exec_b) break;
	}

	cout << snd_b_cnt << endl;

	return 0;
}
