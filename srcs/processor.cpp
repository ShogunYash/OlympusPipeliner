// processor.cpp - Base processor implementation
#include "processor.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>

Processor::Processor(const std::string& filename) {
    loadInstructions(filename);
    // Initialize registers
    for (int i = 0; i < 32; i++) {
        registers[i] = 0;
    }
    // Set stack pointer (x2) to a reasonable value
    registers[2] = 0x7FFFFFFF;
}

void Processor::initialize() {
    // Clear visualization
    pipelineVisualization.clear();
}

void Processor::loadInstructions(const std::string& filename) {
    instructionFile.open(filename);
    if (!instructionFile) {
        std::cerr << "Could not open file: " << filename << std::endl;
        exit(1);
    }
    
    uint32_t machineCode;
    std::string instructionText;
    
    std::string line;
    while (std::getline(instructionFile, line)) {
        // Parse line format: "address: machineCode instruction"
        std::istringstream iss(line);
        
        // Skip address field
        std::string addressStr;
        if (!(iss >> addressStr)) continue;
        
        // Remove the colon at the end
        if (addressStr.back() == ':') {
            addressStr.pop_back();
        }
        
        uint32_t address = 0;
        try {
            address = std::stoul(addressStr, nullptr, 16);
        } catch (const std::exception& e) {
            continue; // Skip invalid lines
        }
        
        // Get machine code
        std::string machineCodeStr;
        if (!(iss >> machineCodeStr)) continue;
        
        machineCode = std::stoul(machineCodeStr, nullptr, 16);
        
        // Get the rest of the line as instruction text
        std::getline(iss >> std::ws, instructionText);
        
        // Store instructions
        instructions.push_back({machineCode, instructionText});
        instructionMap[address] = instructionText;
    }
    
    instructionFile.close();
}

void Processor::run(int cycles) {
    initialize();
    
    // Resize visualization matrix
    pipelineVisualization.resize(instructions.size());
    for (auto& row : pipelineVisualization) {
        row.resize(cycles + 5, " "); // Add extra columns for overflow
    }
    
    // Run for the specified number of cycles
    for (int cycle = 0; cycle < cycles; cycle++) {
        advancePipeline();
    }
    
    printPipelineVisualization();
}

void Processor::printPipelineVisualization() {
    for (size_t i = 0; i < pipelineVisualization.size(); i++) {
        if (i < instructions.size()) {
            std::cout << instructions[i].second;
        } else {
            std::cout << "Unknown";
        }
        
        for (size_t j = 0; j < pipelineVisualization[i].size(); j++) {
            if (!pipelineVisualization[i][j].empty() && pipelineVisualization[i][j] != " ") {
                std::cout << ";" << pipelineVisualization[i][j];
            }
        }
        std::cout << std::endl;
    }
}

int32_t Processor::getRegister(int index) const {
    if (index < 0 || index >= 32) return 0;
    if (index == 0) return 0; // x0 is always 0
    return registers[index];
}

void Processor::setRegister(int index, int32_t value) {
    if (index <= 0 || index >= 32) return;
    registers[index] = value;
}

int32_t Processor::getMemory(uint32_t address) const {
    auto it = memory.find(address);
    if (it != memory.end()) {
        return it->second;
    }
    return 0; // Default value for uninitialized memory
}

void Processor::setMemory(uint32_t address, int32_t value) {
    memory[address] = value;
}

std::string Processor::getInstructionString(uint32_t instruction) {
    // Lookup in instruction map
    auto it = instructionMap.find(instruction);
    if (it != instructionMap.end()) {
        return it->second;
    }
    return "Unknown";
}

// Basic instruction fetch stage
void Processor::fetchInstruction() {
    if (branchTaken) {
        // If branch is taken, PC is already updated
        branchTaken = false;
    } else {
        // Normal fetch
        if (pc / 4 < instructions.size()) {
            if_id.instruction = instructions[pc / 4].first;
            if_id.instructionString = instructions[pc / 4].second;
            if_id.pc = pc;
            // Update PC
            pc += 4;
        }
    }
}

// Basic instruction decode stage
void Processor::decodeInstruction() {
    if (if_id.flush) {
        id_ex.flush = true;
        if_id.flush = false;
        return;
    }
    
    uint32_t instr = if_id.instruction;
    if (instr == 0) return; // No instruction to decode
    
    // Extract opcode (7 bits)
    uint32_t opcode = instr & 0x7F;
    
    // Extract registers
    id_ex.rd = (instr >> 7) & 0x1F;
    id_ex.rs1 = (instr >> 15) & 0x1F;
    id_ex.rs2 = (instr >> 20) & 0x1F;
    
    // Read register values
    id_ex.rs1Value = getRegister(id_ex.rs1);
    id_ex.rs2Value = getRegister(id_ex.rs2);
    
    // Extract immediate (depends on instruction format)
    switch (opcode) {
        // R-type (no immediate)
        case 0x33: // ADD, SUB, etc.
            id_ex.immediate = 0;
            break;
            
        // I-type
        case 0x13: // ADDI, etc.
        case 0x03: // LB, LW, etc.
        case 0x67: // JALR
            // Sign-extend the 12-bit immediate
            id_ex.immediate = ((int32_t)(instr & 0xFFF00000)) >> 20;
            break;
            
        // S-type
        case 0x23: // SB, SW, etc.
            // Sign-extend the 12-bit immediate split into two parts
            id_ex.immediate = ((int32_t)((instr & 0xFE000000) | ((instr >> 7) & 0x1F))) >> 20;
            break;
            
        // B-type
        case 0x63: // BEQ, BNE, etc.
            // Sign-extend the 13-bit immediate split into multiple parts
            id_ex.immediate = ((int32_t)(
                               ((instr & 0x80000000) >> 19) | // bit 12
                               ((instr & 0x7E000000) >> 20) | // bits 10:5
                               ((instr & 0x00000F00) >> 7) |  // bits 4:1
                               ((instr & 0x00000080) << 4)    // bit 11
                               )) >> 19;
            break;
            
        // J-type
        case 0x6F: // JAL
            // Sign-extend the 21-bit immediate split into multiple parts
            id_ex.immediate = ((int32_t)(
                               ((instr & 0x80000000) >> 11) | // bit 20
                               ((instr & 0x7FE00000) >> 20) | // bits 10:1
                               ((instr & 0x00100000) >> 9) |  // bit 11
                               (instr & 0x000FF000)           // bits 19:12
                               )) >> 11;
            break;
            
        // U-type
        case 0x37: // LUI
        case 0x17: // AUIPC
            id_ex.immediate = instr & 0xFFFFF000; // Top 20 bits
            break;
            
        default:
            id_ex.immediate = 0;
            break;
    }
    
    // Set control signals based on opcode and funct fields
    ControlSignals& ctrl = id_ex.controlSignals;
    
    // Default control signals
    ctrl.regWrite = false;
    ctrl.memRead = false;
    ctrl.memWrite = false;
    ctrl.branch = false;
    ctrl.jump = false;
    ctrl.aluSrc = false;
    ctrl.aluOp = 0;
    ctrl.memToReg = false;
    
    // Set control signals based on opcode
    switch (opcode) {
        case 0x33: // R-type (ADD, SUB, etc.)
            ctrl.regWrite = true;
            ctrl.aluOp = 2;
            break;
            
        case 0x13: // I-type (ADDI, etc.)
            ctrl.regWrite = true;
            ctrl.aluSrc = true;
            ctrl.aluOp = 2;
            break;
            
        case 0x03: // Load instructions
            ctrl.regWrite = true;
            ctrl.memRead = true;
            ctrl.aluSrc = true;
            ctrl.memToReg = true;
            break;
            
        case 0x23: // Store instructions
            ctrl.memWrite = true;
            ctrl.aluSrc = true;
            break;
            
        case 0x63: // Branch instructions
            ctrl.branch = true;
            ctrl.aluOp = 1;
            break;
            
        case 0x67: // JALR
            ctrl.jump = true;
            ctrl.regWrite = true;
            break;
            
        case 0x6F: // JAL
            ctrl.jump = true;
            ctrl.regWrite = true;
            break;
            
        case 0x37: // LUI
            ctrl.regWrite = true;
            ctrl.aluSrc = true;
            break;
            
        case 0x17: // AUIPC
            ctrl.regWrite = true;
            ctrl.aluSrc = true;
            break;
    }
    
    // Pass instruction string to next stage
    id_ex.instructionString = if_id.instructionString;
    id_ex.pc = if_id.pc;
}

// Basic execution stage
void Processor::executeInstruction() {
    if (id_ex.flush) {
        ex_mem.flush = true;
        id_ex.flush = false;
        return;
    }
    
    // ALU operation
    int32_t aluInput1 = id_ex.rs1Value;
    int32_t aluInput2 = id_ex.aluSrc ? id_ex.immediate : id_ex.rs2Value;
    
    // Execute based on control signals and instruction
    uint32_t instr = id_ex.instruction;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t funct7 = (instr >> 25) & 0x7F;
    
    // Basic ALU operations
    switch (id_ex.controlSignals.aluOp) {
        case 0: // Load/Store: Add
            ex_mem.aluResult = aluInput1 + aluInput2;
            break;
            
        case 1: // Branch: Subtract for comparison
            ex_mem.aluResult = aluInput1 - aluInput2;
            
            // Handle branch based on funct3
            if (id_ex.controlSignals.branch) {
                bool takeBranch = false;
                
                switch (funct3) {
                    case 0x0: // BEQ
                        takeBranch = (aluInput1 == aluInput2);
                        break;
                    case 0x1: // BNE
                        takeBranch = (aluInput1 != aluInput2);
                        break;
                    case 0x4: // BLT
                        takeBranch = (aluInput1 < aluInput2);
                        break;
                    case 0x5: // BGE
                        takeBranch = (aluInput1 >= aluInput2);
                        break;
                    case 0x6: // BLTU
                        takeBranch = ((uint32_t)aluInput1 < (uint32_t)aluInput2);
                        break;
                    case 0x7: // BGEU
                        takeBranch = ((uint32_t)aluInput1 >= (uint32_t)aluInput2);
                        break;
                }
                
                if (takeBranch) {
                    // Update PC for branch
                    pc = id_ex.pc + id_ex.immediate;
                    branchTaken = true;
                    
                    // Flush the pipeline
                    if_id.flush = true;
                }
            }
            break;
            
        case 2: // R-type or I-type
            switch (funct3) {
                case 0x0: // ADD/SUB
                    if (id_ex.controlSignals.aluSrc || funct7 != 0x20) {
                        ex_mem.aluResult = aluInput1 + aluInput2; // ADD/ADDI
                    } else {
                        ex_mem.aluResult = aluInput1 - aluInput2; // SUB
                    }
                    break;
                    
                case 0x1: // SLL
                    ex_mem.aluResult = aluInput1 << (aluInput2 & 0x1F);
                    break;
                    
                case 0x2: // SLT
                    ex_mem.aluResult = (aluInput1 < aluInput2) ? 1 : 0;
                    break;
                    
                case 0x3: // SLTU
                    ex_mem.aluResult = ((uint32_t)aluInput1 < (uint32_t)aluInput2) ? 1 : 0;
                    break;
                    
                case 0x4: // XOR
                    ex_mem.aluResult = aluInput1 ^ aluInput2;
                    break;
                    
                case 0x5: // SRL/SRA
                    if (funct7 == 0x20) {
                        // SRA - Arithmetic shift right
                        ex_mem.aluResult = aluInput1 >> (aluInput2 & 0x1F);
                    } else {
                        // SRL - Logical shift right
                        ex_mem.aluResult = (uint32_t)aluInput1 >> (aluInput2 & 0x1F);
                    }
                    break;
                    
                case 0x6: // OR
                    ex_mem.aluResult = aluInput1 | aluInput2;
                    break;
                    
                case 0x7: // AND
                    ex_mem.aluResult = aluInput1 & aluInput2;
                    break;
            }
            break;
    }
    
    // Handle jumps (JAL, JALR)
    if (id_ex.controlSignals.jump) {
        // For JAL, PC = PC + imm
        // For JALR, PC = rs1 + imm
        if ((instr & 0x7F) == 0x6F) { // JAL
            pc = id_ex.pc + id_ex.immediate;
        } else { // JALR
            pc = aluInput1 + id_ex.immediate;
        }
        
        // Save return address (PC + 4)
        ex_mem.aluResult = id_ex.pc + 4;
        
        branchTaken = true;
        // Flush the pipeline
        if_id.flush = true;
    }
    
    // Pass control signals and register info to next stage
    ex_mem.controlSignals = id_ex.controlSignals;
    ex_mem.rd = id_ex.rd;
    ex_mem.rs1 = id_ex.rs1;
    ex_mem.rs2 = id_ex.rs2;
    ex_mem.rs2Value = id_ex.rs2Value; // For store instructions
    ex_mem.instructionString = id_ex.instructionString;
}

// Basic memory stage
void Processor::memoryAccess() {
    if (ex_mem.flush) {
        mem_wb.flush = true;
        ex_mem.flush = false;
        return;
    }
    
    // Memory access operations
    if (ex_mem.controlSignals.memRead) {
        // Load data from memory
        mem_wb.memoryData = getMemory(ex_mem.aluResult);
    } else if (ex_mem.controlSignals.memWrite) {
        // Store data to memory
        setMemory(ex_mem.aluResult, ex_mem.rs2Value);
    }
    
    // Pass ALU result to next stage
    mem_wb.aluResult = ex_mem.aluResult;
    
    // Pass control signals and register info to next stage
    mem_wb.controlSignals = ex_mem.controlSignals;
    mem_wb.rd = ex_mem.rd;
    mem_wb.instructionString = ex_mem.instructionString;
}

// Basic write back stage
void Processor::writeBack() {
    if (mem_wb.flush) {
        mem_wb.flush = false;
        return;
    }
    
    // Write back to register file
    if (mem_wb.controlSignals.regWrite) {
        int32_t writeData;
        
        if (mem_wb.controlSignals.memToReg) {
            writeData = mem_wb.memoryData;
        } else {
            writeData = mem_wb.aluResult;
        }
        
        setRegister(mem_wb.rd, writeData);
    }
}