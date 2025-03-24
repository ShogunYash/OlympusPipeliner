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
    
    // Default signals.
    signals.regWrite = false;
    signals.memRead = false;
    signals.memWrite = false;
    signals.memToReg = false;
    signals.aluSrc = false;
    signals.branch = false;
    signals.jump = false;
    signals.aluOp = 0;
    
    switch (opcode) {
        case 0x33:  // R-type
            signals.regWrite = true;
            // NOTE: your original decoding of aluOp is preserved.
            signals.aluOp = ((instruction >> 12) & 0x7) | (((instruction >> 25) & 0x7F) ? 0x8 : 0);
            break;
        case 0x13:  // I-type
            signals.regWrite = true;
            signals.aluSrc = true;
            signals.aluOp = (instruction >> 12) & 0x7;
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
            signals.aluOp = 1;  // Subtraction for comparison.
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
            break;
    }
    return signals;
}

int32_t NoForwardingProcessor::executeALU(int32_t a, int32_t b, uint32_t aluOp) {
    switch (aluOp) {
        case 0:  return a + b;                         // ADD
        case 1:  return a - b;                         // SUB
        case 2:  return a << (b & 0x1F);               // SLL
        case 3:  return (a < b) ? 1 : 0;               // SLT (already signed)
        case 4:  return (static_cast<uint32_t>(a) < static_cast<uint32_t>(b)) ? 1 : 0; // SLTU
        case 5:  return a ^ b;                         // XOR
        case 6:  return static_cast<uint32_t>(a) >> (b & 0x1F); // SRL (logical shift)
        case 7:  return a >> (b & 0x1F);               // SRA (arithmetic shift, already signed)
        case 8:  return a | b;                         // OR
        case 9:  return a & b;                         // AND
        default: return 0;
    }
}

// ---------------------- Pipeline Matrix Helpers ----------------------

// Record a stage value for an instruction row at a given cycle.
void NoForwardingProcessor::recordStage(int instrIndex, int cycle, PipelineStage stage) {
    if (instrIndex < 0 || instrIndex >= matrixRows || cycle < 0 || cycle >= matrixCols)
        return;
    pipelineMatrix[instrIndex][cycle] = stage;
}

// Return the index of an instruction correspondin to pc in instructionStrings.
// (Assumes instructions are in program order.)
int NoForwardingProcessor::getInstructionIndex(int32_t index) const {
    if (index < 0 || (index/4) >= static_cast<int32_t>(instructionStrings.size()))
        return -1;
    return static_cast<int>(index / 4);
}

// Free the dynamically allocated pipeline matrix.
void NoForwardingProcessor::freePipelineMatrix() {
    if (pipelineMatrix) {
        for (int i = 0; i < matrixRows; i++) {
            free(pipelineMatrix[i]);
        }
        free(pipelineMatrix);
        pipelineMatrix = nullptr;
    }
}

// ---------------------- Register Usage Tracker Functions ----------------------

bool NoForwardingProcessor::isRegisterUsedBy(uint32_t regNum) const {
    // Check if instrIndex exists in the usage list for the register
    if (regUsageTracker[regNum].empty()) return false;
    return true;
}

void NoForwardingProcessor::addRegisterUsage(uint32_t regNum) { 
    // Add the true usage flag to the register's usage list
    regUsageTracker[regNum].push_back(true);
}

void NoForwardingProcessor::clearRegisterUsage(uint32_t regNum) {
    // Remove the instruction index from this register's usage list
    regUsageTracker[regNum].pop_back();
}

// ---------------------- Constructor/Destructor ----------------------

NoForwardingProcessor::NoForwardingProcessor() : 
    pc(0), 
    pipelineMatrix(nullptr),  // Initialize pipelineMatrix first
    stall(false),              // Now initialize stall
// Initialize register usage tracker with 32 empty vectors
    regUsageTracker(32)
{
    std::fill(std::begin(regInUse), std::end(regInUse), false);
}

NoForwardingProcessor::~NoForwardingProcessor() {
    freePipelineMatrix();
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
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos)
            continue;
        
        std::istringstream iss(line);
        std::string hexCode, instructionDesc;
        std::getline(iss, hexCode, ';');
        std::getline(iss, instructionDesc);
        hexCode.erase(0, hexCode.find_first_not_of(" \t"));
        hexCode.erase(hexCode.find_last_not_of(" \t") + 1);
        instructionDesc.erase(0, instructionDesc.find_first_not_of(" \t"));
        instructionDesc.erase(instructionDesc.find_last_not_of(" \t") + 1);
        if (hexCode.empty())
            continue;
        std::cout << "  Hex: " << hexCode 
                  << " -> Instruction: " << (instructionDesc.empty() ? hexCode : instructionDesc)
                  << std::endl;
        uint32_t instruction = std::stoul(hexCode, nullptr, 16);
        instructionMemory.push_back(instruction);
        instructionStrings.push_back(instructionDesc.empty() ? hexCode : instructionDesc);
    }
    std::cout << "Loaded " << instructionMemory.size() << " instructions."<< " Instruction strings size: "<<instructionStrings.size() << std::endl;
    return !instructionMemory.empty();
}

// ---------------------- Run Simulation ----------------------

void NoForwardingProcessor::run(int cycles) {
    // Reset pipeline state.
    pc = 0;
    stall = false;
    ifid.isEmpty = true;
    ifid.isStalled = false;
    idex.isEmpty = true;
    idex.isStalled = false;
    exmem.isEmpty = true;
    exmem.isStalled = false;
    memwb.isEmpty = true;
    
    // Allocate the pipeline matrix.
    matrixRows = static_cast<int>(instructionStrings.size());
    matrixCols = cycles;
    pipelineMatrix = (PipelineStage**)malloc(matrixRows * sizeof(PipelineStage*));
    for (int i = 0; i < matrixRows; i++) {
        pipelineMatrix[i] = (PipelineStage*)malloc(matrixCols * sizeof(PipelineStage));
        memset(pipelineMatrix[i], SPACE, matrixCols * sizeof(PipelineStage)); // initialize all cells to STALL marker
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
            int idx = getInstructionIndex(idex.pc);
            if (idx != -1)
                recordStage(idx, cycle, EX);
            int32_t aluOp1 = idex.readData1;  // Changed to signed 32-bit
            int32_t aluOp2 = idex.controls.aluSrc ? idex.imm : idex.readData2;  // Changed to signed 32-bit
            uint32_t opcode = idex.instruction & 0x7F;
            if(opcode != 0x67) // JALR
                exmem.aluResult = executeALU(aluOp1, aluOp2, idex.controls.aluOp);
            else 
                exmem.aluResult = idex.pc + 4;    // PC+4 for JALR
            std::cout << "         ALU operation result: " << exmem.aluResult << std::endl;
            if (opcode == 0x63) {  // Branch
                bool condition = false;
                uint32_t funct3 = (idex.instruction >> 12) & 0x7;
                switch (funct3) {
                    case 0x0: condition = (idex.readData1 == idex.readData2); break;
                    case 0x1: condition = (idex.readData1 != idex.readData2); break;
                    case 0x4: condition = (idex.readData1 < idex.readData2); break;  // Already signed
                    case 0x5: condition = (idex.readData1 >= idex.readData2); break; // Already signed
                    case 0x6: condition = (static_cast<uint32_t>(idex.readData1) < static_cast<uint32_t>(idex.readData2)); break;  // Unsigned comparison
                    case 0x7: condition = (static_cast<uint32_t>(idex.readData1) >= static_cast<uint32_t>(idex.readData2)); break; // Unsigned comparison
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
                branchTarget = (idex.readData1 + idex.imm) & ~1;
                std::cout <<"-> Return register data "<<idex.readData1 <<"         JALR: Jump to PC: " << branchTarget << std::endl;
            }
            else if (opcode == 0x17){ // AUIPC
                exmem.aluResult = idex.pc + idex.imm;
                std::cout << "         AUIPC: Jump to PC: " << exmem.aluResult << std::endl;
            }
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
            int32_t imm = extractImmediate(instruction, opcode);  // Changed to signed 32-bit
            
            // More precise hazard detection based on instruction type
            bool hazard = false;
            
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
            }
            // Instructions with both rs1 and rs2 dependencies
            else if (opcode == 0x33 || // R-type ALU
                     opcode == 0x23 || // STORE
                     opcode == 0x63) { // BRANCH
                // Check both rs1 and rs2 for hazards
                hazard = ((rs1 != 0 && isRegisterUsedBy(rs1)) || (rs2 != 0 && isRegisterUsedBy(rs2)));
            }

            if (!hazard) {
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
                if (idex.controls.regWrite && rd != 0) {
                    addRegisterUsage(rd);
                    std::cout << "         Marking register x" << rd << " as busy "<< " size: "<< regUsageTracker[rd].size() << std::endl;
                }
            }
            else {
                stall = true;
                ifid.isStalled = true;
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
            ifid.isStalled = false;
            int idx = getInstructionIndex(ifid.pc);
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
            // Make registers in use available for writing of ID stage ones.
            clearRegisterUsage(idex.rd);
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

// ---------------------- Print Pipeline Diagram ----------------------

void NoForwardingProcessor::printPipelineDiagram() {
    std::ofstream outFile("output.csv");
    if (!outFile.is_open()) {
        std::cerr << "Error: Unable to open output.txt for writing" << std::endl;
        return;
    }
    
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
        outFile << "," << i;
    }
    outFile << "," << std::endl;
    
    // For each instruction (row), print the stage per cycle with fixed width
    for (int i = 0; i < matrixRows; i++) {  
        std::string instr = instructionStrings[i];
        // Clean the instruction text
        instr.erase(std::remove(instr.begin(), instr.end(), '\n'), instr.end());
        instr.erase(std::remove(instr.begin(), instr.end(), '\r'), instr.end());
        
        // Print the instruction with fixed width
        outFile << std::left << std::setw(instrColumnWidth) << instr;
        
        // Print each cycle's stage
        for (int j = 0; j < matrixCols; j++) {
            outFile << "," << stageToString(pipelineMatrix[i][j]);
        }
        outFile << "," << std::endl;
    }
    outFile.close();
}
