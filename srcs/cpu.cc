#include <iostream>
#include <string>
#include <vector>
#include "register.cc"
using namepsace std;

struct Memory {
	vector<int> data;
};

struct IF {
	Instr memory;
};

struct RegisterFile {
	vector<Register> registers;
};

struct ALU {
	int result;
};

struct
struct CPU {
	int cycles;
	Instr instr;
	vector<Register> registers;
	Memory memory;
	int PC;
};

