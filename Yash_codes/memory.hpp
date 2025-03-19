#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>

class Memory {
private:
    // Using a sparse memory representation
    std::unordered_map<uint32_t, uint8_t> memory;
    
public:
    Memory();
    
    // Memory access functions
    uint8_t readByte(uint32_t address);
    uint16_t readHalfWord(uint32_t address);
    uint32_t readWord(uint32_t address);
    
    void writeByte(uint32_t address, uint8_t value);
    void writeHalfWord(uint32_t address, uint16_t value);
    void writeWord(uint32_t address, uint32_t value);
    
    // Load program instructions
    void loadProgram(const std::vector<uint32_t>& instructions, uint32_t start_address = 0);
};

#endif // MEMORY_HPP
