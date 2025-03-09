// processor.hpp - Base processor class
#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <memory>

// Forward declarations
class Stage;

// Control signals struct
struct ControlSignals {
    bool regWrite = false;
    bool memRead = false;
    bool memWrite = false;
    bool branch = false;
    bool jump = false;
    bool aluSrc = false;
    int aluOp = 0;
    bool memToReg = false;
    // Add more signals as needed
};

// Pipeline register struct
struct PipelineRegister {
    uint32_t pc = 0;
    uint32_t instruction = 0;
    int rs1 = 0;
    int rs2 = 0;
    int rd = 0;
    int32_t rs1Value = 0;
    int32_t rs2Value = 0;
    int32_t immediate = 0;
    int32_t aluResult = 0;
    int32_t memoryData = 0;
    ControlSignals controlSignals;
    std::string instructionString = "";
    bool stall = false;
    bool flush = false;
    bool aluSrc;
};

// Base Processor class
class Processor {
protected:
    // Registers
    int32_t registers[32] = {0};
    
    // Memory (simplified)
    std::unordered_map<uint32_t, int32_t> memory;
    
    // Pipeline stages
    std::unique_ptr<Stage> ifStage;
    std::unique_ptr<Stage> idStage;
    std::unique_ptr<Stage> exStage;
    std::unique_ptr<Stage> memStage;
    std::unique_ptr<Stage> wbStage;
    
    // Pipeline registers
    PipelineRegister if_id;
    PipelineRegister id_ex;
    PipelineRegister ex_mem;
    PipelineRegister mem_wb;
    
    // Program counter
    uint32_t pc = 0;
    
    // Input file
    std::ifstream instructionFile;
    
    // Instruction memory
    std::vector<std::pair<uint32_t, std::string>> instructions;
    
    // For visualization
    std::vector<std::vector<std::string>> pipelineVisualization;
    
    // Is a branch taken?
    bool branchTaken = false;
    
    // Instruction map for visualization
    std::unordered_map<uint32_t, std::string> instructionMap;
    
public:
    Processor(const std::string& filename);
    virtual ~Processor() = default;
    
    // Initialize processor
    virtual void initialize();
    
    // Run the processor for a number of cycles
    void run(int cycles);
    
    // Load instructions from file
    void loadInstructions(const std::string& filename);
    
    // Print the pipeline visualization
    void printPipelineVisualization();
    
    // Virtual methods to be implemented by derived classes
    virtual void advancePipeline() = 0;
    virtual void handleHazards() = 0;
    
    // Base stage functions
    void fetchInstruction();
    void decodeInstruction();
    void executeInstruction();
    void memoryAccess();
    void writeBack();
    
    // Utility functions
    int32_t getRegister(int index) const;
    void setRegister(int index, int32_t value);
    int32_t getMemory(uint32_t address) const;
    void setMemory(uint32_t address, int32_t value);
    std::string getInstructionString(uint32_t instruction);
};

#endif // PROCESSOR_HPP