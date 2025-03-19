#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <cstdint>
#include <string>

// Define instruction types
enum class InstructionType {
    R_TYPE,  // Register-Register operations
    I_TYPE,  // Immediate operations
    S_TYPE,  // Store operations
    B_TYPE,  // Branch operations
    U_TYPE,  // Upper immediate operations
    J_TYPE,  // Jump operations
    UNKNOWN
};

// Supported operations
enum class Operation {
    // R-type
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
    // I-type
    ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI, LB, LH, LW, LBU, LHU,
    // S-type
    SB, SH, SW,
    // B-type
    BEQ, BNE, BLT, BGE, BLTU, BGEU,
    // U-type
    LUI, AUIPC,
    // J-type
    JAL, JALR,
    // Special
    NOP, UNKNOWN
};

class Instruction {
private:
    uint32_t raw_instruction; // 32-bit machine code
    InstructionType type;     // Instruction type
    Operation op;             // Operation
    
    // Decoded fields
    uint8_t rd;               // Destination register
    uint8_t rs1;              // Source register 1
    uint8_t rs2;              // Source register 2
    int32_t imm;              // Immediate value
    
    // Human-readable representation
    std::string assembly;     // Assembly representation
    
    // Helper methods for decoding
    void decode();
    
public:
    Instruction(uint32_t instruction);
    
    // Getters
    uint32_t getRawInstruction() const { return raw_instruction; }
    InstructionType getType() const { return type; }
    Operation getOperation() const { return op; }
    uint8_t getRd() const { return rd; }
    uint8_t getRs1() const { return rs1; }
    uint8_t getRs2() const { return rs2; }
    int32_t getImm() const { return imm; }
    std::string getAssembly() const { return assembly; }
    
    // Utility methods
    bool isNop() const { return op == Operation::NOP; }
    bool useRs1() const;
    bool useRs2() const;
    bool useRd() const;
    bool isLoad() const;
    bool isStore() const;
    bool isBranch() const;
    bool isJump() const;
};

#endif // INSTRUCTION_HPP
