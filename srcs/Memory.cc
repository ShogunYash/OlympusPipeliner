#include "Memory.hpp"

Memory::Memory() {}

uint8_t Memory::readByte(uint32_t address) const {
    auto it = memory.find(address);
    if (it != memory.end()) {
        return it->second;
    }
    return 0;
}

int16_t Memory::readHalfWord(uint32_t address) const {
    uint16_t halfWord = 0;
    // Little-endian
    halfWord |= static_cast<uint16_t>(readByte(address));
    halfWord |= static_cast<uint16_t>(readByte(address + 1)) << 8;
    
    // Sign extend if the MSB is set
    if (halfWord & 0x8000) {
        return static_cast<int16_t>(halfWord | 0xFFFF0000);
    }
    return static_cast<int16_t>(halfWord);
}

int32_t Memory::readWord(uint32_t address) const {
    uint32_t word = 0;
    // Little-endian
    word |= static_cast<uint32_t>(readByte(address));
    word |= static_cast<uint32_t>(readByte(address + 1)) << 8;
    word |= static_cast<uint32_t>(readByte(address + 2)) << 16;
    word |= static_cast<uint32_t>(readByte(address + 3)) << 24;
    
    return static_cast<int32_t>(word);
}

void Memory::writeByte(uint32_t address, uint8_t value) {
    memory[address] = value;
}

void Memory::writeHalfWord(uint32_t address, int16_t value) {
    // Little-endian
    writeByte(address, value & 0xFF);
    writeByte(address + 1, (value >> 8) & 0xFF);
}

void Memory::writeWord(uint32_t address, int32_t value) {
    // Little-endian
    writeByte(address, value & 0xFF);
    writeByte(address + 1, (value >> 8) & 0xFF);
    writeByte(address + 2, (value >> 16) & 0xFF);
    writeByte(address + 3, (value >> 24) & 0xFF);
}
