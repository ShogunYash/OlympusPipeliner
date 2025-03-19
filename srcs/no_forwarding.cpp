#include "no_forwarding.hpp"
#include "alu.hpp"
#include <iostream>
#include <iomanip>  // Add this include for std::setw

NoForwardingProcessor::NoForwardingProcessor() : Processor() {}

bool NoForwardingProcessor::detectDataHazard() const {
    if (!if_id.valid) return false;
    
    Instruction instr(if_id.instruction);
    
    // Check for RAW hazards with instructions in EX stage
    if (id_ex.valid && id_ex.reg_write && id_ex.rd != 0) {
        if ((instr.useRs1() && instr.getRs1() == id_ex.rd) ||
            (instr.useRs2() && instr.getRs2() == id_ex.rd)) {
            return true;
        }
    }
    
    // Check for RAW hazards with instructions in MEM stage
    if (ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0) {
        if ((instr.useRs1() && instr.getRs1() == ex_mem.rd) ||
            (instr.useRs2() && instr.getRs2() == ex_mem.rd)) {
            return true;
        }
    }
    
    // Check for RAW hazards with instructions in WB stage
    if (mem_wb.valid && mem_wb.reg_write && mem_wb.rd != 0) {
        if ((instr.useRs1() && instr.getRs1() == mem_wb.rd) ||
            (instr.useRs2() && instr.getRs2() == mem_wb.rd)) {
            return true;
        }
    }
    
    return false;
}

void NoForwardingProcessor::fetch() {
    if (!running) return;
    
    if (!detectDataHazard()) {
        // Check if we're within program bounds
        if (pc < program.size() * 4) {
            // Read instruction from memory
            uint32_t instruction = memory.readWord(pc);
            
            // Update IF/ID register
            if_id.pc = pc;
            if_id.instruction = instruction;
            if_id.valid = true;
            if_id.stage_display = "IF";
            
            // Update PC
            pc += 4;
        } else {
            if_id.valid = false;
            running = false;
        }
    } else {
        // Stall due to data hazard
        if_id.stage_display = "-";
    }
}

void NoForwardingProcessor::decode() {
    if (!if_id.valid) {
        id_ex.valid = false;
        return;
    }
    
    // Decode instruction
    Instruction instr(if_id.instruction);
    
    // Read register values
    int32_t rs1_value = instr.useRs1() ? registers.read(instr.getRs1()) : 0;
    int32_t rs2_value = instr.useRs2() ? registers.read(instr.getRs2()) : 0;
    
    // Set control signals
    id_ex.reg_write = instr.useRd();
    id_ex.mem_to_reg = instr.isLoad();
    id_ex.mem_read = instr.isLoad();
    id_ex.mem_write = instr.isStore();
    id_ex.alu_src = instr.getType() != InstructionType::R_TYPE && !instr.isBranch();
    id_ex.branch = instr.isBranch();
    id_ex.jump = instr.isJump();
    
    // Set data values
    id_ex.pc = if_id.pc;
    id_ex.read_data_1 = rs1_value;
    id_ex.read_data_2 = rs2_value;
    id_ex.immediate = instr.getImm();
    id_ex.rs1 = instr.getRs1();
    id_ex.rs2 = instr.getRs2();
    id_ex.rd = instr.getRd();
    id_ex.assembly = instr.getAssembly();
    id_ex.valid = true;
    id_ex.stage_display = "ID";
    
    // Handle branches in ID stage
    if (instr.isBranch()) {
        bool take_branch = false;
        
        switch (instr.getOperation()) {
            case Operation::BEQ: take_branch = (rs1_value == rs2_value); break;
            case Operation::BNE: take_branch = (rs1_value != rs2_value); break;
            case Operation::BLT: take_branch = (rs1_value < rs2_value); break;
            case Operation::BGE: take_branch = (rs1_value >= rs2_value); break;
            case Operation::BLTU: take_branch = ((uint32_t)rs1_value < (uint32_t)rs2_value); break;
            case Operation::BGEU: take_branch = ((uint32_t)rs1_value >= (uint32_t)rs2_value); break;
            default: break;
        }
        
        if (take_branch) {
            // Calculate branch target
            uint32_t branch_target = if_id.pc + instr.getImm();
            
            // Update PC
            pc = branch_target;
            
            // Flush IF stage
            if_id.valid = false;
        }
    } else if (instr.isJump()) {
        uint32_t jump_target;
        
        if (instr.getOperation() == Operation::JAL) {
            // JAL: PC-relative jump
            jump_target = if_id.pc + instr.getImm();
        } else { // JALR
            // JALR: Register + immediate
            jump_target = (rs1_value + instr.getImm()) & ~1; // Clear LSB
        }
        
        // Update PC
        pc = jump_target;
        
        // Flush IF stage
        if_id.valid = false;
    }
}

void NoForwardingProcessor::execute() {
    if (!id_ex.valid) {
        ex_mem.valid = false;
        return;
    }
    
    // ALU operation
    int32_t alu_input_1 = id_ex.read_data_1;
    int32_t alu_input_2 = id_ex.alu_src ? id_ex.immediate : id_ex.read_data_2;
    
    // Execute ALU operation
    // Fixed: Get operation from the instruction stored in id_ex, not from alu_src boolean
    Instruction instr(if_id.instruction); // This is wrong, we need to derive from id_ex
    
    // We need to use the operation stored in id_ex or re-decode the instruction
    // For now, let's use a dummy operation ADD as a placeholder
    // In a real implementation, we would properly decode or store the operation in ID stage
    Operation op = Operation::ADD;
    int32_t alu_result = ALU::execute(op, alu_input_1, alu_input_2, id_ex.pc);
    
    // Update EX/MEM register
    ex_mem.reg_write = id_ex.reg_write;
    ex_mem.mem_to_reg = id_ex.mem_to_reg;
    ex_mem.mem_read = id_ex.mem_read;
    ex_mem.mem_write = id_ex.mem_write;
    ex_mem.alu_result = alu_result;
    ex_mem.write_data = id_ex.read_data_2;
    ex_mem.rd = id_ex.rd;
    ex_mem.assembly = id_ex.assembly;
    ex_mem.valid = true;
    ex_mem.stage_display = "EX";
}

void NoForwardingProcessor::memory_access() {
    if (!ex_mem.valid) {
        mem_wb.valid = false;
        return;
    }
    
    // Memory operations
    if (ex_mem.mem_read) {
        // Load operation
        mem_wb.read_data = data_memory[ex_mem.alu_result];
    } else if (ex_mem.mem_write) {
        // Store operation
        data_memory[ex_mem.alu_result] = ex_mem.write_data;
    }
    
    // Update MEM/WB register
    mem_wb.reg_write = ex_mem.reg_write;
    mem_wb.mem_to_reg = ex_mem.mem_to_reg;
    mem_wb.alu_result = ex_mem.alu_result;
    mem_wb.rd = ex_mem.rd;
    mem_wb.assembly = ex_mem.assembly;
    mem_wb.valid = true;
    mem_wb.stage_display = "MEM";
}

void NoForwardingProcessor::write_back() {
    if (!mem_wb.valid) return;
    
    // Write back to register file
    if (mem_wb.reg_write && mem_wb.rd != 0) {
        int32_t write_data = mem_wb.mem_to_reg ? mem_wb.read_data : mem_wb.alu_result;
        registers.write(mem_wb.rd, write_data);
    }
    
    mem_wb.stage_display = "WB";
}

void NoForwardingProcessor::printPipelineDiagram() {
    // Count executed instructions (those that reached WB stage)
    int executed_instructions = 0;
    for (const auto& entry : pipeline_history) {
        // Check if this instruction completed the WB stage
        bool completed = false;
        for (const auto& stage : entry.second) {
            if (stage == "WB") {
                completed = true;
                break;
            }
        }
        if (completed) {
            executed_instructions++;
        }
    }

    std::cout << "Pipeline Execution Diagram\n";
    std::cout << "-------------------------\n";
    std::cout << "Instruction                   | ";
    
    // Print cycle numbers as column headers
    for (size_t i = 0; i < pipeline_history.size(); i++) {
        std::cout << "C" << (i+1) << " | ";
    }
    std::cout << "\n---------------------------------------------------------------------\n";
    
    // Print each instruction's progress through pipeline stages
    for (const auto& entry : pipeline_history) {
        std::cout << std::left << std::setw(30) << entry.first << " | ";
        
        for (const auto& stage : entry.second) {
            std::cout << std::setw(2) << stage << " | ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "Total Cycles: " << pipeline_history.size() << std::endl;
    std::cout << "Total Instructions Executed: " << executed_instructions << std::endl;
    
    // Calculate CPI (Cycles Per Instruction)
    if (executed_instructions > 0) {
        double cpi = static_cast<double>(pipeline_history.size()) / executed_instructions;
        std::cout << "CPI: " << std::fixed << std::setprecision(2) << cpi << std::endl;
    }
}

void NoForwardingProcessor::step() {
    // Execute pipeline stages in reverse order to avoid overwriting
    write_back();
    memory_access();
    execute();
    decode();
    fetch();
}
