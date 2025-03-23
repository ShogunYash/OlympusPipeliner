#pragma once
#include <cstdint>
#include <unordered_map>

class Memory {
private:
    std::unordered_map<uint32_t, uint8_t> memory;

public:
    Memory();
    
    uint8_t readByte(uint32_t address) const;
    uint32_t readWord(uint32_t address) const;  // Returns 32-bit value
    
    void writeByte(uint32_t address, uint8_t value);
    void writeWord(uint32_t address, uint32_t value);  // Stores 32-bit value
};
