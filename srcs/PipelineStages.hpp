#pragma once
#include <cstdint>
#include <string>

// Control signals
struct ControlSignals {
    bool regWrite;
    bool memRead;
    bool memWrite;
    bool memToReg;
    bool aluSrc;
    bool branch;
    bool jump;
    uint32_t aluOp;
};

// IF/ID Pipeline Register
struct IFIDRegister {
    int32_t pc;                   // Changed from uint32_t to int32_t
    uint32_t instruction;         // Raw machine code.
    std::string instructionString;
    bool isStalled;
    bool isEmpty;

    IFIDRegister() : pc(0), instruction(0), isStalled(false), isEmpty(true) {}
};

// ID/EX Pipeline Register
struct IDEXRegister {
    int32_t pc;                   // Changed from uint32_t to int32_t
    uint32_t instruction;         // Propagated raw machine code.
    int32_t readData1;
    int32_t readData2;
    int32_t imm;
    uint32_t rs1;
    uint32_t rs2;
    uint32_t rd;
    ControlSignals controls;
    std::string instructionString;
    bool isStalled;
    bool isEmpty;

    IDEXRegister() : pc(0), instruction(0), readData1(0), readData2(0), imm(0), rs1(0), rs2(0), rd(0),
                     isStalled(false), isEmpty(true) {}
};

// EX/MEM Pipeline Register
struct EXMEMRegister {
    int32_t pc;                   // Changed from uint32_t to int32_t
    uint32_t instruction;         // Propagated raw machine code.
    int32_t aluResult;
    int32_t readData2;
    uint32_t rd;
    ControlSignals controls;
    std::string instructionString;
    bool isStalled;
    bool isEmpty;

    EXMEMRegister() : pc(0), instruction(0), aluResult(0), readData2(0), rd(0), isStalled(false), isEmpty(true) {}
};

// MEM/WB Pipeline Register
struct MEMWBRegister {
    int32_t pc;                   // Changed from uint32_t to int32_t
    uint32_t instruction;         // Propagated raw machine code.
    int32_t aluResult;
    int32_t readData;
    uint32_t rd;
    ControlSignals controls;
    std::string instructionString;
    bool isStalled;
    bool isEmpty;

    MEMWBRegister() : pc(0), instruction(0), aluResult(0), readData(0), rd(0), isStalled(false), isEmpty(true) {}
};
