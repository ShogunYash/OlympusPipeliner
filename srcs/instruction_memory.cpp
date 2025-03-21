#include "instruction_memory.hpp"

InstructionMemory::InstructionMemory() {}

uint32_t InstructionMemory::readInstruction(uint32_t address) const {
    auto it = instructions.find(address);
    if (it != instructions.end()) {
        return it->second;
    }
    return 0; // Return 0 for addresses with no instruction
}

void InstructionMemory::loadProgram(const std::vector<uint32_t>& program, uint32_t start_address) {
    uint32_t address = start_address;
    for (uint32_t instr : program) {
        instructions[address] = instr;
        address += 4; // Word-aligned addresses
    }
}

bool InstructionMemory::hasInstructionAt(uint32_t address) const {
    return instructions.find(address) != instructions.end();
}
