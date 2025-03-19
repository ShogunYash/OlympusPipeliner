#include "register_file.hpp"
#include <iostream>
#include <iomanip>

RegisterFile::RegisterFile() {
    registers.fill(0);
}

int32_t RegisterFile::read(uint8_t reg) const {
    if (reg >= 32) {
        std::cerr << "Error: Invalid register number: " << (int)reg << std::endl;
        return 0;
    }
    return registers[reg];
}

void RegisterFile::write(uint8_t reg, int32_t value) {
    if (reg >= 32) {
        std::cerr << "Error: Invalid register number: " << (int)reg << std::endl;
        return;
    }
    // x0 is hardwired to 0
    if (reg != 0) {
        registers[reg] = value;
    }
}

void RegisterFile::dump() const {
    for (int i = 0; i < 32; i++) {
        std::cout << "x" << i << ": 0x" << std::hex << std::setw(8) 
                  << std::setfill('0') << registers[i] << std::dec << std::endl;
    }
}
