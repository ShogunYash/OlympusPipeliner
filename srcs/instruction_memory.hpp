#ifndef INSTRUCTION_MEMORY_HPP
#define INSTRUCTION_MEMORY_HPP

#include <vector>
#include <cstdint>
#include <unordered_map>

class InstructionMemory {
private:
    std::unordered_map<uint32_t, uint32_t> instructions;

public:
    InstructionMemory();
    
    // Read an instruction from the given address
    uint32_t readInstruction(uint32_t address) const;
    
    // Load program into instruction memory
    void loadProgram(const std::vector<uint32_t>& program, uint32_t start_address = 0);
    
    // Returns true if the instruction memory has an instruction at the given address
    bool hasInstructionAt(uint32_t address) const;
};

#endif // INSTRUCTION_MEMORY_HPP
