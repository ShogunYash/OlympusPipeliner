#include "processor.hpp"
#include <iostream>
#include <iomanip>

Processor::Processor() : pc(0), running(false) {}

void Processor::loadProgram(const std::vector<uint32_t>& instructions) {
    program = instructions;
    memory.loadProgram(instructions);
    pc = 0;
    running = true;
    
    // Clear pipeline registers
    if_id = IF_ID_Register();
    id_ex = ID_EX_Register();
    ex_mem = EX_MEM_Register();
    mem_wb = MEM_WB_Register();
    
    // Clear visualization history
    pipeline_history.clear();
}

void Processor::record_pipeline_state() {
    // Helper function to record a stage
    auto record_stage = [this](const std::string& assembly, const std::string& stage_display) {
        if (assembly.empty()) return;
        
        bool found = false;
        for (auto& entry : pipeline_history) {
            if (entry.first == assembly) {
                // Check if we need to add more stages
                while (entry.second.size() < pipeline_history.size()) {
                    entry.second.push_back(" ");
                }
                entry.second.push_back(stage_display);
                found = true;
                break;
            }
        }
        
        if (!found) {
            pipeline_history.push_back({assembly, {stage_display}});
        }
    };

    // Record IF stage
    if (if_id.valid) {
        Instruction instr(if_id.instruction);
        std::string assembly = instr.getAssembly();
        record_stage(assembly, if_id.stage_display);
    }
    
    // Record ID stage
    if (id_ex.valid) {
        record_stage(id_ex.assembly, id_ex.stage_display);
    }
    
    // Record EX stage
    if (ex_mem.valid) {
        record_stage(ex_mem.assembly, ex_mem.stage_display);
    }
    
    // Record MEM stage
    if (mem_wb.valid) {
        record_stage(mem_wb.assembly, mem_wb.stage_display);
    }
}

void Processor::run(int cycles) {
    for (int i = 0; i < cycles && running; i++) {
        step();
        record_pipeline_state();
    }
}

void Processor::displayPipeline() const {
    for (const auto& entry : pipeline_history) {
        std::cout << entry.first;
        
        for (const auto& stage : entry.second) {
            std::cout << ";" << stage;
        }
        
        std::cout << std::endl;
    }
}
