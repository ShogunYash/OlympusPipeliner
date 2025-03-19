#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include "instruction.hpp"
#include "register_file.hpp"
#include "memory.hpp"
#include "pipeline.hpp"

class Processor {
protected:
    // Processor state
    uint32_t pc;              // Program counter
    bool running;
    
    // Components
    RegisterFile registers;
    Memory memory;
    std::vector<uint32_t> program;
    
    // Pipeline registers
    IF_ID_Register if_id;
    ID_EX_Register id_ex;
    EX_MEM_Register ex_mem;
    MEM_WB_Register mem_wb;
    
    // Memory for data storage
    std::unordered_map<uint32_t, int32_t> data_memory;
    
    // Pipeline visualization
    std::vector<std::pair<std::string, std::vector<std::string>>> pipeline_history;
    
    // Pipeline stage implementation
    virtual void fetch() = 0;
    virtual void decode() = 0;
    virtual void execute() = 0;
    virtual void memory_access() = 0;
    virtual void write_back() = 0;
    
    // Record pipeline state for visualization
    void record_pipeline_state();
    
public:
    Processor();
    virtual ~Processor() = default;
    
    // Load program
    void loadProgram(const std::vector<uint32_t>& instructions);
    
    // Run processor
    virtual void step() = 0;
    void run(int cycles);
    
    // Output pipeline diagram
    void displayPipeline() const;
};

#endif // PROCESSOR_HPP
