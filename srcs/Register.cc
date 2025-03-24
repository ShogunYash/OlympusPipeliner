#include "Register.hpp"

RegisterFile::RegisterFile() {
    registers.fill(0);
}

uint32_t RegisterFile::read(uint32_t index) const {
    if (index == 0) return 0; // x0 is hardwired to 0
    return registers[index];
}

void RegisterFile::write(uint32_t index, uint32_t value) {
    if (index != 0) { // Cannot write to x0
        registers[index] = value;
    }
}
