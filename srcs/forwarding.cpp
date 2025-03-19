#include "forwarding.hpp"
#include "alu.hpp"
#include <iostream>
#include <iomanip>

ForwardingProcessor::ForwardingProcessor(const std::vector<uint32_t>& program) : Processor() {
    loadProgram(program);
}

bool ForwardingProcessor::detectLoadUseHazard() const {
    if (!if_id.valid) return false;
    
    Instruction instr(if_id.instruction);
    
    // Load-use hazard: when a load is followed immediately by an instruction that uses the loaded value
    if (id_ex.valid && id_ex.mem_read && id_ex.rd != 0) {
        if ((instr.useRs1() && instr.getRs1() == id_ex.rd) ||
            (instr.useRs2() && instr.getRs2() == id_ex.rd)) {
            return true;
        }
    }
    
    return false;
}

ForwardingProcessor::ForwardingSource ForwardingProcessor::getForwardingSource1() const {
    if (!id_ex.valid || id_ex.rs1 == 0) return ForwardingSource::NONE;
    
    // Forward from EX/MEM stage (highest priority)
    if (ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0 && ex_mem.rd == id_ex.rs1) {
        return ForwardingSource::EX_MEM;
    }
    
    // Forward from MEM/WB stage
    if (mem_wb.valid && mem_wb.reg_write && mem_wb.rd != 0 && mem_wb.rd == id_ex.rs1) {
        return ForwardingSource::MEM_WB;
    }
    
    return ForwardingSource::NONE;
}

ForwardingProcessor::ForwardingSource ForwardingProcessor::getForwardingSource2() const {
    if (!id_ex.valid || id_ex.rs2 == 0) return ForwardingSource::NONE;
    
    // Forward from EX/MEM stage (highest priority)
    if (ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0 && ex_mem.rd == id_ex.rs2) {
        return ForwardingSource::EX_MEM;
    }
    
    // Forward from MEM/WB stage
    if (mem_wb.valid && mem_wb.reg_write && mem_wb.rd != 0 && mem_wb.rd == id_ex.rs2) {
        return ForwardingSource::MEM_WB;
    }
    
    return ForwardingSource::NONE;
}

void ForwardingProcessor::fetch() {
    if (!running) return;
    
    if (!detectLoadUseHazard()) {
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
        // Stall due to load-use hazard only (we can't forward from MEM stage)
        if_id.stage_display = "-";
    }
}

void ForwardingProcessor::decode() {
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
    
    // Handle branches in ID stage with forwarding for branch operands
    if (instr.isBranch()) {
        bool take_branch = false;
        
        // Forward values for branch comparison if needed
        int32_t rs1_forwarded = rs1_value;
        int32_t rs2_forwarded = rs2_value;
        
        // Forward from EX/MEM stage
        if (ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0) {
            if (instr.getRs1() == ex_mem.rd) {
                rs1_forwarded = ex_mem.alu_result;
            }
            if (instr.getRs2() == ex_mem.rd) {
                rs2_forwarded = ex_mem.alu_result;
            }
        }
        
        // Forward from MEM/WB stage
        if (mem_wb.valid && mem_wb.reg_write && mem_wb.rd != 0) {
            if (instr.getRs1() == mem_wb.rd && 
                !(ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0 && ex_mem.rd == instr.getRs1())) {
                rs1_forwarded = mem_wb.mem_to_reg ? mem_wb.read_data : mem_wb.alu_result;
            }
            if (instr.getRs2() == mem_wb.rd &&
                !(ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0 && ex_mem.rd == instr.getRs2())) {
                rs2_forwarded = mem_wb.mem_to_reg ? mem_wb.read_data : mem_wb.alu_result;
            }
        }
        
        // Use forwarded values for branch decision
        switch (instr.getOperation()) {
            case Operation::BEQ: take_branch = (rs1_forwarded == rs2_forwarded); break;
            case Operation::BNE: take_branch = (rs1_forwarded != rs2_forwarded); break;
            case Operation::BLT: take_branch = (rs1_forwarded < rs2_forwarded); break;
            case Operation::BGE: take_branch = (rs1_forwarded >= rs2_forwarded); break;
            case Operation::BLTU: take_branch = ((uint32_t)rs1_forwarded < (uint32_t)rs2_forwarded); break;
            case Operation::BGEU: take_branch = ((uint32_t)rs1_forwarded >= (uint32_t)rs2_forwarded); break;
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
            // JALR: Register + immediate (use forwarded value if needed)
            int32_t rs1_forwarded = rs1_value;
            
            // Forward from EX/MEM stage
            if (ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0 && instr.getRs1() == ex_mem.rd) {
                rs1_forwarded = ex_mem.alu_result;
            }
            
            // Forward from MEM/WB stage
            if (mem_wb.valid && mem_wb.reg_write && mem_wb.rd != 0 && instr.getRs1() == mem_wb.rd &&
                !(ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0 && ex_mem.rd == instr.getRs1())) {
                rs1_forwarded = mem_wb.mem_to_reg ? mem_wb.read_data : mem_wb.alu_result;
            }
            
            jump_target = (rs1_forwarded + instr.getImm()) & ~1; // Clear LSB
        }
        
        // Update PC
        pc = jump_target;
        
        // Flush IF stage
        if_id.valid = false;
    }
}

void ForwardingProcessor::execute() {
    if (!id_ex.valid) {
        ex_mem.valid = false;
        return;
    }
    
    // Get forwarded values for ALU inputs
    int32_t alu_input_1 = id_ex.read_data_1;
    int32_t alu_input_2 = id_ex.alu_src ? id_ex.immediate : id_ex.read_data_2;
    
    // Determine forwarding sources
    ForwardingSource fwd_src1 = getForwardingSource1();
    ForwardingSource fwd_src2 = getForwardingSource2();
    
    // Apply forwarding for first operand
    if (fwd_src1 == ForwardingSource::EX_MEM) {
        alu_input_1 = ex_mem.alu_result;
    } else if (fwd_src1 == ForwardingSource::MEM_WB) {
        alu_input_1 = mem_wb.mem_to_reg ? mem_wb.read_data : mem_wb.alu_result;
    }
    
    // Apply forwarding for second operand (only if not using immediate)
    if (!id_ex.alu_src) {
        if (fwd_src2 == ForwardingSource::EX_MEM) {
            alu_input_2 = ex_mem.alu_result;
        } else if (fwd_src2 == ForwardingSource::MEM_WB) {
            alu_input_2 = mem_wb.mem_to_reg ? mem_wb.read_data : mem_wb.alu_result;
        }
    }
    
    // Execute ALU operation - improve operation detection
    Operation op = Operation::ADD; // Default operation
    std::string assembly = id_ex.assembly;
    
    if (assembly.find("add") == 0 && assembly.find("addi") != 0) op = Operation::ADD;
    else if (assembly.find("addi") == 0) op = Operation::ADD;
    else if (assembly.find("sub") == 0) op = Operation::SUB;
    else if (assembly.find("and") == 0 && assembly.find("andi") != 0) op = Operation::AND;
    else if (assembly.find("andi") == 0) op = Operation::AND;
    else if (assembly.find("or") == 0 && assembly.find("ori") != 0) op = Operation::OR;
    else if (assembly.find("ori") == 0) op = Operation::OR;
    else if (assembly.find("xor") == 0 && assembly.find("xori") != 0) op = Operation::XOR;
    else if (assembly.find("xori") == 0) op = Operation::XOR;
    else if (assembly.find("slt") == 0 && assembly.find("sltu") != 0 && assembly.find("slti") != 0) op = Operation::SLT;
    else if (assembly.find("slti") == 0) op = Operation::SLT;
    else if (assembly.find("sltu") == 0) op = Operation::SLTU;
    else if (assembly.find("sll") == 0 && assembly.find("slli") != 0) op = Operation::SLL;
    else if (assembly.find("slli") == 0) op = Operation::SLL;
    else if (assembly.find("srl") == 0 && assembly.find("srli") != 0) op = Operation::SRL;
    else if (assembly.find("srli") == 0) op = Operation::SRL;
    else if (assembly.find("sra") == 0 && assembly.find("srai") != 0) op = Operation::SRA;
    else if (assembly.find("srai") == 0) op = Operation::SRA;
    else if (assembly.find("lui") == 0) op = Operation::ADD; // lui just loads immediate
    else if (assembly.find("auipc") == 0) op = Operation::ADD; // auipc adds to PC
    else if (assembly.find("jal") == 0) op = Operation::ADD; // jal computes return address as PC+4
    else if (assembly.find("jalr") == 0) op = Operation::ADD; // jalr computes return address as PC+4
    else if (assembly.find("beq") == 0) op = Operation::BEQ;
    else if (assembly.find("bne") == 0) op = Operation::BNE;
    else if (assembly.find("blt") == 0) op = Operation::BLT;
    else if (assembly.find("bge") == 0) op = Operation::BGE;
    else if (assembly.find("bltu") == 0) op = Operation::BLTU;
    else if (assembly.find("bgeu") == 0) op = Operation::BGEU;
    else if (assembly.find("lb") == 0) op = Operation::ADD; // Address calculation for load
    else if (assembly.find("lh") == 0) op = Operation::ADD; // Address calculation for load
    else if (assembly.find("lw") == 0) op = Operation::ADD; // Address calculation for load
    else if (assembly.find("lbu") == 0) op = Operation::ADD; // Address calculation for load
    else if (assembly.find("lhu") == 0) op = Operation::ADD; // Address calculation for load
    else if (assembly.find("sb") == 0) op = Operation::ADD; // Address calculation for store
    else if (assembly.find("sh") == 0) op = Operation::ADD; // Address calculation for store
    else if (assembly.find("sw") == 0) op = Operation::ADD; // Address calculation for store
    else if (assembly.find("mv") == 0) op = Operation::ADD; // mv is an alias for addi rd, rs, 0
    else if (assembly.find("j") == 0) op = Operation::ADD; // j is an alias for jal x0, offset
    else if (assembly.find("li") == 0) op = Operation::ADD; // li loads immediate
    else if (assembly.find("ret") == 0) op = Operation::ADD; // ret is an alias for jalr x0, x1, 0
    
    int32_t alu_result = ALU::execute(op, alu_input_1, alu_input_2, id_ex.pc);
    
    // Update EX/MEM register
    ex_mem.reg_write = id_ex.reg_write;
    ex_mem.mem_to_reg = id_ex.mem_to_reg;
    ex_mem.mem_read = id_ex.mem_read;
    ex_mem.mem_write = id_ex.mem_write;
    ex_mem.alu_result = alu_result;
    
    // For store instructions, get the forwarded value for write_data
    int32_t write_data = id_ex.read_data_2;
    if (id_ex.mem_write) {
        if (fwd_src2 == ForwardingSource::EX_MEM) {
            write_data = ex_mem.alu_result;
        } else if (fwd_src2 == ForwardingSource::MEM_WB) {
            write_data = mem_wb.mem_to_reg ? mem_wb.read_data : mem_wb.alu_result;
        }
    }
    
    ex_mem.write_data = write_data;
    ex_mem.rd = id_ex.rd;
    ex_mem.assembly = id_ex.assembly;
    ex_mem.valid = true;
    ex_mem.stage_display = "EX";
}

void ForwardingProcessor::memory_access() {
    if (!ex_mem.valid) {
        mem_wb.valid = false;
        return;
    }
    
    // Memory operations
    if (ex_mem.mem_read) {
        // Load operation
        mem_wb.read_data = memory.readWord(ex_mem.alu_result);
    } else if (ex_mem.mem_write) {
        // Store operation
        memory.writeWord(ex_mem.alu_result, ex_mem.write_data);
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

void ForwardingProcessor::write_back() {
    if (!mem_wb.valid) return;
    
    // Write back to register file
    if (mem_wb.reg_write && mem_wb.rd != 0) {
        int32_t write_data = mem_wb.mem_to_reg ? mem_wb.read_data : mem_wb.alu_result;
        registers.write(mem_wb.rd, write_data);
    }
    
    mem_wb.stage_display = "WB";
}

void ForwardingProcessor::step() {
    // Execute pipeline stages in reverse order to avoid overwriting
    write_back();
    memory_access();
    execute();
    decode();
    fetch();
}

void ForwardingProcessor::run(int cycle_count) {
    // Clear any previous history
    pipeline_history.clear();
    executed_instructions = 0;
    
    // Run until program naturally completes
    // (all instructions processed and pipeline is empty)
    while (running || if_id.valid || id_ex.valid || ex_mem.valid || mem_wb.valid) {
        // Execute one cycle
        step();
        
        // Record current pipeline state
        std::vector<PipelineState> current_state;
        
        // Record state for all valid pipeline registers
        if (if_id.valid) {
            Instruction instr(if_id.instruction);
            current_state.push_back({instr.getAssembly(), if_id.stage_display});
        }
        
        if (id_ex.valid) {
            current_state.push_back({id_ex.assembly, id_ex.stage_display});
        }
        
        if (ex_mem.valid) {
            current_state.push_back({ex_mem.assembly, ex_mem.stage_display});
        }
        
        if (mem_wb.valid) {
            current_state.push_back({mem_wb.assembly, mem_wb.stage_display});
            
            // Count completed instructions (those in WB stage)
            if (mem_wb.stage_display == "WB") {
                executed_instructions++;
            }
        }
        
        // Add this cycle's state to history
        pipeline_history.push_back(current_state);
        
        if (cycle_count > 0 && pipeline_history.size() >= (size_t)cycle_count) {
            // Only enforce cycle limit if explicitly specified
            break;
        }
    }
    
    // Report completion
    std::cout << "Program completed after " << pipeline_history.size() << " cycles." << std::endl;
}

void ForwardingProcessor::printPipelineDiagram() {
    std::cout << "Pipeline Execution Diagram (Forwarding)\n";
    std::cout << "-----------------------------------\n";
    std::cout << "Cycle | Instruction                   | IF | ID | EX | MEM | WB\n";
    std::cout << "---------------------------------------------------------------------\n";
    
    // Print pipeline history
    for (size_t i = 0; i < pipeline_history.size(); i++) {
        std::cout << std::setw(5) << (i+1) << " | ";
        
        // Print each instruction and its progress through pipeline
        for (size_t j = 0; j < pipeline_history[i].size(); j++) {
            const auto& instr_state = pipeline_history[i][j];
            
            if (j == 0) {
                // Print instruction mnemonic for the first entry
                std::cout << std::left << std::setw(30) << instr_state.assembly << " | ";
            }
            
            // Print stage symbols
            std::cout << std::setw(2) << instr_state.stage << " | ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "Total Cycles: " << pipeline_history.size() << std::endl;
    std::cout << "Total Instructions Executed: " << executed_instructions << std::endl;
    
    // Calculate CPI
    if (executed_instructions > 0) {
        double cpi = static_cast<double>(pipeline_history.size()) / executed_instructions;
        std::cout << "CPI: " << std::fixed << std::setprecision(2) << cpi << std::endl;
    }
}
