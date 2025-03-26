#include "ForwardingProcessor.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <string.h>

ForwardingProcessor::ForwardingProcessor() : NoForwardingProcessor() {
    // Initialize any forwarding-specific variables
}

// Check for data hazards that can be resolved with forwarding
ForwardingProcessor::ForwardingUnit ForwardingProcessor::checkForwarding() {
    ForwardingUnit unit = {false, false};
    
    // No forwarding needed if ID/EX is empty
    if (idex.isEmpty) {
        return unit;
    }
    
    // ---- Check for EX/MEM forwarding ----
    if (!exmem.isEmpty && exmem.controls.regWrite && exmem.rd != 0) {
        // Forward A if rs1 matches rd in EX/MEM
        if (idex.rs1 == exmem.rd) {
            unit.forwardA = true;
            unit.forwardASource = EX_MEM_FORWARD; // From EX/MEM
            std::cout << "         Forwarding from EX/MEM to rs1 (x" << idex.rs1 << ")" << std::endl;
        }
        
        // Forward B if rs2 matches rd in EX/MEM
        if (idex.rs2 == exmem.rd) {
            unit.forwardB = true;
            unit.forwardBSource =  EX_MEM_FORWARD; // From EX/MEM
            std::cout << "         Forwarding from EX/MEM to rs2 (x" << idex.rs2 << ")" << std::endl;
        }
    }
    
    // ---- Check for MEM/WB forwarding ----
    if (!memwb.isEmpty && memwb.controls.regWrite && memwb.rd != 0) {
        // Forward A if rs1 matches rd in MEM/WB and not already forwarded from EX/MEM
        if (idex.rs1 == memwb.rd && !(unit.forwardA && unit.forwardASource == 1)) {
            unit.forwardA = true;
            unit.forwardASource = MEM_WB_FORWARD; // From MEM/WB
            std::cout << "         Forwarding from MEM/WB to rs1 (x" << idex.rs1 << ")" << std::endl;
        }
        
        // Forward B if rs2 matches rd in MEM/WB and not already forwarded from EX/MEM
        if (idex.rs2 == memwb.rd && !(unit.forwardB && unit.forwardBSource == 1)) {
            unit.forwardB = true;
            unit.forwardBSource = MEM_WB_FORWARD; // From MEM/WB
            std::cout << "         Forwarding from MEM/WB to rs2 (x" << idex.rs2 << ")" << std::endl;
        }
    }
    
    return unit;
}

void ForwardingProcessor::run(int cycles) {
    // Reset pipeline state
    pc = 0;
    stall = false;
    ifid.isEmpty = true;
    ifid.isStalled = false;
    idex.isEmpty = true;
    idex.isStalled = false;
    exmem.isEmpty = true;
    exmem.isStalled = false;
    memwb.isEmpty = true;
    
    // Clear register usage tracker - using the new register tracking methods
    for (auto& regTracker : regUsageTracker) {
        regTracker.clear();
    }
    
    // Initialize the pipeline matrix using the 3D vector approach
    matrixRows = static_cast<int>(instructionStrings.size());
    matrixCols = cycles;
    
    // Resize the outer vector to have matrixRows elements
    pipelineMatrix3D.resize(matrixRows);

    // Resize each inner vector to have matrixCols elements
    for (int i = 0; i < matrixRows; i++) {
        pipelineMatrix3D[i].resize(matrixCols);
        // Initialize each cell with a vector containing a single SPACE value
        for (int j = 0; j < matrixCols; j++) {
            pipelineMatrix3D[i][j].push_back(SPACE);
        }
    }
    
    // Simulation loop
    for (int cycle = 0; cycle < cycles; cycle++) {
        std::cout << "========== Starting Cycle " << cycle << " ==========" << std::endl;
        bool branchTaken = false;
        int32_t branchTarget = 0;
        
        // -------------------- WB Stage --------------------
        if (!memwb.isEmpty) {
            std::cout << "Cycle " << cycle << " - WB: Processing " << memwb.instructionString << " at PC: " << memwb.pc << std::endl;
            int idx = getInstructionIndex(memwb.pc);
            if (idx != -1)
                recordStage(idx, cycle, WB);
            if (memwb.controls.regWrite && memwb.rd != 0) {
                int32_t writeData = memwb.controls.memToReg ? memwb.readData : memwb.aluResult;
                registers.write(memwb.rd, writeData);
                clearRegisterUsage(memwb.rd); // Use the new method to clear register usage
                std::cout << "         Written " << writeData << " to register x" << memwb.rd << std::endl;
            }
        }
        else {
            std::cout << "Cycle " << cycle << " - WB: No instruction" << std::endl;
        }
        
        // -------------------- MEM Stage --------------------
        if (!exmem.isEmpty) {
            std::cout << "Cycle " << cycle << " - MEM: Processing " << exmem.instructionString << " at PC: " << exmem.pc << std::endl;
            int idx = getInstructionIndex(exmem.pc);
            if (idx != -1)
                recordStage(idx, cycle, MEM);
            if (exmem.controls.memRead) {
                // Use the memory access methods that handle different load types
                uint32_t funct3 = (exmem.instruction >> 12) & 0x7;
                switch (funct3) {
                    case 0x0: // LB - Load Byte (sign-extended)
                        memwb.readData = static_cast<int8_t>(dataMemory.readByte(exmem.aluResult));
                        break;
                    case 0x1: // LH - Load Half-word (sign-extended)
                        memwb.readData = dataMemory.readHalfWord(exmem.aluResult);
                        break;
                    case 0x2: // LW - Load Word
                        memwb.readData = dataMemory.readWord(exmem.aluResult);
                        break;
                    case 0x4: // LBU - Load Byte (zero-extended)
                        memwb.readData = dataMemory.readByte(exmem.aluResult);
                        break;
                    case 0x5: // LHU - Load Half-word (zero-extended)
                        memwb.readData = static_cast<uint16_t>(dataMemory.readHalfWord(exmem.aluResult) & 0xFFFF);
                        break;
                    default: // Default to word for unknown types
                        memwb.readData = dataMemory.readWord(exmem.aluResult);
                        break;
                }
                std::cout << "         Read from memory at address " << exmem.aluResult << " data: " << memwb.readData << std::endl;
            }
            if (exmem.controls.memWrite) {
                // Use the memory access methods that handle different store types
                uint32_t funct3 = (exmem.instruction >> 12) & 0x7;
                switch (funct3) {
                    case 0x0: // SB - Store Byte
                        dataMemory.writeByte(exmem.aluResult, exmem.readData2 & 0xFF);
                        break;
                    case 0x1: // SH - Store Half-word
                        dataMemory.writeHalfWord(exmem.aluResult, exmem.readData2 & 0xFFFF);
                        break;
                    case 0x2: // SW - Store Word
                        dataMemory.writeWord(exmem.aluResult, exmem.readData2);
                        break;
                    default: // Default to word for unknown types
                        dataMemory.writeWord(exmem.aluResult, exmem.readData2);
                        break;
                }
                std::cout << "         Wrote " << exmem.readData2 << " to memory at address " << exmem.aluResult << " ---> Funct3: " << funct3 << std::endl;
            }
            memwb.pc = exmem.pc;
            memwb.aluResult = exmem.aluResult;
            memwb.rd = exmem.rd;
            memwb.controls = exmem.controls;
            memwb.instruction = exmem.instruction;
            memwb.instructionString = exmem.instructionString;
            memwb.isEmpty = false;
        }
        else {
            memwb.isEmpty = true;
            std::cout << "Cycle " << cycle << " - MEM: No instruction" << std::endl;
        }
        
        // -------------------- EX Stage --------------------
        if (!idex.isEmpty) {
            std::cout << "Cycle " << cycle << " - EX: Processing " << idex.instructionString << " at PC: " << idex.pc << std::endl;
            int idx = getInstructionIndex(idex.pc);
            if (idx != -1)
                recordStage(idx, cycle, EX);
            
            // Get forwarding control signals
            ForwardingUnit forwardingUnit = checkForwarding();
            
            // Get ALU operands with forwarding consideration
            int32_t aluOp1 = idex.readData1;  // Default to register value
            int32_t aluOp2 = idex.controls.aluSrc ? idex.imm : idex.readData2;  // Default based on aluSrc
            
            // Apply forwarding for ALU operand 1 if needed
            if (forwardingUnit.forwardA) {
                if (forwardingUnit.forwardASource == EX_MEM_FORWARD) {  // From EX/MEM
                    aluOp1 = exmem.aluResult;
                } else if (forwardingUnit.forwardASource == MEM_WB_FORWARD) {  // From MEM/WB
                    aluOp1 = memwb.controls.memToReg ? memwb.readData : memwb.aluResult;
                }
            }
            
            // Apply forwarding for ALU operand 2 if needed and not using immediate
            if (forwardingUnit.forwardB && !idex.controls.aluSrc) {
                if (forwardingUnit.forwardBSource == EX_MEM_FORWARD) {  // From EX/MEM
                    aluOp2 = exmem.aluResult;
                } else if (forwardingUnit.forwardBSource == MEM_WB_FORWARD) {  // From MEM/WB
                    aluOp2 = memwb.controls.memToReg ? memwb.readData : memwb.aluResult;
                }
            }
            
            uint32_t opcode = idex.instruction & 0x7F;
            
            // For AUIPC, override the ALU result
            if (opcode == 0x17) { // AUIPC
                exmem.aluResult = idex.pc + idex.imm;
                std::cout << "         AUIPC: PC + imm = " << exmem.aluResult << std::endl;
            }
            // For JALR and JAL, override the ALU result
            else if (opcode == 0x67 || opcode == 0x6F) {
                exmem.aluResult = idex.aluResult; // Return address (PC+4) calculated in ID stage
                std::cout << "         Setting return address (PC+4): " << exmem.aluResult << std::endl;
            }
            // Only handle ALU operations here
            else {
                exmem.aluResult = executeALU(aluOp1, aluOp2, idex.controls.aluOp);
                std::cout << "         ALU operation result: " << exmem.aluResult << std::endl;
            }
            
            // Handle branches and jumps with forwarded values
            if (opcode == 0x63) {  // Branch
                bool condition = false;
                uint32_t funct3 = (idex.instruction >> 12) & 0x7;
                condition = evaluateBranchCondition(aluOp1, aluOp2, funct3);
                
                if (condition) {
                    branchTaken = true;
                    branchTarget = idex.pc + idex.imm;
                    std::cout << "         Branch taken to PC: " << branchTarget << std::endl;
                }
                else {
                    std::cout << "         Branch not taken" << std::endl;
                }
            }
            else if (opcode == 0x6F) {  // JAL
                branchTaken = true;
                branchTarget = idex.pc + idex.imm;
                std::cout << "         JAL: Jump to PC: " << branchTarget << std::endl;
            }
            else if (opcode == 0x67) {  // JALR
                branchTaken = true;
                branchTarget = (aluOp1 + idex.imm) & ~1;  // Using forwarded value for rs1
                std::cout << "         JALR: Jump to PC: " << branchTarget << std::endl;
            }
            
            // Pass values to EX/MEM stage
            exmem.pc = idex.pc;
            exmem.readData2 = idex.readData2;
            // If we're forwarding for a store instruction, update the data to be stored
            if (opcode == 0x23 && forwardingUnit.forwardB) {  // STORE instruction
                if (forwardingUnit.forwardBSource == EX_MEM_FORWARD) {  // From EX/MEM
                    exmem.readData2 = exmem.aluResult;
                } else if (forwardingUnit.forwardBSource == MEM_WB_FORWARD) {  // From MEM/WB
                    exmem.readData2 = memwb.controls.memToReg ? memwb.readData : memwb.aluResult;
                }
                std::cout << "         Forwarded value " << exmem.readData2 << " for store instruction" << std::endl;
            }
            exmem.rd = idex.rd;
            exmem.controls = idex.controls;
            exmem.instruction = idex.instruction;
            exmem.instructionString = idex.instructionString;
            exmem.isEmpty = false;
        }
        else {
            exmem.isEmpty = true;
            std::cout << "Cycle " << cycle << " - EX: No instruction" << std::endl;
        }
        
        // -------------------- ID Stage --------------------
        if (!ifid.isEmpty) {
            std::cout << "Cycle " << cycle << " - ID: Processing " << ifid.instructionString << " at PC: " << ifid.pc << std::endl;
            int idx = getInstructionIndex(ifid.pc);
            if (idx != -1)
                recordStage(idx, cycle, ID);
            uint32_t instruction = ifid.instruction;
            uint32_t opcode = instruction & 0x7F;
            uint32_t rd  = (instruction >> 7) & 0x1F;
            uint32_t rs1 = (instruction >> 15) & 0x1F;
            uint32_t rs2 = (instruction >> 20) & 0x1F;
            int32_t imm = extractImmediate(instruction, opcode);
            
            // With forwarding, we only need to stall for load-use hazards
            bool loadUseHazard = false;
            if (!idex.isEmpty && idex.controls.memRead) {
                // Check if the instruction in ID/EX is a load and its destination register
                // is used as a source by the current instruction
                if (idex.rd != 0 && (idex.rd == rs1 || idex.rd == rs2)) {
                    loadUseHazard = true;
                    std::cout << "         Load-use hazard detected: Stalling pipeline." << std::endl;
                }
            }
            
            if (!loadUseHazard) {
                // Read register values
                int32_t rs1Value = registers.read(rs1);
                int32_t rs2Value = registers.read(rs2);
                
                // For JAL and JALR, calculate PC+4 in ID stage
                if ((opcode == 0x67 || opcode == 0x6F) && rd != 0) {
                    idex.aluResult = ifid.pc + 4;
                    std::cout << "         Setting return address (PC+4): " << idex.aluResult << " for register x" << rd << std::endl;
                }
                
                idex.readData1 = rs1Value;
                idex.readData2 = rs2Value;
                idex.pc = ifid.pc;
                idex.imm = imm;
                idex.rs1 = rs1;
                idex.rs2 = rs2;
                idex.rd = rd;
                idex.controls = decodeControlSignals(instruction);
                idex.instruction = ifid.instruction;
                idex.instructionString = ifid.instructionString;
                idex.isEmpty = false;
                idex.isStalled = false;
                
                // Add register usage tracking only for instructions that write to registers
                if (idex.controls.regWrite && rd != 0) {
                    addRegisterUsage(rd);
                    std::cout << "         Marking register x" << rd << " as busy " << " size: " << regUsageTracker[rd].size() << std::endl;
                }
            }
            else {
                stall = true;
                ifid.isStalled = true;
                idex.isEmpty = true;
            }
        }
        else {
            idex.isEmpty = true;
            std::cout << "Cycle " << cycle << " - ID: No instruction" << std::endl;
        }
        
        // -------------------- IF Stage --------------------
        std::cout << "Stall: " << stall << "; pc: " << pc << "; instructionMemory.size(): " << instructionMemory.size() << std::endl;
        if (!stall && (pc / 4 < static_cast<int32_t>(instructionMemory.size()))) {
            ifid.instruction = instructionMemory[pc / 4];
            ifid.pc = pc;
            ifid.instructionString = instructionStrings[pc / 4];
            ifid.isEmpty = false;
            ifid.isStalled = false;
            int idx = getInstructionIndex(pc);
            if (idx != -1)
                recordStage(idx, cycle, IF);
            std::cout << "Cycle " << cycle << " - IF: Fetched " << ifid.instructionString << " at PC: " << pc << std::endl;
            pc += 4;
        }
        else if (stall) {
            int idx = getInstructionIndex(pc);
            if (idx != -1)
                recordStage(idx, cycle, STALL);
            std::cout << "Cycle " << cycle << " - IF: Stall in effect, instruction remains same" << std::endl;
        }
        else {
            ifid.isEmpty = true;
            std::cout << "Cycle " << cycle << " - IF: No instruction fetched" << std::endl;
        }
        
        // -------------------- End-of-Cycle Processing --------------------
        if (branchTaken) {
            pc = branchTarget;
            ifid.isEmpty = true;
            idex.isEmpty = true;
            std::cout << "         Flushing pipeline due to branch/jump" << std::endl;
        }
        if (stall) {
            stall = false;
            ifid.isStalled = false;
        }
        
        std::cout << "========== Ending Cycle " << cycle << " ==========" << std::endl << std::endl;
    }
}