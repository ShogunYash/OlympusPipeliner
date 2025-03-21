#include "instruction.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

Instruction::Instruction(uint32_t instruction) : raw_instruction(instruction) {
    decode();
}

void Instruction::decode() {
    // Extract opcode (bits 0-6)
    uint8_t opcode = raw_instruction & 0x7F;
    
    // Extract other fields
    rd = (raw_instruction >> 7) & 0x1F;     // bits 7-11
    uint8_t funct3 = (raw_instruction >> 12) & 0x7; // bits 12-14
    rs1 = (raw_instruction >> 15) & 0x1F;    // bits 15-19
    rs2 = (raw_instruction >> 20) & 0x1F;    // bits 20-24
    uint8_t funct7 = (raw_instruction >> 25) & 0x7F; // bits 25-31
    
    // Default initialization
    type = InstructionType::UNKNOWN;
    op = Operation::UNKNOWN;
    imm = 0;
    assembly = "unknown";
    
    // Decode based on opcode
    switch (opcode) {
        case 0x33: { // R-type
            type = InstructionType::R_TYPE;
            switch (funct3) {
                case 0x0:
                    if (funct7 == 0x00) { op = Operation::ADD; assembly = "add x" + std::to_string(rd) + " x" + std::to_string(rs1) + " x" + std::to_string(rs2); }
                    else if (funct7 == 0x20) { op = Operation::SUB; assembly = "sub x" + std::to_string(rd) + " x" + std::to_string(rs1) + " x" + std::to_string(rs2); }
                    break;
                case 0x1: op = Operation::SLL; assembly = "sll x" + std::to_string(rd) + " x" + std::to_string(rs1) + " x" + std::to_string(rs2); break;
                case 0x4: op = Operation::XOR; assembly = "xor x" + std::to_string(rd) + " x" + std::to_string(rs1) + " x" + std::to_string(rs2); break;
                case 0x6: op = Operation::OR; assembly = "or x" + std::to_string(rd) + " x" + std::to_string(rs1) + " x" + std::to_string(rs2); break;
                case 0x7: op = Operation::AND; assembly = "and x" + std::to_string(rd) + " x" + std::to_string(rs1) + " x" + std::to_string(rs2); break;
                // Add more R-type instructions as needed
            }
            break;
        }
        case 0x13: { // I-type (immediate)
            type = InstructionType::I_TYPE;
            // Sign extend immediate
            imm = ((int32_t)(raw_instruction & 0xFFF00000)) >> 20;
            
            switch (funct3) {
                case 0x0: op = Operation::ADDI; assembly = "addi x" + std::to_string(rd) + " x" + std::to_string(rs1) + " " + std::to_string(imm); break;
                // Add more I-type instructions as needed
            }
            break;
        }
        case 0x3: { // I-type (loads)
            type = InstructionType::I_TYPE;
            // Sign extend immediate
            imm = ((int32_t)(raw_instruction & 0xFFF00000)) >> 20;
            
            switch (funct3) {
                case 0x0: op = Operation::LB; assembly = "lb x" + std::to_string(rd) + " " + std::to_string(imm) + "(x" + std::to_string(rs1) + ")"; break;
                // Add more load instructions as needed
            }
            break;
        }
        case 0x23: { // S-type
            type = InstructionType::S_TYPE;
            // Construct immediate from split fields
            uint32_t imm11_5 = (raw_instruction >> 25) & 0x7F;
            uint32_t imm4_0 = (raw_instruction >> 7) & 0x1F;
            imm = ((int32_t)((imm11_5 << 5) | imm4_0) << 20) >> 20; // Sign extend
            
            switch (funct3) {
                case 0x0: op = Operation::SB; assembly = "sb x" + std::to_string(rs2) + " " + std::to_string(imm) + "(x" + std::to_string(rs1) + ")"; break;
                // Add more store instructions as needed
            }
            break;
        }
        case 0x63: { // B-type
            type = InstructionType::B_TYPE;
            // Construct immediate for branch (×2 for byte addressing)
            uint32_t imm12 = (raw_instruction >> 31) & 0x1;
            uint32_t imm11 = (raw_instruction >> 7) & 0x1;
            uint32_t imm10_5 = (raw_instruction >> 25) & 0x3F;
            uint32_t imm4_1 = (raw_instruction >> 8) & 0xF;
            imm = ((imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1));
            // Sign extend
            if (imm12) imm |= 0xFFFFE000;
            
            switch (funct3) {
                case 0x0: op = Operation::BEQ; assembly = "beq x" + std::to_string(rs1) + " x" + std::to_string(rs2) + " " + std::to_string(imm); break;
                // Add more branch instructions as needed
            }
            break;
        }
        case 0x6F: { // J-type (jal)
            type = InstructionType::J_TYPE;
            // Construct immediate for jump (×2 for byte addressing)
            uint32_t imm20 = (raw_instruction >> 31) & 0x1;
            uint32_t imm19_12 = (raw_instruction >> 12) & 0xFF;
            uint32_t imm11 = (raw_instruction >> 20) & 0x1;
            uint32_t imm10_1 = (raw_instruction >> 21) & 0x3FF;
            imm = ((imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1));
            // Sign extend
            if (imm20) imm |= 0xFFE00000;
            
            op = Operation::JAL;
            assembly = "jal x" + std::to_string(rd) + " " + std::to_string(imm);
            break;
        }
        case 0x67: { // I-type (jalr)
            type = InstructionType::I_TYPE;
            // Sign extend immediate
            imm = ((int32_t)(raw_instruction & 0xFFF00000)) >> 20;
            
            if (funct3 == 0x0) {
                op = Operation::JALR;
                assembly = "jalr x" + std::to_string(rd) + " x" + std::to_string(rs1) + " " + std::to_string(imm);
            }
            break;
        }
        default:
            if (raw_instruction == 0) {
                op = Operation::NOP;
                assembly = "nop";
            }
            break;
    }
}

std::string Instruction::getAssembly() const {
    // Add special case for ecall
    if (raw_instruction == 0x00000073) {
        return "ecall";
    }
    
    return assembly;
}

bool Instruction::useRs1() const {
    return op != Operation::LUI && op != Operation::AUIPC && op != Operation::JAL && op != Operation::NOP;
}

bool Instruction::useRs2() const {
    return type == InstructionType::R_TYPE || type == InstructionType::S_TYPE || type == InstructionType::B_TYPE;
}

bool Instruction::useRd() const {
    return type != InstructionType::S_TYPE && type != InstructionType::B_TYPE && op != Operation::NOP;
}

bool Instruction::isLoad() const {
    return op == Operation::LB || op == Operation::LH || op == Operation::LW || 
           op == Operation::LBU || op == Operation::LHU;
}

bool Instruction::isStore() const {
    return op == Operation::SB || op == Operation::SH || op == Operation::SW;
}

bool Instruction::isBranch() const {
    return type == InstructionType::B_TYPE;
}

bool Instruction::isJump() const {
    return op == Operation::JAL || op == Operation::JALR;
}
