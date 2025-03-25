#pragma once
#include "Register.hpp"
#include "Memory.hpp"
#include "PipelineStages.hpp"  // if you still use your old pipeline register structs
#include <string>
#include <vector>
#include <cstdlib>     // for malloc/free
#include <cstring>     // for memset

// Enumeration for pipeline stages.
// We use STALL to indicate either a stall or an empty cell.
enum PipelineStage {
    SPACE=0,  // Empty cell in the pipeline diagram
    STALL,
    SLASH, 
    IF,
    ID,
    EX,
    MEM,
    WB
};

// Helper function to convert enum value to printable string.
inline const char* stageToString(PipelineStage stage) {
    switch (stage) {
        case IF:   return "IF";
        case ID:   return "ID";
        case EX:   return "EX";
        case MEM:  return "ME";
        case WB:   return "WB";
        case SLASH: return "/";
        case STALL: return "- ";
        default:   return "  ";
    }
}

class NoForwardingProcessor {
private:
    int32_t pc;  // Changed to signed 32-bit
    RegisterFile registers;
    Memory dataMemory;
    std::vector<uint32_t> instructionMemory;
    std::vector<std::string> instructionStrings;
    
    // Pipeline registers (structs defined in PipelineStages.hpp)
    IFIDRegister ifid;
    IDEXRegister idex;
    EXMEMRegister exmem;
    MEMWBRegister memwb;
    
    // Rows correspond to instructions (in program order) and columns to cycle numbers.
    // Implement new type of pipeline matrix with 3D array to store stages of same instruction in same cycle with vector of stages
    std::vector<std::vector<std::vector<PipelineStage>>> pipelineMatrix3D;
    int matrixRows;   // equal to number of instructions loaded
    int matrixCols;   // equal to the number of cycles (set when run() is called)
    
    bool stall;
    
    // Advanced register usage tracking: vector of vectors to track which instruction uses each register
    // First dimension is register number (0-31), second dimension is variable-length list of instruction IDs
    std::vector<std::vector<bool>> regUsageTracker;
    
    // Helper functions
    ControlSignals decodeControlSignals(uint32_t instruction);
    int32_t executeALU(int32_t a, int32_t b, uint32_t aluOp);  // Changed to signed 32-bit
    int32_t extractImmediate(uint32_t instruction, uint32_t opcode);  // Changed to signed 32-bit
    
    // Branch and jump related functions
    bool evaluateBranchCondition(int32_t rs1Value, int32_t rs2Value, uint32_t funct3);
    
    // Updated to work in ID stage, using register values directly
    bool handleBranchAndJump(uint32_t opcode, uint32_t instruction, int32_t rs1Value, 
                            int32_t imm, int32_t pc, int32_t rs2Value, int32_t& branchTarget);
    
    // Helper to record a stage in the pipeline matrix.
    // 'instrIndex' is the row index (the instruction’s program order index)
    // 'cycle' is the current cycle.
    void recordStage(int instrIndex, int cycle, PipelineStage stage);
    
    // Helper: returns the index of the given pc in that stage
    int getInstructionIndex(int32_t index) const;
    
    // New helper function to check if a register is used by a specific instruction
    bool isRegisterUsedBy(uint32_t regNum) const;
    
    // New helper function to add tracking of register usage
    void addRegisterUsage(uint32_t regNum);
    
    // New helper function to clear register usage when instruction completes
    void clearRegisterUsage(uint32_t regNum);

public:
    NoForwardingProcessor();
    ~NoForwardingProcessor();  // Destructor to free memory
    bool loadInstructions(const std::string& filename);
    void run(int cycles);
    void printPipelineDiagram(std::string& InputFile); // Print pipeline diagram to file
    size_t getInstructionCount() const { return instructionStrings.size(); }
};
