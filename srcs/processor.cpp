#include "processor.hpp"
#include <iostream>
#include <iomanip>

Processor::Processor() : pc(0), running(false), cycle_count(0), executed_instructions(0) {}

void Processor::loadProgram(const std::vector<uint32_t>& instructions) {
    program = instructions;
    instruction_memory.loadProgram(instructions);
    pc = 0;
    running = true;
    
    // Clear pipeline registers
    if_id = IF_ID_Register();
    id_ex = ID_EX_Register();
    ex_mem = EX_MEM_Register();
    mem_wb = MEM_WB_Register();
    
    // Clear visualization history
    pipeline_history.clear();
    cycle_count = 0;
    executed_instructions = 0;
}

void Processor::record_pipeline_state() {
    // Initialize empty state for this cycle for all tracked instructions
    for (auto& entry : pipeline_history) {
        if (entry.second.size() < static_cast<size_t>(cycle_count)) {
            entry.second.push_back(" ");
        }
    }
    
    // Record current stages for active instructions
    if (if_id.valid) {
        Instruction instr(if_id.instruction);
        updateInstructionStage(instr.getAssembly(), if_id.stage_display);
    }
    
    if (id_ex.valid) {
        updateInstructionStage(id_ex.assembly, id_ex.stage_display);
    }
    
    if (ex_mem.valid) {
        updateInstructionStage(ex_mem.assembly, ex_mem.stage_display);
    }
    
    if (mem_wb.valid) {
        updateInstructionStage(mem_wb.assembly, mem_wb.stage_display);
        
        // Count completed instructions
        if (mem_wb.stage_display == "WB") {
            executed_instructions++;
        }
    }
    
    cycle_count++;
}

void Processor::updateInstructionStage(const std::string& assembly, const std::string& stage) {
    if (assembly.empty()) return;
    
    // Don't record stalls ("-") for visualization clarity
    if (stage == "-") return;
    
    bool found = false;
    for (auto& entry : pipeline_history) {
        if (entry.first == assembly) {
            // Only update if this is a new stage for this instruction
            // (prevents duplicating EX and MEM stages for ECALL)
            if (entry.second.empty() || entry.second.back() != stage) {
                entry.second.push_back(stage);
            }
            found = true;
            break;
        }
    }
    
    if (!found) {
        // First time seeing this instruction
        std::vector<std::string> stages;
        stages.push_back(stage);
        pipeline_history.push_back({assembly, stages});
    }
}

void Processor::run(int cycles) {
    int num_cycles = 0;
    bool had_activity = false;
    
    // Add debugging to see what's happening
    std::cout << "Starting simulation..." << std::endl;
    
    // Run until program naturally completes
    while (running || if_id.valid || id_ex.valid || ex_mem.valid || mem_wb.valid) {
        // Print debug info
        std::cout << "Cycle " << num_cycles + 1 << ": PC=" << std::hex << pc << std::dec;
        std::cout << " IF:" << (if_id.valid ? "valid" : "invalid");
        std::cout << " ID:" << (id_ex.valid ? "valid" : "invalid");
        std::cout << " EX:" << (ex_mem.valid ? "valid" : "invalid");
        std::cout << " MEM:" << (mem_wb.valid ? "valid" : "invalid") << std::endl;
        
        // Execute one pipeline cycle
        step();
        
        // Record pipeline state for visualization
        record_pipeline_state();
        
        had_activity = true;
        num_cycles++;
        
        // Exit conditions
        if (!running && !if_id.valid && !id_ex.valid && !ex_mem.valid && !mem_wb.valid) {
            std::cout << "Program completed normally after " << num_cycles << " cycles." << std::endl;
            break;
        }
        
        // Prevent infinite loops
        if (num_cycles >= 100) {
            std::cout << "Reached maximum cycle limit (100). Stopping." << std::endl;
            break;
        }
        
        // User-specified limit
        if (cycles > 0 && num_cycles >= cycles) {
            std::cout << "Reached user-specified cycle limit (" << cycles << "). Stopping." << std::endl;
            break;
        }
        
        // Check for lack of progress
        if (!had_activity) {
            std::cout << "Simulation stalled with no activity. Stopping." << std::endl;
            break;
        }
        
        // Reset activity flag for next cycle
        had_activity = false;
    }
    
    std::cout << "Simulation ended after " << num_cycles << " cycles." << std::endl;
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

void Processor::handle_syscall() {
    // Get syscall number from a7 (x17)
    int syscall_num = registers.read(17); // a7 is x17
    
    std::cout << "Handling syscall: " << syscall_num << std::endl;
    
    switch (syscall_num) {
        case 1: // Print integer
            std::cout << "Syscall 1 (print_int): " << registers.read(10) << std::endl; // a0 is x10
            break;
            
        case 4: { // Print string
            uint32_t addr = registers.read(10); // String address in a0
            std::string output;
            char c;
            while ((c = data_memory.readByte(addr++)) != 0) {
                output += c;
            }
            std::cout << output;
            break;
        }
            
        case 10: // Exit program
            running = false;
            std::cout << "Program exit requested via syscall" << std::endl;
            break;
            
        case 11: { // Print character
            char c = registers.read(10) & 0xFF; // Get character from a0
            std::cout << c;
            break;
        }
            
        default:
            std::cerr << "Unknown syscall: " << syscall_num << std::endl;
            break;
    }
}
