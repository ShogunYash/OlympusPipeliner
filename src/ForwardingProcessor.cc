#include "ForwardingProcessor.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>

// Constructor/Destructor
ForwardingProcessor::ForwardingProcessor() : 
    NoForwardingProcessor(){
}

ForwardingProcessor::~ForwardingProcessor() {
}

// Override run method to implement forwarding
void ForwardingProcessor::run(int cycles) {
    // Reset pipeline state.
    pc = 0;
    stall = false;
    ifid.isEmpty = true;
    idex.isEmpty = true;
    exmem.isEmpty = true;
    memwb.isEmpty = true;
    Imm_valid = true;
    bool clear = false;
    // Allocate the pipeline matrix.
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
    
    // Simulation loop.
    for (int cycle = 0; cycle < cycles; cycle++) {
        std::cout << "========== Starting Cycle " << cycle << " ==========" << std::endl;
        bool branchTaken = false;
        int32_t branchTarget = 0;  // Changed to signed 32-bit
        
        // -------------------- WB Stage --------------------
        if (!memwb.isEmpty) {
            std::cout << "Cycle " << cycle << " - WB: Processing " << memwb.instructionString << " at PC: " << memwb.pc << std::endl;
            int idx = getInstructionIndex(memwb.pc);
            if (idx != -1)
                recordStage(idx, cycle, WB);
            // Removed write in WB stage to allow for forwarding as writing is done now earlier in MEM and EX stages
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
                // Determine the type of load based on the funct3 field
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
                // Determine the type of store based on the funct3 field
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
                std::cout << "         Wrote " << exmem.readData2 << " to memory at address " << exmem.aluResult << "---> Funt3: "<< funct3 << std::endl;
            }
            memwb.pc = exmem.pc;
            memwb.aluResult = exmem.aluResult;
            memwb.rd = exmem.rd;
            memwb.controls = exmem.controls;
            memwb.instruction = exmem.instruction;
            memwb.instructionString = exmem.instructionString;
            memwb.isEmpty = false;
            // Adding forwarding logic when load instructions are used
            if (memwb.controls.memToReg && memwb.rd != 0 && memwb.controls.regWrite ) {
                int32_t writeData = memwb.readData;
                registers.write(memwb.rd, writeData);
                // get opcodes for branches and jumps
                // uint32_t opcode = memwb.instruction & 0x7F;
                // if (!(opcode == 0x6F || opcode == 0x67 || opcode == 0x63)) 
                //     clearRegisterUsage(memwb.rd);
                std::cout << "         Written " << writeData << " to register x" << memwb.rd << std::endl;
            }
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
                
            int32_t aluOp1 = idex.readData1;
            int32_t aluOp2 = idex.controls.aluSrc ? idex.imm : idex.readData2;
            uint32_t opcode = idex.instruction & 0x7F;
            
            
            // For AUIPC, override the ALU result
            if (opcode == 0x17) { // AUIPC
                exmem.aluResult = idex.pc + idex.imm;
                std::cout << "         AUIPC: PC + imm = " << exmem.aluResult << std::endl;
            }
            // Add special case for LUI
            else if (opcode == 0x37) { // LUI
                exmem.aluResult = idex.imm;
                std::cout << "         LUI: imm = " << exmem.aluResult << std::endl;
            }
            // For JALR and JAL, override the ALU result
            else if (opcode == 0x67 || opcode == 0x6F) {
                exmem.aluResult = idex.aluResult;
                std::cout << "         Setting return address (PC+4): " << exmem.aluResult << std::endl;
            }
            // Only handle ALU operations here, branch/jump is already handled in ID stage
            else {
                exmem.aluResult = executeALU(aluOp1, aluOp2, idex.controls.aluOp);
            }
            
            // Debug output for XORI instruction
            if (opcode == 0x13 && ((idex.instruction >> 12) & 0x7) == 0x5) {
                std::cout << "         XORI operation: " << aluOp1 << " ^ " << aluOp2
                         << " = " << exmem.aluResult << " (ALU op: " << idex.controls.aluOp << ")" << std::endl;
            }
            
            std::cout << "         ALU operation result: " << exmem.aluResult << std::endl;
            
            exmem.pc = idex.pc;
            exmem.readData2 = idex.readData2;
            exmem.rd = idex.rd;
            exmem.controls = idex.controls;
            exmem.instruction = idex.instruction;
            exmem.instructionString = idex.instructionString;
            exmem.isEmpty = false;

            // Adding forwarding logic here when EX stage computes a register value to write
            if (exmem.controls.regWrite && exmem.rd != 0 && !exmem.controls.memToReg && !(opcode == 0x6F)) {
                int32_t writeData = exmem.aluResult;
                registers.write(exmem.rd, writeData);
                // // get opcodes for branches and jumps
                // if (!(opcode == 0x6F || opcode == 0x67 || opcode == 0x63)) 
                //     clearRegisterUsage(exmem.rd);
                std::cout << "         Written " << writeData << " to register x" << exmem.rd << std::endl;
            }
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
            
            // Read register values here for hazard detection and branch computation
            int32_t rs1Value = registers.read(rs1);
            int32_t rs2Value = registers.read(rs2);
            
            // use opcode to find whether the instruction is a branch or jump
            clear = false;
            bool rs1UsedEX = (!exmem.isEmpty && rs1 == exmem.rd && exmem.controls.regWrite && exmem.rd != 0 && !exmem.controls.memToReg);
            bool rs1UsedMEM = (!memwb.isEmpty && rs1 == memwb.rd && exmem.controls.memToReg && rs1!=0);
            bool rs2UsedEX = (!exmem.isEmpty && rs2 == exmem.rd && exmem.controls.regWrite && exmem.rd != 0 && !exmem.controls.memToReg);
            bool rs2UsedMEM = (!memwb.isEmpty && rs2 == memwb.rd && exmem.controls.memToReg && rs2!=0);
            clear = ((opcode == 0x67 &&( rs1UsedEX || rs1UsedMEM)) || (opcode == 0x63 && (rs1UsedEX || rs1UsedMEM || rs2UsedEX || rs2UsedMEM)));
            if (!clear) {
                // Updating register usage for Ex and Mem stage for forwarding and not for branch/jump instructions
                if(!exmem.isEmpty && exmem.controls.regWrite && exmem.rd != 0 && !exmem.controls.memToReg){
                    clearRegisterUsage(exmem.rd);
                    std::cout<<"----------------------> x"<< exmem.rd << " is not a branch or jump instruction"<<std::endl;
                }
                if(!memwb.isEmpty && memwb.controls.memToReg && memwb.rd != 0 && memwb.controls.regWrite){
                    clearRegisterUsage(memwb.rd);
                    std::cout<<"----------------------> x"<< memwb.rd << " is not a branch or jump instruction"<<std::endl;
                }
            }

            // More precise hazard detection based on instruction type
            bool hazard = false;
            hazard = detect_hazard(hazard, opcode, rs1, rs2);

            if (!hazard) {
                // Calculate branch or jump target in ID stage if applicable
                if (opcode == 0x63 || opcode == 0x67 || opcode == 0x6F) {
                    branchTaken = handleBranchAndJump(opcode, instruction, rs1Value, 
                                                     imm, ifid.pc, rs2Value, branchTarget);
                    if(!Imm_valid){
                        std::cout<<"Invalid Immediate value"<<std::endl;
                        std::cout<<"Instruction: "<<ifid.instructionString<<std::endl;
                        std::cout<<"----------------------> Breaking the simulation"<<std::endl;
                        return;
                    }
                }
                
                // For JAL and JALR, store PC+4 in register rd
                if ((opcode == 0x67 || opcode == 0x6F) && rd != 0) {
                    // Set up the return address to be written to rd in later stages
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
                // Added to support illegal instruction detection
                if(idex.controls.illegal_instruction){
                    std::cout<<"Illegal instruction detected at PC: "<< ifid.pc <<std::endl;
                    std::cout<<"Instruction: "<<ifid.instructionString<<std::endl;
                    std::cout<<"----------------------> Breaking the simulation"<<std::endl;
                    return;
                }
                if (idex.controls.regWrite && rd != 0 ) {                          
                    addRegisterUsage(rd);
                    std::cout << "         Marking register x" << rd << " as busy "<< " size: "<< regUsageTracker[rd].size() << std::endl;
                }
                
                if(opcode == 0x6F && rd != 0){
                    registers.write(rd, idex.aluResult);
                    std::cout << "         Written " << idex.aluResult << " to register x" << rd << std::endl;
                    clearRegisterUsage(rd);
                    std::cout<<"----------------------> x"<< rd << " is not a branch or jump instruction"<<std::endl;
                }
            }
            else {
                stall = true;
                idex.isEmpty = true;
                std::cout << "         Hazard detected: Stalling pipeline." << std::endl;
                if (rs1 != 0 && isRegisterUsedBy(rs1))
                    std::cout << "         Register x" << rs1 << " is in use"<< " size: "<< regUsageTracker[rd].size() << std::endl;
                if (rs2 != 0 && isRegisterUsedBy(rs2) &&
                    (opcode == 0x33 || opcode == 0x23 || opcode == 0x63))
                    std::cout << "         Register x" << rs2 << " is in use"<< " size: "<< regUsageTracker[rd].size() << std::endl;
            }
        }
        else {
            idex.isEmpty = true;
            std::cout << "Cycle " << cycle << " - ID: No instruction" << std::endl;
        }
        
        // Updating register usage for Ex and Mem stage for forwarding and branch/jump instructions
        // uint32_t opcode =  ifid.instruction & 0x7F;
        if(clear) {
            if(!exmem.isEmpty && exmem.controls.regWrite && exmem.rd != 0 && !exmem.controls.memToReg){
                clearRegisterUsage(exmem.rd);
            }
            if(!memwb.isEmpty && memwb.controls.memToReg && memwb.rd != 0 && memwb.controls.regWrite)
                clearRegisterUsage(memwb.rd);
        }
        // -------------------- IF Stage --------------------
        std::cout << "Stall: " << stall << "; pc: " << pc << "; instructionMemory.size(): " << instructionMemory.size() << std::endl;
        if (!stall && (pc / 4 < static_cast<int32_t>(instructionMemory.size()))) {  // Adjusted for signed pc
            ifid.instruction = instructionMemory[pc / 4];
            ifid.pc = pc;
            ifid.instructionString = instructionStrings[pc / 4];
            ifid.isEmpty = false;
            int idx = getInstructionIndex(ifid.pc);
            if (idx != -1)
                recordStage(idx, cycle, IF);
            std::cout << "Cycle " << cycle << " - IF: Fetched " << ifid.instructionString << " at PC: " << pc << std::endl;
            pc += 4;
        }
        else if (stall) {
            int idx = getInstructionIndex(pc);
            if (idx != -1)
                recordStage(idx, cycle, IF);
            std::cout << "Cycle " << cycle << " - IF: Stall in effect, instruction remains same" << std::endl;
        }
        else {
            ifid.isEmpty = true;
            std::cout << "Cycle " << cycle << " - IF: No instruction fetched" << std::endl;
        }
        
        // -------------------- End-of-Cycle Processing --------------------


        if (branchTaken) {
            pc = branchTarget;
            // If we have a branch/jump in ID, we only need to flush IF stage
            ifid.isEmpty = true;
            std::cout << "         Flushing pipeline due to branch/jump" << std::endl;
        }
        if (stall) {
            stall = false;
        }
        
        std::cout << "========== Ending Cycle " << cycle << " ==========" << std::endl << std::endl;
    }
}