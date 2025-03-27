#include "Processor.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <string.h>

// ---------------------- Helper Functions ----------------------
// In our updated design we still use your old immediate extraction, etc.
int32_t NoForwardingProcessor::extractImmediate(uint32_t instruction, uint32_t opcode) {
    int32_t imm = 0;
    // I-type
    if (opcode == 0x13 || opcode == 0x03 || opcode == 0x67) {
        imm = instruction >> 20;
        if (imm & 0x800) imm |= 0xFFFFF000;
    }
    // S-type
    else if (opcode == 0x23) {
        imm = ((instruction >> 25) & 0x7F) << 5;
        imm |= (instruction >> 7) & 0x1F;
        if (imm & 0x800) imm |= 0xFFFFF000;
    }
    // B-type
    else if (opcode == 0x63) {
        imm = ((instruction >> 31) & 0x1) << 12;
        imm |= ((instruction >> 7) & 0x1) << 11;
        imm |= ((instruction >> 25) & 0x3F) << 5;
        imm |= ((instruction >> 8) & 0xF) << 1;
        if (imm & 0x1000) imm |= 0xFFFFE000;
    }
    // U-type
    else if (opcode == 0x37 || opcode == 0x17) {
        imm = instruction & 0xFFFFF000;
    }
    // J-type
    else if (opcode == 0x6F) {
        imm = ((instruction >> 31) & 0x1) << 20;
        imm |= ((instruction >> 12) & 0xFF) << 12;
        imm |= ((instruction >> 20) & 0x1) << 11;
        imm |= ((instruction >> 21) & 0x3FF) << 1;
        if (imm & 0x100000) imm |= 0xFFF00000;
    }
    return imm;
}

ControlSignals NoForwardingProcessor::decodeControlSignals(uint32_t instruction) {
    ControlSignals signals;
    uint32_t opcode = instruction & 0x7F;
    uint32_t funct7 = (instruction >> 25) & 0x7F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    
    // Default signals
    signals.regWrite = false;
    signals.memRead = false;
    signals.memWrite = false;
    signals.memToReg = false;
    signals.aluSrc = false;
    signals.branch = false;
    signals.jump = false;
    signals.aluOp = 0;
    signals.illegal_instruction = false;

    switch (opcode) {
        case 0x33:  // R-type
            signals.regWrite = true;
            
            if (funct7 == 0x01) {  // M extension instructions
                // Set ALU operation based on funct3 for M-extension
                signals.aluOp = 10 + funct3;  // Use 10-17 for M-extension
            } else if (funct7 == 0x20) {
                // R-type with funct7=0x20 (SUB, SRA)
                signals.aluOp = 8 + funct3;  // Use 8-15 for R-type with funct7=0x20
            } else {
                // Standard R-type (funct7=0x00)
                signals.aluOp = funct3;  // Use 0-7 for basic operations
            }
            break;
            
        case 0x13:  // I-type
            signals.regWrite = true;
            signals.aluSrc = true;
            
            // Special case for shifts which use the funct7 field
            if (funct3 == 0x1 || funct3 == 0x5) {
                uint32_t shiftType = (instruction >> 30) & 0x1;  // bit 30 distinguishes different shifts
                if (funct3 == 0x5 && shiftType == 1) {
                    signals.aluOp = 7;  // Use SRAI (mapped to case 7)
                } else if (funct3 == 0x5) {
                    signals.aluOp = 6;  // Use SRLI (mapped to case 6)
                } else {
                    signals.aluOp = 2;  // Use SLLI (mapped to case 2)
                }
            } 
            // Special handling for bitwise operations
            else if (funct3 == 0x6) {  // ORI
                signals.aluOp = 8;  // Use OR operation (ALU op 8)
            } 
            else if (funct3 == 0x7) {  // ANDI
                signals.aluOp = 9;  // Use AND operation (ALU op 9)
            }
            else if (funct3 == 0x4) {  // XORI
                signals.aluOp = 5;  // Use XOR operation (ALU op 5) 
            }
            // All other I-type instructions use funct3 directly
            else {
                signals.aluOp = funct3;
            }
            break;
            
        case 0x03:  // LOAD
            signals.regWrite = true;
            signals.memRead = true;
            signals.memToReg = true;
            signals.aluSrc = true;
            break;
            
        case 0x23:  // STORE
            signals.memWrite = true;
            signals.aluSrc = true;
            break;
            
        case 0x63:  // BRANCH
            signals.branch = true;
            signals.aluOp = 1;  // Subtraction for comparison
            break;
            
        case 0x6F:  // JAL
            signals.regWrite = true;
            signals.jump = true;
            break;
            
        case 0x67:  // JALR
            signals.regWrite = true;
            signals.jump = true;
            signals.aluSrc = true;
            break;
            
        case 0x37:  // LUI
            signals.regWrite = true;
            signals.aluSrc = true;
            break;
            
        case 0x17:  // AUIPC
            signals.regWrite = true;
            signals.aluSrc = true;
            break;
            
        default:
            std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
            signals.illegal_instruction = true; // Add this flag to ControlSignals
            break;
    }
    return signals;
}

int32_t NoForwardingProcessor::executeALU(int32_t a, int32_t b, uint32_t aluOp) {
    switch (aluOp) {
        case 0:  return a + b;                         // ADD/ADDI
        case 1:  return a - b;                         // SUB
        case 2:  return a << (b & 0x1F);               // SLL/SLLI
        case 3:  return (a < b) ? 1 : 0;               // SLT/SLTIparison) - funct3=2parison)
        case 4:  return (static_cast<uint32_t>(a) < static_cast<uint32_t>(b)) ? 1 : 0; // SLTU/SLTIU/SLTIU (unsigned) - funct3=3/SLTIU (unsigned)
        case 5:  return a ^ b;                         // XOR/XORI
        case 6:  return static_cast<uint32_t>(a) >> (b & 0x1F); // SRL/SRLI
        case 7:  return a >> (b & 0x1F);               // SRA/SRAI
        case 8:  return a | b;                         // OR
        case 9:  return a & b;                         // AND
        
        // M extension instructions (ALU ops 10-17)
        case 10: return a * b;                         // MUL
        case 11: return static_cast<int64_t>(a) * static_cast<int64_t>(b) >> 32; // MULH
        case 12: return static_cast<int64_t>(a) * static_cast<uint64_t>(b) >> 32; // MULHSU
        case 13: return static_cast<uint64_t>(static_cast<uint32_t>(a)) * static_cast<uint64_t>(static_cast<uint32_t>(b)) >> 32; // MULHU
        case 14: return (b == 0) ? -1 : (a / b);      // DIV (handle division by zero)
        case 15: return (b == 0) ? -1 : (static_cast<uint32_t>(a) / static_cast<uint32_t>(b)); // DIVU
        case 16: return (b == 0) ? a : (a % b);       // REM (handle division by zero)
        case 17: return (b == 0) ? a : (static_cast<uint32_t>(a) % static_cast<uint32_t>(b)); // REMU
        
        default: return 0;
    }
}

// ---------------------- Pipeline Matrix Helpers ----------------------
// Record a stage value for an instruction row at a given cycle.
void NoForwardingProcessor::recordStage(int instrIndex, int cycle, PipelineStage stage) {
    if (instrIndex < 0 || instrIndex >= matrixRows || cycle < 0 || cycle >= matrixCols)
        return;
    // Check if there's only one element and it's SPACE
    if ( pipelineMatrix3D[instrIndex][cycle][0] == SPACE ) {
        // Replace the SPACE with the actual stage
        pipelineMatrix3D[instrIndex][cycle][0] = stage;
    } else {
        // Append the new stage to the existing vector
        pipelineMatrix3D[instrIndex][cycle].push_back(stage);
    }
    
}

// Return the index of an instruction correspondin to pc in instructionStrings.
// (Assumes instructions are in program order.)
int NoForwardingProcessor::getInstructionIndex(int32_t index) const {
    if (index < 0 || (index/4) >= static_cast<int32_t>(instructionStrings.size()))
        return -1;
    return static_cast<int>(index / 4);
}

// ---------------------- Register Usage Tracker Functions ----------------------
bool NoForwardingProcessor::isRegisterUsedBy(uint32_t regNum) const {
    // Check if instrIndex exists in the usage list for the register
    if (regUsageTracker[regNum].empty()) 
        return false;
    if(regUsageTracker[regNum].size()==0) 
        return false;
    return true;
}

void NoForwardingProcessor::addRegisterUsage(uint32_t regNum) { 
    // Add the true usage flag to the register's usage list
    regUsageTracker[regNum].push_back(true);
}

void NoForwardingProcessor::clearRegisterUsage(uint32_t regNum) {
    // Remove the instruction index from this register's usage list
    if (!regUsageTracker[regNum].empty())
        regUsageTracker[regNum].pop_back();

}

// ---------------------- Constructor/Destructor ----------------------
NoForwardingProcessor::NoForwardingProcessor() : 
    pc(0), 
    stall(false),
    regUsageTracker(32)  // Initialize register usage tracker with 32 empty vectors
{
    // No need to initialize regInUse array anymore
}

NoForwardingProcessor::~NoForwardingProcessor() {
}

// ---------------------- Instruction Loading ---------------------- 
bool NoForwardingProcessor::loadInstructions(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }
    
    std::string line;
    std::cout << "Loading instructions from " << filename << ":" << std::endl;
    while (std::getline(file, line)) {
        // Trim leading whitespace.
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty())
            continue;
        
        // Ensure the line has at least 8 characters for the hex code.
        if (line.size() < 8) {
            std::cerr << "Warning: line does not contain enough characters for a valid hex code: " << line << std::endl;
            continue;
        }
        
        // Extract the first 8 characters as the hex code.
        std::string hexCode = line.substr(0, 8);
        
        // Extract the rest of the line as the instruction description and trim it.
        std::string instructionDesc;
        if (line.size() > 8) {
            instructionDesc = line.substr(8);
            // Trim leading and trailing whitespace.
            instructionDesc.erase(0, instructionDesc.find_first_not_of(" \t"));
            instructionDesc.erase(instructionDesc.find_last_not_of(" \t") + 1);
        }
        
        // If instruction description is empty, use hexCode.
        if (instructionDesc.empty())
            instructionDesc = hexCode;
        
        std::cout << "  Hex: " << hexCode 
                  << " -> Instruction: " << instructionDesc
                  << std::endl;
                  
        uint32_t instruction = std::stoul(hexCode, nullptr, 16);
        instructionMemory.push_back(instruction);
        instructionStrings.push_back(instructionDesc);
    }
    
    std::cout << "Loaded " << instructionMemory.size() << " instructions. Instruction strings size: " 
              << instructionStrings.size() << std::endl;
    return !instructionMemory.empty();
}

// ---------------------- Hazard Detection ----------------------
bool NoForwardingProcessor::detect_hazard(bool hazard, uint32_t opcode, uint32_t rs1, uint32_t rs2) {
    // Instructions with no source register dependencies
    if (opcode == 0x6F || // JAL
        opcode == 0x37 || // LUI
        opcode == 0x17) { // AUIPC
        // No source registers used, no hazard possible
        hazard = false;
    }
    // Instructions with only rs1 dependency
    else if (opcode == 0x67 || // JALR
                opcode == 0x03 || // LOAD
                opcode == 0x13) { // I-type ALU
        // Only check rs1 for hazard
        hazard = (rs1 != 0 && isRegisterUsedBy(rs1));
        std::cout<<"---------------> x"<<rs1<<" is used by instruction "<<std::endl;
    }
    // Instructions with both rs1 and rs2 dependencies
    else if (opcode == 0x33 || // R-type ALU
                opcode == 0x23 || // STORE
                opcode == 0x63) { // BRANCH
        // Check both rs1 and rs2 for hazards
        hazard = ((rs1 != 0 && isRegisterUsedBy(rs1)) || (rs2 != 0 && isRegisterUsedBy(rs2)));
    }
    return hazard;
}

// ---------------------- Run Simulation ----------------------
void NoForwardingProcessor::run(int cycles) {
    // Reset pipeline state.
    pc = 0;
    stall = false;
    ifid.isEmpty = true;
    idex.isEmpty = true;
    exmem.isEmpty = true;
    memwb.isEmpty = true;
    Imm_valid = true;

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
            if (memwb.controls.regWrite && memwb.rd != 0) {
                int32_t writeData = memwb.controls.memToReg ? memwb.readData : memwb.aluResult;
                registers.write(memwb.rd, writeData);
                clearRegisterUsage(memwb.rd);       
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
            std::cout << "         ALU operation result: " << exmem.aluResult << std::endl;
            
            exmem.pc = idex.pc;
            exmem.readData2 = idex.readData2;
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
            
            // Read register values here for hazard detection and branch computation
            int32_t rs1Value = registers.read(rs1);
            int32_t rs2Value = registers.read(rs2);
            
            // More precise hazard detection based on instruction type
            bool hazard = false;
            
            hazard = detect_hazard(hazard, opcode, rs1, rs2);
            
            //  If no hazards not detected
            if (!hazard) {
                // Calculate branch or jump target in ID stage if applicable
                if (opcode == 0x63 || opcode == 0x67 || opcode == 0x6F) {
                    branchTaken = handleBranchAndJump(opcode, instruction, rs1Value, 
                                                     imm, ifid.pc, rs2Value, branchTarget);
                    if(!Imm_valid){
                        std::cout<<"Invalid Immediate value at PC: "<< ifid.pc <<std::endl;
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
                // Check for illegal instruction
                if(idex.controls.illegal_instruction){
                    std::cout<<"Illegal instruction detected at PC: "<< ifid.pc <<std::endl;
                    std::cout<<"Instruction: "<<ifid.instructionString<<std::endl;
                    std::cout<<"----------------------> Breaking the simulation"<<std::endl;
                    return;
                }
                if (idex.controls.regWrite && rd != 0) {                          
                    addRegisterUsage(rd);
                    std::cout << "         Marking register x" << rd << " as busy "<< " size: "<< regUsageTracker[rd].size() << std::endl;
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

// ---------------------- Print Pipeline Diagram ----------------------
void NoForwardingProcessor::printPipelineDiagram(std::string& filename, bool isforwardcpu) {
    // Create outputfiles directory if it doesn't exist - one level above srcs directory
    std::string outputDir = "../outputfiles";
    
    #ifdef _WIN32
    // Windows-specific directory creation
    system(("mkdir " + outputDir + " 2>nul").c_str());
    #else
    // Linux/Unix directory creation
    system(("mkdir -p " + outputDir).c_str());
    #endif
    
    // Get base filename without directory path
    std::string baseFilename = filename.substr(filename.find_last_of("/\\") + 1);
    // Fix: Properly extract the filename without extension
    size_t lastDotPos = baseFilename.find_last_of('.');
    if (lastDotPos != std::string::npos) {
        baseFilename = baseFilename.substr(0, lastDotPos);
    }
    std::string outputFilename;
    // Output file name will be in outputfiles folder with _no_forward_out.txt appended
    if (!isforwardcpu)
        outputFilename = outputDir + "/" + baseFilename + "_noforward_out.txt";
    else
        outputFilename = outputDir + "/" + baseFilename + "_forward_out.txt";
        
    std::ofstream outFile(outputFilename);
    
    if (!outFile.is_open()) {
        std::cerr << "Error: Unable to open " << outputFilename << " for writing" << std::endl;
        return;
    }
    
    std::cout << "Writing pipeline diagram to " << outputFilename << std::endl;
    
    // Find the longest instruction string to determine column width
    size_t maxInstrLength = 0;
    for (const auto& instr : instructionStrings) {
        std::string cleanInstr = instr;
        cleanInstr.erase(std::remove(cleanInstr.begin(), cleanInstr.end(), '\n'), cleanInstr.end());
        cleanInstr.erase(std::remove(cleanInstr.begin(), cleanInstr.end(), '\r'), cleanInstr.end());
        maxInstrLength = std::max(maxInstrLength, cleanInstr.length());
    }
    
    // Set a minimum width for the instruction column (at least 20 characters)
    const size_t instrColumnWidth = std::max(maxInstrLength, static_cast<size_t>(20));

    // Print header with fixed width
    outFile << std::left << std::setw(instrColumnWidth) << "Instruction";
    for (int i = 0; i < matrixCols; i++) {
        outFile << ";" << i;
    }
    outFile << std::endl;
    
    // For each instruction (row), print the stage per cycle with fixed width
    for (int i = 0; i < matrixRows; i++) {  
        std::string instr = instructionStrings[i];
        // Clean the instruction text
        instr.erase(std::remove(instr.begin(), instr.end(), '\n'), instr.end());
        instr.erase(std::remove(instr.begin(), instr.end(), '\r'), instr.end());
        
        // Print the instruction with fixed width
        outFile << std::left << std::setw(instrColumnWidth) << instr;
        
        PipelineStage prevStage = SPACE;
        // Print each cycle's stage
        for (int j = 0; j < matrixCols; j++) {
            // Get all stages for this instruction in this cycle
            const std::vector<PipelineStage>& stages = pipelineMatrix3D[i][j];
            // Start with a comma for CSV format
            outFile << ";";
            
            // If there are no stages or only SPACE, print empty cell
            if (stages.size() == 1 && stages[0] == SPACE) {
                outFile << "  "; // Print spaces for empty cell
                prevStage = SPACE;
                continue;
            }
            
            // Print first stage and stall if previous stage is same as current stage
            if (stages.size()==1){
                if (stages[0]==prevStage && stages[0]!=SPACE)
                    outFile << "-";
                else
                    outFile << stageToString(stages[0]);
                prevStage = stages[0];
            }
            else{
                // Print stages in reverse format if there are multiple stages
                outFile << stageToString(stages[stages.size()-1]);
                for (int k = stages.size()-2; k >= 0; k--) {
                    outFile<<"/"<<stageToString(stages[k]);
                }
                prevStage = SPACE;
            }
        }
        
        outFile << std::endl;
    }
    outFile.close();
}

// New function to evaluate branch conditions
bool NoForwardingProcessor::evaluateBranchCondition(int32_t rs1Value, int32_t rs2Value, uint32_t funct3) {
    std::cout << "------------------->         Branch condition: " << funct3 << std::endl;
    std::cout << "------------------->         Comparing " << rs1Value << " and " << rs2Value << std::endl;
    
    switch (funct3) {
        case 0x0: return rs1Value == rs2Value;                                          // BEQ
        case 0x1: return rs1Value != rs2Value;                                          // BNE
        case 0x4: return rs1Value < rs2Value;                                           // BLT (signed)
        case 0x5: return rs1Value >= rs2Value;                                          // BGE (signed)
        case 0x6: return static_cast<uint32_t>(rs1Value) < static_cast<uint32_t>(rs2Value);   // BLTU (unsigned)
        case 0x7: return static_cast<uint32_t>(rs1Value) >= static_cast<uint32_t>(rs2Value);  // BGEU (unsigned)
        default:  return false;
    }
}

// Updated to work in ID stage now
bool NoForwardingProcessor::handleBranchAndJump(uint32_t opcode, uint32_t instruction, int32_t rs1Value, 
                                              int32_t imm, int32_t pc, int32_t rs2Value, int32_t& branchTarget) {
    bool branchTaken = false;
    Imm_valid = true;
    // ERROR Check if wrong imm value is being used
    if (imm%4 != 0){
        std::cout << "ERROR: Incorrect immediate value: " << imm << std::endl;
        Imm_valid = false;
        return false;
    }
    if (opcode == 0x63) {  // Branch
        uint32_t funct3 = (instruction >> 12) & 0x7;
        bool condition = evaluateBranchCondition(rs1Value, rs2Value, funct3);
        
        if (condition) {
            branchTaken = true;
            branchTarget = pc + imm;
            std::cout << "         Branch taken to PC: " << branchTarget << std::endl;
        }
        else {
            std::cout << "         Branch not taken" << std::endl;
        }
    }
    else if (opcode == 0x6F) {  // JAL
        branchTaken = true;
        branchTarget = pc + imm;
        std::cout << "         JAL: Jump to PC: " << branchTarget << std::endl;
    }
    else if (opcode == 0x67) {  // JALR
            branchTaken = true;
            branchTarget = (rs1Value + imm) ; // Clear least significant bit per spec
            std::cout << "-> Return register data " << rs1Value << "         JALR: Jump to PC: " << branchTarget << std::endl;
    }
    
    return branchTaken;
}