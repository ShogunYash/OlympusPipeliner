#include "memory.hpp"

Memory::Memory() {}

uint8_t Memory::readByte(uint32_t address) {
    return memory[address];
}

uint16_t Memory::readHalfWord(uint32_t address) {
    return static_cast<uint16_t>(memory[address]) |
           (static_cast<uint16_t>(memory[address + 1]) << 8);
}

uint32_t Memory::readWord(uint32_t address) {
    return static_cast<uint32_t>(memory[address]) |
           (static_cast<uint32_t>(memory[address + 1]) << 8) |
           (static_cast<uint32_t>(memory[address + 2]) << 16) |
           (static_cast<uint32_t>(memory[address + 3]) << 24);
}

void Memory::writeByte(uint32_t address, uint8_t value) {
    memory[address] = value;
}

void Memory::writeHalfWord(uint32_t address, uint16_t value) {
    memory[address] = value & 0xFF;
    memory[address + 1] = (value >> 8) & 0xFF;
}

void Memory::writeWord(uint32_t address, uint32_t value) {
    memory[address] = value & 0xFF;
    memory[address + 1] = (value >> 8) & 0xFF;
    memory[address + 2] = (value >> 16) & 0xFF;
    memory[address + 3] = (value >> 24) & 0xFF;
}

void Memory::loadProgram(const std::vector<uint32_t>& instructions, uint32_t start_address) {
    uint32_t address = start_address;
    for (uint32_t instr : instructions) {
        writeWord(address, instr);
        address += 4;
    }
}
