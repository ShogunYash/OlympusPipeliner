#include "Memory.hpp"

Memory::Memory() {}

uint8_t Memory::readByte(uint32_t address) const {
    auto it = memory.find(address);
    if (it != memory.end()) {
        return it->second;
    }
    return 0;
}

uint32_t Memory::readWord(uint32_t address) const {
    uint32_t word = 0;
    // Little-endian
    word |= static_cast<uint32_t>(readByte(address));
    word |= static_cast<uint32_t>(readByte(address + 1)) << 8;
    word |= static_cast<uint32_t>(readByte(address + 2)) << 16;
    word |= static_cast<uint32_t>(readByte(address + 3)) << 24;
    return word;
}

void Memory::writeByte(uint32_t address, uint8_t value) {
    memory[address] = value;
}

void Memory::writeWord(uint32_t address, uint32_t value) {
    writeByte(address, value & 0xFF);
    writeByte(address + 1, (value >> 8) & 0xFF);
    writeByte(address + 2, (value >> 16) & 0xFF);
    writeByte(address + 3, (value >> 24) & 0xFF);
}
