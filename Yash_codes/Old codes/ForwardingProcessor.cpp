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
    
    // Clear register usage flags - with forwarding we don't need these
    std::fill(std::begin(regInUse), std::end(regInUse), false);
    
    // Allocate the pipeline matrix
    matrixRows = static_cast<int>(instructionStrings.size());
    matrixCols = cycles;
    pipelineMatrix = (PipelineStage**)malloc(matrixRows * sizeof(PipelineStage*));
    for (int i = 0; i < matrixRows; i++) {
        pipelineMatrix[i] = (PipelineStage*)malloc(matrixCols * sizeof(PipelineStage));
        memset(pipelineMatrix[i], STALL, matrixCols * sizeof(PipelineStage)); // initialize all cells to STALL marker
    }
    
    // Simulation loop
    for (int cycle = 0; cycle < cycles; cycle++) {
        std::cout << "========== Starting Cycle " << cycle << " ==========" << std::endl;
        bool branchTaken = false;
        int32_t branchTarget = 0;
        
        // -------------------- WB Stage --------------------
        if (!memwb.isEmpty) {
            std::cout << "Cycle " << cycle << " - WB: Processing " << memwb.instructionString << " at PC: " << memwb.pc << std::endl;
            int idx = getInstructionIndex(memwb.instructionString);
            if (idx != -1)
                recordStage(idx, cycle, WB);
            if (memwb.controls.regWrite && memwb.rd != 0) {
                uint32_t writeData = memwb.controls.memToReg ? memwb.readData : memwb.aluResult;
                registers.write(memwb.rd, writeData);
                std::cout << "         Written " << writeData << " to register x" << memwb.rd << std::endl;
            }
        }
        else {
            std::cout << "Cycle " << cycle << " - WB: No instruction" << std::endl;
        }
        
        // -------------------- MEM Stage --------------------
        if (!exmem.isEmpty) {
            std::cout << "Cycle " << cycle << " - MEM: Processing " << exmem.instructionString << " at PC: " << exmem.pc << std::endl;
            int idx = getInstructionIndex(exmem.instructionString);
            if (idx != -1)
                recordStage(idx, cycle, MEM);
            if (exmem.controls.memRead) {
                memwb.readData = dataMemory.readWord(exmem.aluResult);
                std::cout << "         Read from memory at address " << exmem.aluResult << " data: " << memwb.readData << std::endl;
            }
            if (exmem.controls.memWrite) {
                dataMemory.writeWord(exmem.aluResult, exmem.readData2);
                std::cout << "         Wrote " << exmem.readData2 << " to memory at address " << exmem.aluResult << std::endl;
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
            int idx = getInstructionIndex(idex.instructionString);
            if (idx != -1)
                recordStage(idx, cycle, EX);
            
            // Get forwarding control signals
            ForwardingUnit forwardingUnit = checkForwarding();
            
            // Get ALU operands with forwarding consideration
            int32_t aluOp1 = idex.readData1;  // Default to register value
            int32_t aluOp2 = idex.controls.aluSrc ? idex.imm : idex.readData2;  // Default based on aluSrc
            
            // Apply forwarding for ALU operand 1 if needed
            if (forwardingUnit.forwardA) {
                if (forwardingUnit.forwardASource == 1) {  // From EX/MEM
                    aluOp1 = exmem.aluResult;
                } else if (forwardingUnit.forwardASource == 2) {  // From MEM/WB
                    aluOp1 = memwb.controls.memToReg ? memwb.readData : memwb.aluResult;
                }
            }
            
            // Apply forwarding for ALU operand 2 if needed and not using immediate
            if (forwardingUnit.forwardB && !idex.controls.aluSrc) {
                if (forwardingUnit.forwardBSource == 1) {  // From EX/MEM
                    aluOp2 = exmem.aluResult;
                } else if (forwardingUnit.forwardBSource == 2) {  // From MEM/WB
                    aluOp2 = memwb.controls.memToReg ? memwb.readData : memwb.aluResult;
                }
            }
            
            // Execute ALU operation
            exmem.aluResult = executeALU(aluOp1, aluOp2, idex.controls.aluOp);
            std::cout << "         ALU operation result: " << exmem.aluResult << std::endl;
            
            // Handle branches and jumps
            uint32_t opcode = idex.instruction & 0x7F;
            if (opcode == 0x63) {  // Branch
                bool condition = false;
                uint32_t funct3 = (idex.instruction >> 12) & 0x7;
                switch (funct3) {
                    case 0x0: condition = (aluOp1 == aluOp2); break;  // BEQ - using forwarded values
                    case 0x1: condition = (aluOp1 != aluOp2); break;  // BNE - using forwarded values
                    case 0x4: condition = (aluOp1 < aluOp2); break;   // BLT - using forwarded values
                    case 0x5: condition = (aluOp1 >= aluOp2); break;  // BGE - using forwarded values
                    case 0x6: condition = (static_cast<uint32_t>(aluOp1) < static_cast<uint32_t>(aluOp2)); break;  // BLTU - using forwarded values
                    case 0x7: condition = (static_cast<uint32_t>(aluOp1) >= static_cast<uint32_t>(aluOp2)); break; // BGEU - using forwarded values
                }
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
                if (forwardingUnit.forwardBSource == 1) {  // From EX/MEM
                    exmem.readData2 = exmem.aluResult;
                } else if (forwardingUnit.forwardBSource == 2) {  // From MEM/WB
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
            int idx = getInstructionIndex(ifid.instructionString);
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
                idex.readData1 = registers.read(rs1);
                idex.readData2 = registers.read(rs2);
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
            int idx = getInstructionIndex(ifid.instructionString);
            if (idx != -1)
                recordStage(idx, cycle, IF);
            std::cout << "Cycle " << cycle << " - IF: Fetched " << ifid.instructionString << " at PC: " << pc << std::endl;
            pc += 4;
        }
        else if (stall) {
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