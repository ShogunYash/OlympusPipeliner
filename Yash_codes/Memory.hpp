#pragma once
#include <cstdint>
#include <unordered_map>

class Memory {
private:
    std::unordered_map<uint32_t, uint8_t> memory;

public:
    Memory();
    
    uint8_t readByte(uint32_t address) const;
    int16_t readHalfWord(uint32_t address) const;  // Returns 16-bit value (sign extended)
    int32_t readWord(uint32_t address) const;      // Returns 32-bit value (signed)
    
    void writeByte(uint32_t address, uint8_t value);
    void writeHalfWord(uint32_t address, int16_t value);  // Stores 16-bit value
    void writeWord(uint32_t address, int32_t value);      // Stores 32-bit value (signed)
};
