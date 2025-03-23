#include "alu.hpp"

int32_t ALU::execute(Operation op, int32_t a, int32_t b, int32_t pc) {
    switch (op) {
        // R-type operations
        case Operation::ADD: return a + b;
        case Operation::SUB: return a - b;
        case Operation::SLL: return a << (b & 0x1F);
        case Operation::SLT: return (a < b) ? 1 : 0;
        case Operation::SLTU: return ((uint32_t)a < (uint32_t)b) ? 1 : 0;
        case Operation::XOR: return a ^ b;
        case Operation::SRL: return (uint32_t)a >> (b & 0x1F);
        case Operation::SRA: return a >> (b & 0x1F);
        case Operation::OR: return a | b;
        case Operation::AND: return a & b;
        
        // I-type operations
        case Operation::ADDI: return a + b;
        
        // Memory operations (address calculation)
        case Operation::LB:
        case Operation::LH:
        case Operation::LW:
        case Operation::LBU:
        case Operation::LHU:
        case Operation::SB:
        case Operation::SH:
        case Operation::SW:
            return a + b; // Calculate memory address
            
        // Branch comparison operations
        case Operation::BEQ: return (a == b) ? 1 : 0;
        case Operation::BNE: return (a != b) ? 1 : 0;
        case Operation::BLT: return (a < b) ? 1 : 0;
        case Operation::BGE: return (a >= b) ? 1 : 0;
        case Operation::BLTU: return ((uint32_t)a < (uint32_t)b) ? 1 : 0;
        case Operation::BGEU: return ((uint32_t)a >= (uint32_t)b) ? 1 : 0;
        
        // U-type operations
        case Operation::LUI: return b;
        case Operation::AUIPC: return pc + b;
        
        // Jump operations
        case Operation::JAL: return pc + 4; // Store return address (PC+4)
        case Operation::JALR: return pc + 4; // Store return address (PC+4)
        
        default: return 0;
    }
}
