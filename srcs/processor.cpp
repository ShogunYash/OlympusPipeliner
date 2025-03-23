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
            // Only update if this is a reasonable next stage
            // Fix the issue with stages being duplicated or out of order
            if (entry.second.empty() || isPipelineProgressionValid(entry.second.back(), stage)) {
                entry.second.push_back(stage);
            } else {
                // If we get here with an ECALL that's behaving oddly, just keep the current stage
                if (assembly == "ecall" && entry.second.back() == stage) {
                    // Don't add duplicate stages for ECALL
                } else {
                    // For non-ECALL instructions, add the stage if different
                    if (entry.second.back() != stage) {
                        entry.second.push_back(stage);
                    }
                }
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

// Add helper function to check valid pipeline progression
bool Processor::isPipelineProgressionValid(const std::string& current, const std::string& next) {
    // Define the valid progression order
    std::string order = "IF-ID-EX-ME-WB";
    
    // Get positions
    size_t curr_pos = order.find(current);
    size_t next_pos = order.find(next);
    
    // Valid if next stage comes after current stage in the pipeline
    return (curr_pos != std::string::npos && next_pos != std::string::npos && next_pos > curr_pos);
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
        if (num_cycles >= 30) {
            std::cout << "Reached maximum cycle limit (30). Stopping." << std::endl;
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
    int a0_value = registers.read(10);    // a0 is x10
    
    std::cout << "SYSCALL " << syscall_num << " (a0=" << a0_value << ")" << std::endl;
    
    switch (syscall_num) {
        case 1: // Print integer
            std::cout << "OUTPUT: " << a0_value << std::endl;
            break;
            
        case 4: // Print string
            {
                uint32_t addr = a0_value; // String address in a0
                std::string output;
                char c;
                while ((c = data_memory.readByte(addr++)) != 0) {
                    output += c;
                }
                std::cout << "OUTPUT STRING: " << output << std::endl;
            }
            break;
            
        case 10: // Exit program
            std::cout << "Program exit requested via syscall" << std::endl;
            running = false;
            // Clear all pipeline stages immediately
            if_id.valid = false;
            id_ex.valid = false;
            ex_mem.valid = false;
            mem_wb.valid = false;
            break;
            
        case 0: // Likely a register value isn't set yet
            std::cerr << "Warning: Invalid syscall number 0 - register a7 might not be initialized yet" << std::endl;
            break;
            
        default:
            std::cerr << "Error: Unknown syscall: " << syscall_num << std::endl;
            break;
    }
}

void Processor::update_pipeline_registers() {
    // Update all pipeline registers with their next values
    if_id = next_if_id;
    id_ex = next_id_ex;
    ex_mem = next_ex_mem;
    mem_wb = next_mem_wb;
    
    // Clear the next registers for the next cycle
    next_if_id = IF_ID_Register();
    next_id_ex = ID_EX_Register();
    next_ex_mem = EX_MEM_Register();
    next_mem_wb = MEM_WB_Register();
}
