#pragma once
#include "Register.hpp"
#include "Memory.hpp"
#include "PipelineStages.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>

class NoForwardingProcessor {
private:
    uint32_t pc;
    RegisterFile registers;
    Memory dataMemory;
    std::vector<uint32_t> instructionMemory;
    std::vector<std::string> instructionStrings;
    
    // Pipeline registers
    IFIDRegister ifid;
    IDEXRegister idex;
    EXMEMRegister exmem;
    MEMWBRegister memwb;
    
    // Pipeline history for display
    std::unordered_map<std::string, std::vector<std::string>> pipelineHistory;
    std::unordered_map<std::string, uint32_t> instructionPCs;    // Map unique IDs to PCs
    std::unordered_map<std::string, std::string> instructionTexts; // Map unique IDs to instruction texts
    
    bool stall;
    
    // Register In-Use Array: true means the register is pending a write-back.
    bool regInUse[32];
    
    // Helper functions
    ControlSignals decodeControlSignals(uint32_t instruction);
    uint32_t executeALU(uint32_t a, uint32_t b, uint32_t aluOp);
    void updatePipelineHistory(int cycle);
    std::string getInstructionString(uint32_t instruction);
    uint32_t extractImmediate(uint32_t instruction, uint32_t opcode);

public:
    NoForwardingProcessor();
    bool loadInstructions(const std::string& filename);
    void run(int cycles);
    void printPipelineDiagram(int cycles);
    size_t getInstructionCount() const { return instructionTexts.size(); }
};