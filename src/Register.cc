#include "Register.hpp"

RegisterFile::RegisterFile() {
    registers.fill(0);
    
    // Initialize stack pointer (x2) to the end of memory space
    registers[2] = 0x7ffffff0;
    
    // Initialize global pointer (x3) to point to the middle of memory
    registers[3] = 0x10000000;
}

int32_t RegisterFile::read(uint32_t index) const {  // Changed return type to int32_t
    if (index == 0) return 0; // x0 is hardwired to 0
    return registers[index];
}

void RegisterFile::write(uint32_t index, int32_t value) {  // Changed parameter type to int32_t
    if (index != 0) { // Cannot write to x0
        registers[index] = value;
    }
}
