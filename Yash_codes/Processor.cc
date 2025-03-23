#include "Processor.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

// Constructor: Initialize PC, stall flag, and the register-in-use array.
NoForwardingProcessor::NoForwardingProcessor() : pc(0), stall(false) {
    std::fill(std::begin(regInUse), std::end(regInUse), false);
}

bool NoForwardingProcessor::loadInstructions(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }
    
    std::string line;
    std::cout << "Loading instructions from " << filename << ":" << std::endl;
    
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }
        
        std::istringstream iss(line);
        std::string hexCode, instructionDesc;
        
        // Parse the line with format: hexcode;assembly_code
        std::getline(iss, hexCode, ';');
        std::getline(iss, instructionDesc);
        
        // Trim whitespace
        hexCode.erase(0, hexCode.find_first_not_of(" \t"));
        hexCode.erase(hexCode.find_last_not_of(" \t") + 1);
        instructionDesc.erase(0, instructionDesc.find_first_not_of(" \t"));
        instructionDesc.erase(instructionDesc.find_last_not_of(" \t") + 1);
        
        if (hexCode.empty()) {
            continue;
        }
        
        // Debug output: print hex code and instruction text
        std::cout << "  Hex: " << hexCode << " -> Instruction: " << instructionDesc << std::endl;
        
        // Convert hex string to uint32_t
        uint32_t instruction = std::stoul(hexCode, nullptr, 16);
        
        instructionMemory.push_back(instruction);
        // Save assembly string if available; otherwise save the hex code.
        instructionStrings.push_back(instructionDesc.empty() ? hexCode : instructionDesc);
    }
    
    std::cout << "Loaded " << instructionMemory.size() << " instructions." << std::endl;
    return !instructionMemory.empty();
}

ControlSignals NoForwardingProcessor::decodeControlSignals(uint32_t instruction) {
    ControlSignals signals;
    uint32_t opcode = instruction & 0x7F;
    
    // Default signals
    signals.regWrite = false;
    signals.memRead = false;
    signals.memWrite = false;
    signals.memToReg = false;
    signals.aluSrc = false;
    signals.branch = false;
    signals.jump = false;
    signals.aluOp = 0;
    
    // Decode based on opcode
    switch (opcode) {
        case 0x33:  // R-type
            signals.regWrite = true;
            signals.aluOp = ((instruction >> 12) & 0x7) | (((instruction >> 25) & 0x7F) ? 0x8 : 0);
            break;
        case 0x13:  // I-type (ADDI, etc.)
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
            signals.aluOp = 0x1;  // Subtraction for comparison
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
    }
    
    return signals;
}

uint32_t NoForwardingProcessor::executeALU(uint32_t a, uint32_t b, uint32_t aluOp) {
    switch (aluOp) {
        case 0:  // ADD
            return a + b;
        case 1:  // SUB
            return a - b;
        case 2:  // SLL
            return a << (b & 0x1F);
        case 3:  // SLT
            return static_cast<int32_t>(a) < static_cast<int32_t>(b) ? 1 : 0;
        case 4:  // SLTU
            return a < b ? 1 : 0;
        case 5:  // XOR
            return a ^ b;
        case 6:  // SRL
            return a >> (b & 0x1F);
        case 7:  // SRA
            return static_cast<int32_t>(a) >> (b & 0x1F);
        case 8:  // OR
            return a | b;
        case 9:  // AND
            return a & b;
        default:
            return 0;
    }
}

uint32_t NoForwardingProcessor::extractImmediate(uint32_t instruction, uint32_t opcode) {
    uint32_t imm = 0;
    
    // I-type
    if (opcode == 0x13 || opcode == 0x03 || opcode == 0x67) {
        imm = instruction >> 20;
        // Sign extend
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

void NoForwardingProcessor::updatePipelineHistory(int cycle) {
    // Convert cycle to unsigned for vector size comparisons
    size_t unsignedCycle = static_cast<size_t>(cycle);
    
    // Update pipeline history for each active instruction in each stage
    if (!ifid.isEmpty) {
        if (pipelineHistory.find(ifid.instructionString) == pipelineHistory.end()) {
            pipelineHistory[ifid.instructionString] = std::vector<std::string>(unsignedCycle + 1, " ");
        }
        if (pipelineHistory[ifid.instructionString].size() <= unsignedCycle) {
            pipelineHistory[ifid.instructionString].resize(unsignedCycle + 1, " ");
        }
        pipelineHistory[ifid.instructionString][unsignedCycle] = ifid.isStalled ? "-" : "IF";
    }
    
    if (!idex.isEmpty) {
        if (pipelineHistory.find(idex.instructionString) == pipelineHistory.end()) {
            pipelineHistory[idex.instructionString] = std::vector<std::string>(unsignedCycle + 1, " ");
        }
        if (pipelineHistory[idex.instructionString].size() <= unsignedCycle) {
            pipelineHistory[idex.instructionString].resize(unsignedCycle + 1, " ");
        }
        pipelineHistory[idex.instructionString][unsignedCycle] = idex.isStalled ? "-" : "ID";
    }
    
    if (!exmem.isEmpty) {
        if (pipelineHistory.find(exmem.instructionString) == pipelineHistory.end()) {
            pipelineHistory[exmem.instructionString] = std::vector<std::string>(unsignedCycle + 1, " ");
        }
        if (pipelineHistory[exmem.instructionString].size() <= unsignedCycle) {
            pipelineHistory[exmem.instructionString].resize(unsignedCycle + 1, " ");
        }
        pipelineHistory[exmem.instructionString][unsignedCycle] = exmem.isStalled ? "-" : "EX";
    }
    
    if (!memwb.isEmpty) {
        if (pipelineHistory.find(memwb.instructionString) == pipelineHistory.end()) {
            pipelineHistory[memwb.instructionString] = std::vector<std::string>(unsignedCycle + 1, " ");
        }
        if (pipelineHistory[memwb.instructionString].size() <= unsignedCycle) {
            pipelineHistory[memwb.instructionString].resize(unsignedCycle + 1, " ");
        }
        pipelineHistory[memwb.instructionString][unsignedCycle] = "MEM";
    }
    
    // Update WB stage for any instruction that was in MEM in previous cycle
    for (auto& pair : pipelineHistory) {
        if (pair.second.size() <= unsignedCycle) {
            pair.second.resize(unsignedCycle + 1, " ");
        }
        if (cycle > 0 && pair.second.size() > static_cast<size_t>(cycle - 1) && pair.second[cycle - 1] == "MEM") {
            pair.second[unsignedCycle] = "WB";
        }
    }
    
    // Mark stalls for the IF stage if a stall condition exists
    if (stall) {
        for (auto& pair : pipelineHistory) {
            if (pair.second.size() > unsignedCycle && pair.second[unsignedCycle] == "IF") {
                pair.second[unsignedCycle] = "-";
                break;  // Only mark one instruction as stalled.
            }
        }
    }
}


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
    pipelineHistory.clear();
    
    for (int cycle = 0; cycle < cycles; cycle++) {
        std::cout << "========== Starting Cycle " << cycle << " ==========" << std::endl;
        
        // WB Stage
        if (!memwb.isEmpty) {
            std::cout << "Cycle " << cycle << " - WB: Processing " 
                      << memwb.instructionString << " at PC: " << memwb.pc << std::endl;
            if (memwb.controls.regWrite && memwb.rd != 0) {
                uint32_t writeData = memwb.controls.memToReg ? memwb.readData : memwb.aluResult;
                registers.write(memwb.rd, writeData);
                // Clear the register-in-use flag since WB completes the write.
                regInUse[memwb.rd] = false;
                std::cout << "         Written " << writeData << " to register x" << memwb.rd << std::endl;
            }
        } else {
            std::cout << "Cycle " << cycle << " - WB: No instruction" << std::endl;
        }
        
        // MEM Stage
        if (!exmem.isEmpty) {
            std::cout << "Cycle " << cycle << " - MEM: Processing " 
                      << exmem.instructionString << " at PC: " << exmem.pc << std::endl;
            if (exmem.controls.memRead) {
                memwb.readData = dataMemory.readWord(exmem.aluResult);
                std::cout << "         Read from memory at address " << exmem.aluResult << " data: " 
                          << memwb.readData << std::endl;
            }
            if (exmem.controls.memWrite) {
                dataMemory.writeWord(exmem.aluResult, exmem.readData2);
                std::cout << "         Wrote " << exmem.readData2 << " to memory at address " << exmem.aluResult << std::endl;
            }
            
            // Propagate to WB stage, including raw machine code.
            memwb.pc = exmem.pc;
            memwb.aluResult = exmem.aluResult;
            memwb.rd = exmem.rd;
            memwb.controls = exmem.controls;
            memwb.instruction = exmem.instruction;             // Propagate machine code.
            memwb.instructionString = exmem.instructionString;   // Propagate assembly string.
            memwb.isEmpty = false;
        } else {
            memwb.isEmpty = true;
            std::cout << "Cycle " << cycle << " - MEM: No instruction" << std::endl;
        }
        
        // EX Stage
        if (!idex.isEmpty) {
            std::cout << "Cycle " << cycle << " - EX: Processing " 
                      << idex.instructionString << " at PC: " << idex.pc << std::endl;
            uint32_t aluOp1 = idex.readData1;
            uint32_t aluOp2 = idex.controls.aluSrc ? idex.imm : idex.readData2;
            
            exmem.aluResult = executeALU(aluOp1, aluOp2, idex.controls.aluOp);
            std::cout << "         ALU operation result: " << exmem.aluResult << std::endl;
            
            // Propagate to MEM stage, including raw machine code.
            exmem.pc = idex.pc;
            exmem.readData2 = idex.readData2;
            exmem.rd = idex.rd;
            exmem.controls = idex.controls;
            exmem.instruction = idex.instruction;             // Propagate machine code.
            exmem.instructionString = idex.instructionString;   // Propagate assembly string.
            exmem.isEmpty = false;
        } else {
            exmem.isEmpty = true;
            std::cout << "Cycle " << cycle << " - EX: No instruction" << std::endl;
        }
        
        // ID Stage
        if (!ifid.isEmpty) {
            std::cout << "Cycle " << cycle << " - ID: Processing " 
                      << ifid.instructionString << " at PC: " << ifid.pc << std::endl;
            uint32_t instruction = ifid.instruction;
            
            // Decode instruction fields.
            uint32_t opcode = instruction & 0x7F;
            uint32_t rd = (instruction >> 7) & 0x1F;
            uint32_t rs1 = (instruction >> 15) & 0x1F;
            uint32_t rs2 = (instruction >> 20) & 0x1F;
            
            // Extract immediate value.
            uint32_t imm = extractImmediate(instruction, opcode);
            
            // Hazard detection.
            bool hazard = false;
            // Check register-in-use flags first.
            if ((rs1 != 0 && regInUse[rs1]) || (rs2 != 0 && regInUse[rs2])) {
                hazard = true;
                std::cout << "         Hazard detected: Source register busy" << std::endl;
            }
            
            // Handle control hazards for branch/jump.
            bool takeJump = false;
            uint32_t jumpTarget = 0;
            
            if (opcode == 0x63) {  // Branch
                uint32_t rs1Value = registers.read(rs1);
                uint32_t rs2Value = registers.read(rs2);
                bool condition = false;
                uint32_t funct3 = (instruction >> 12) & 0x7;
                
                switch (funct3) {
                    case 0x0:  // BEQ
                        condition = (rs1Value == rs2Value);
                        break;
                    case 0x1:  // BNE
                        condition = (rs1Value != rs2Value);
                        break;
                    case 0x4:  // BLT
                        condition = (static_cast<int32_t>(rs1Value) < static_cast<int32_t>(rs2Value));
                        break;
                    case 0x5:  // BGE
                        condition = (static_cast<int32_t>(rs1Value) >= static_cast<int32_t>(rs2Value));
                        break;
                    case 0x6:  // BLTU
                        condition = (rs1Value < rs2Value);
                        break;
                    case 0x7:  // BGEU
                        condition = (rs1Value >= rs2Value);
                        break;
                }
                
                if (condition) {
                    takeJump = true;
                    jumpTarget = ifid.pc + imm;
                    std::cout << "         Branch taken to PC: " << jumpTarget << std::endl;
                }
            }
            else if (opcode == 0x6F) {  // JAL
                takeJump = true;
                jumpTarget = ifid.pc + imm;
                std::cout << "         JAL: Jump to PC: " << jumpTarget << std::endl;
            }
            else if (opcode == 0x67) {  // JALR
                takeJump = true;
                jumpTarget = (registers.read(rs1) + imm) & ~1;
                std::cout << "         JALR: Jump to PC: " << jumpTarget << std::endl;
            }
            
            if (!hazard) {
                // No hazard: Read registers and set up ID/EX stage.
                idex.readData1 = registers.read(rs1);
                idex.readData2 = registers.read(rs2);
                idex.pc = ifid.pc;
                idex.imm = imm;
                idex.rs1 = rs1;
                idex.rs2 = rs2;
                idex.rd = rd;
                idex.controls = decodeControlSignals(instruction);
                // Propagate raw machine code and assembly string.
                idex.instruction = ifid.instruction;
                idex.instructionString = ifid.instructionString;
                idex.isEmpty = false;
                idex.isStalled = false;
                
                // Mark destination register as busy in the register-in-use array.
                if (idex.controls.regWrite && rd != 0) {
                    regInUse[rd] = true;
                    std::cout << "         Marking register x" << rd << " as busy" << std::endl;
                }
                
                // Handle branch/jump.
                if (takeJump) {
                    pc = jumpTarget;
                    ifid.isEmpty = true;  // Flush IF/ID.
                    std::cout << "         Flushing IF stage due to jump/branch" << std::endl;
                } else {
                    pc += 4;  // Advance PC normally.
                }
            } else {
                // Hazard detected: stall the pipeline.
                stall = true;
                ifid.isStalled = true;  // Mark IF/ID as stalled.
                idex.isEmpty = true;
                std::cout << "         Stalling pipeline due to hazard" << std::endl;
            }
        } else {
            idex.isEmpty = true;
            std::cout << "Cycle " << cycle << " - ID: No instruction" << std::endl;
        }
        
        // IF Stage
        if (!stall && (pc / 4 < instructionMemory.size())) {
            ifid.instruction = instructionMemory[pc / 4];
            ifid.pc = pc;
            ifid.instructionString = instructionStrings[pc / 4];
            ifid.isEmpty = false;
            ifid.isStalled = false;
            std::cout << "Cycle " << cycle << " - IF: Fetched " 
                      << ifid.instructionString << " at PC: " << pc << std::endl;
            pc += 4;  // Advance PC normally.
        } else if (stall) {
            // In case of stall, keep current instruction in IF stage (PC remains unchanged).
            std::cout << "Cycle " << cycle << " - IF: Stall in effect, instruction remains same" << std::endl;
        } else {
            ifid.isEmpty = true;
            ifid.isStalled = false;
            std::cout << "Cycle " << cycle << " - IF: No instruction fetched" << std::endl;
        }
        
        // Update pipeline history for visualization.
        updatePipelineHistory(cycle);
        
        // Reset stall flag for next cycle.
        if (stall) {
            stall = false;
            ifid.isStalled = false;
        }
        
        std::cout << "========== Ending Cycle " << cycle << " ==========" << std::endl << std::endl;
    }
}

// void NoForwardingProcessor::run(int cycles) {
//     // Reset pipeline state.
//     pc = 0;
//     stall = false;
//     ifid.isEmpty = true;
//     ifid.isStalled = false;
//     idex.isEmpty = true;
//     idex.isStalled = false;
//     exmem.isEmpty = true;
//     exmem.isStalled = false;
//     memwb.isEmpty = true;
//     pipelineHistory.clear();
    
//     for (int cycle = 0; cycle < cycles; cycle++) {
// std::cout << "========== Starting Cycle " << cycle << " ==========" << std::endl;
        
//         // WB Stage
//         if (!memwb.isEmpty) {
// std::cout << "Cycle " << cycle << " - WB: Processing " << memwb.instructionString << std::endl;
//             if (memwb.controls.regWrite && memwb.rd != 0) {
//                 uint32_t writeData = memwb.controls.memToReg ? memwb.readData : memwb.aluResult;
//                 registers.write(memwb.rd, writeData);
//                 // Clear the register-in-use flag since WB completes the write.
//                 regInUse[memwb.rd] = false;
//             }
//         }
        
//         // MEM Stage
//         if (!exmem.isEmpty) {
//             if (exmem.controls.memRead) {
//                 memwb.readData = dataMemory.readWord(exmem.aluResult);
//             }
//             if (exmem.controls.memWrite) {
//                 dataMemory.writeWord(exmem.aluResult, exmem.readData2);
//             }
            
//             // Propagate to WB stage, including raw machine code.
//             memwb.pc = exmem.pc;
//             memwb.aluResult = exmem.aluResult;
//             memwb.rd = exmem.rd;
//             memwb.controls = exmem.controls;
//             memwb.instruction = exmem.instruction;             // Propagate machine code.
//             memwb.instructionString = exmem.instructionString;   // Propagate assembly string.
//             memwb.isEmpty = false;
//         } else {
//             memwb.isEmpty = true;
//         }
        
//         // EX Stage
//         if (!idex.isEmpty) {
//             uint32_t aluOp1 = idex.readData1;
//             uint32_t aluOp2 = idex.controls.aluSrc ? idex.imm : idex.readData2;
            
//             exmem.aluResult = executeALU(aluOp1, aluOp2, idex.controls.aluOp);
            
//             // Propagate to MEM stage, including raw machine code.
//             exmem.pc = idex.pc;
//             exmem.readData2 = idex.readData2;
//             exmem.rd = idex.rd;
//             exmem.controls = idex.controls;
//             exmem.instruction = idex.instruction;             // Propagate machine code.
//             exmem.instructionString = idex.instructionString;   // Propagate assembly string.
//             exmem.isEmpty = false;
//         } else {
//             exmem.isEmpty = true;
//         }
        
//         // ID Stage
//         if (!ifid.isEmpty) {
//             uint32_t instruction = ifid.instruction;
            
//             // Decode instruction fields.
//             uint32_t opcode = instruction & 0x7F;
//             uint32_t rd = (instruction >> 7) & 0x1F;
//             uint32_t rs1 = (instruction >> 15) & 0x1F;
//             uint32_t rs2 = (instruction >> 20) & 0x1F;
            
//             // Extract immediate value.
//             uint32_t imm = extractImmediate(instruction, opcode);
            
//             // Hazard detection.
//             bool hazard = false;
//             // Check register-in-use flags first.
//             if ((rs1 != 0 && regInUse[rs1]) || (rs2 != 0 && regInUse[rs2])) {
//                 hazard = true;
//             }
            
//             // Handle control hazards for branch/jump.
//             bool takeJump = false;
//             uint32_t jumpTarget = 0;
            
//             if (opcode == 0x63) {  // Branch
//                 uint32_t rs1Value = registers.read(rs1);
//                 uint32_t rs2Value = registers.read(rs2);
//                 bool condition = false;
//                 uint32_t funct3 = (instruction >> 12) & 0x7;
                
//                 switch (funct3) {
//                     case 0x0:  // BEQ
//                         condition = (rs1Value == rs2Value);
//                         break;
//                     case 0x1:  // BNE
//                         condition = (rs1Value != rs2Value);
//                         break;
//                     case 0x4:  // BLT
//                         condition = (static_cast<int32_t>(rs1Value) < static_cast<int32_t>(rs2Value));
//                         break;
//                     case 0x5:  // BGE
//                         condition = (static_cast<int32_t>(rs1Value) >= static_cast<int32_t>(rs2Value));
//                         break;
//                     case 0x6:  // BLTU
//                         condition = (rs1Value < rs2Value);
//                         break;
//                     case 0x7:  // BGEU
//                         condition = (rs1Value >= rs2Value);
//                         break;
//                 }
                
//                 if (condition) {
//                     takeJump = true;
//                     jumpTarget = ifid.pc + imm;
//                 }
//             }
//             else if (opcode == 0x6F) {  // JAL
//                 takeJump = true;
//                 jumpTarget = ifid.pc + imm;
//             }
//             else if (opcode == 0x67) {  // JALR
//                 takeJump = true;
//                 jumpTarget = (registers.read(rs1) + imm) & ~1;
//             }
            
//             if (!hazard) {
//                 // No hazard: Read registers and set up ID/EX stage.
//                 idex.readData1 = registers.read(rs1);
//                 idex.readData2 = registers.read(rs2);
//                 idex.pc = ifid.pc;
//                 idex.imm = imm;
//                 idex.rs1 = rs1;
//                 idex.rs2 = rs2;
//                 idex.rd = rd;
//                 idex.controls = decodeControlSignals(instruction);
//                 // Propagate raw machine code and assembly string.
//                 idex.instruction = ifid.instruction;
//                 idex.instructionString = ifid.instructionString;
//                 idex.isEmpty = false;
//                 idex.isStalled = false;
                
//                 // Mark destination register as busy in the register-in-use array.
//                 if (idex.controls.regWrite && rd != 0) {
//                     regInUse[rd] = true;
//                 }
                
//                 // Handle branch/jump.
//                 if (takeJump) {
//                     pc = jumpTarget;
//                     ifid.isEmpty = true;  // Flush IF/ID.
//                 } else {
//                     pc += 4;  // Advance PC normally.
//                 }
//             } else {
//                 // Hazard detected: stall the pipeline.
//                 stall = true;
//                 ifid.isStalled = true;  // Mark IF/ID as stalled.
//                 idex.isEmpty = true;
//             }
//         } else {
//             idex.isEmpty = true;
//         }
        
//         // IF Stage
//         if (!stall && (pc / 4 < instructionMemory.size())) {
//             ifid.instruction = instructionMemory[pc / 4];
//             ifid.pc = pc;
//             ifid.instructionString = instructionStrings[pc / 4];
//             ifid.isEmpty = false;
//             ifid.isStalled = false;
//             pc += 4;  // Advance PC normally.
//         } else if (stall) {
//             // In case of stall, keep current instruction in IF stage (PC remains unchanged).
//             pc = pc;
//         } else {
//             ifid.isEmpty = true;
//             ifid.isStalled = false;
//         }
        
//         // Update pipeline history for visualization.
//         updatePipelineHistory(cycle);
        
//         // Reset stall flag for next cycle.
//         if (stall) {
//             stall = false;
//             ifid.isStalled = false;
//         }
//     }
// }

void NoForwardingProcessor::printPipelineDiagram() {
    // Open output file "output.txt"
    std::ofstream outFile("output.txt");
    if (!outFile.is_open()) {
        std::cerr << "Error: Unable to open output.txt for writing" << std::endl;
        return;
    }
    
    // Print table header with semicolons as separators.
    outFile << "Instruction";
    for (size_t i = 0; i < 15; i++) { // Assuming maximum 15 cycles.
        outFile << ";" << i;
    }
    outFile << ";" << std::endl;
    
    // Print each instruction with its pipeline stages in program order.
    for (size_t i = 0; i < instructionStrings.size(); i++) {
        const std::string& instr = instructionStrings[i];
        
        // Remove any newline or carriage return characters.
        std::string cleanInstr = instr;
        cleanInstr.erase(std::remove(cleanInstr.begin(), cleanInstr.end(), '\n'), cleanInstr.end());
        cleanInstr.erase(std::remove(cleanInstr.begin(), cleanInstr.end(), '\r'), cleanInstr.end());
        
        // Print instruction name.
        outFile << cleanInstr;
        
        // Print each stage for this instruction using semicolons.
        if (pipelineHistory.find(instr) != pipelineHistory.end()) {
            for (size_t j = 0; j < pipelineHistory[instr].size(); j++) {
                outFile << ";" << pipelineHistory[instr][j];
            }
            // Fill remaining columns with empty cells.
            for (size_t j = pipelineHistory[instr].size(); j < 15; j++) {
                outFile << ";";
            }
        } else {
            // If the instruction was never executed, print empty cells for all cycles.
            for (size_t j = 0; j < 15; j++) {
                outFile << ";";
            }
        }
        
        outFile << ";" << std::endl;
    }
    
    outFile.close();
}


// #include "Processor.hpp"
// #include <iostream>
// #include <fstream>
// #include <sstream>
// #include <iomanip>
// #include <algorithm>

// NoForwardingProcessor::NoForwardingProcessor() : pc(0), stall(false) {}

// bool NoForwardingProcessor::loadInstructions(const std::string& filename) {
//     std::ifstream file(filename);
//     if (!file.is_open()) {
//         std::cerr << "Error: Cannot open file " << filename << std::endl;
//         return false;
//     }
    
//     std::string line;
    
//     while (std::getline(file, line)) {
//         // Skip empty lines
//         if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
//             continue;
//         }
        
//         std::istringstream iss(line);
//         std::string hexCode, instructionDesc;
        
//         // Parse the line format which has format: hexcode;assembly_code
//         std::getline(iss, hexCode, ';');
//         std::getline(iss, instructionDesc);
        
//         // Trim whitespace
//         hexCode.erase(0, hexCode.find_first_not_of(" \t"));
//         hexCode.erase(hexCode.find_last_not_of(" \t") + 1);
        
//         instructionDesc.erase(0, instructionDesc.find_first_not_of(" \t"));
//         instructionDesc.erase(instructionDesc.find_last_not_of(" \t") + 1);
        
//         if (hexCode.empty()) {
//             continue;
//         }
        
//         // Convert hex string to uint32_t
//         uint32_t instruction = std::stoul(hexCode, nullptr, 16);
        
//         instructionMemory.push_back(instruction);
//         instructionStrings.push_back(instructionDesc.empty() ? hexCode : instructionDesc);
//     }
    
//     return !instructionMemory.empty();
// }

// ControlSignals NoForwardingProcessor::decodeControlSignals(uint32_t instruction) {
//     ControlSignals signals;
//     uint32_t opcode = instruction & 0x7F;
    
//     // Default signals
//     signals.regWrite = false;
//     signals.memRead = false;
//     signals.memWrite = false;
//     signals.memToReg = false;
//     signals.aluSrc = false;
//     signals.branch = false;
//     signals.jump = false;
//     signals.aluOp = 0;
    
//     // Decode based on opcode
//     switch (opcode) {
//         case 0x33:  // R-type
//             signals.regWrite = true;
//             signals.aluOp = ((instruction >> 12) & 0x7) | (((instruction >> 25) & 0x7F) ? 0x8 : 0);
//             break;
        
//         case 0x13:  // I-type (ADDI, etc.)
//             signals.regWrite = true;
//             signals.aluSrc = true;
//             signals.aluOp = (instruction >> 12) & 0x7;
//             break;
        
//         case 0x03:  // LOAD
//             signals.regWrite = true;
//             signals.memRead = true;
//             signals.memToReg = true;
//             signals.aluSrc = true;
//             break;
        
//         case 0x23:  // STORE
//             signals.memWrite = true;
//             signals.aluSrc = true;
//             break;
        
//         case 0x63:  // BRANCH
//             signals.branch = true;
//             signals.aluOp = 0x1;  // Subtraction for comparison
//             break;
        
//         case 0x6F:  // JAL
//             signals.regWrite = true;
//             signals.jump = true;
//             break;
        
//         case 0x67:  // JALR
//             signals.regWrite = true;
//             signals.jump = true;
//             signals.aluSrc = true;
//             break;
        
//         case 0x37:  // LUI
//             signals.regWrite = true;
//             signals.aluSrc = true;
//             break;
        
//         case 0x17:  // AUIPC
//             signals.regWrite = true;
//             signals.aluSrc = true;
//             break;
//     }
    
//     return signals;
// }

// uint32_t NoForwardingProcessor::executeALU(uint32_t a, uint32_t b, uint32_t aluOp) {
//     switch (aluOp) {
//         case 0:  // ADD
//             return a + b;
//         case 1:  // SUB
//             return a - b;
//         case 2:  // SLL
//             return a << (b & 0x1F);
//         case 3:  // SLT
//             return static_cast<int32_t>(a) < static_cast<int32_t>(b) ? 1 : 0;
//         case 4:  // SLTU
//             return a < b ? 1 : 0;
//         case 5:  // XOR
//             return a ^ b;
//         case 6:  // SRL
//             return a >> (b & 0x1F);
//         case 7:  // SRA
//             return static_cast<int32_t>(a) >> (b & 0x1F);
//         case 8:  // OR
//             return a | b;
//         case 9:  // AND
//             return a & b;
//         default:
//             return 0;
//     }
// }

// uint32_t NoForwardingProcessor::extractImmediate(uint32_t instruction, uint32_t opcode) {
//     uint32_t imm = 0;
    
//     // I-type
//     if (opcode == 0x13 || opcode == 0x03 || opcode == 0x67) {
//         imm = instruction >> 20;
//         // Sign extend
//         if (imm & 0x800) imm |= 0xFFFFF000;
//     }
//     // S-type
//     else if (opcode == 0x23) {
//         imm = ((instruction >> 25) & 0x7F) << 5;
//         imm |= (instruction >> 7) & 0x1F;
//         if (imm & 0x800) imm |= 0xFFFFF000;
//     }
//     // B-type
//     else if (opcode == 0x63) {
//         imm = ((instruction >> 31) & 0x1) << 12;
//         imm |= ((instruction >> 7) & 0x1) << 11;
//         imm |= ((instruction >> 25) & 0x3F) << 5;
//         imm |= ((instruction >> 8) & 0xF) << 1;
//         if (imm & 0x1000) imm |= 0xFFFFE000;
//     }
//     // U-type
//     else if (opcode == 0x37 || opcode == 0x17) {
//         imm = instruction & 0xFFFFF000;
//     }
//     // J-type
//     else if (opcode == 0x6F) {
//         imm = ((instruction >> 31) & 0x1) << 20;
//         imm |= ((instruction >> 12) & 0xFF) << 12;
//         imm |= ((instruction >> 20) & 0x1) << 11;
//         imm |= ((instruction >> 21) & 0x3FF) << 1;
//         if (imm & 0x100000) imm |= 0xFFF00000;
//     }
    
//     return imm;
// }

// void NoForwardingProcessor::updatePipelineHistory(int cycle) {
//     // Convert cycle to unsigned for vector size comparisons
//     size_t unsignedCycle = static_cast<size_t>(cycle);
    
//     // Update pipeline history for each active instruction
//     if (!ifid.isEmpty) {
//         if (pipelineHistory.find(ifid.instructionString) == pipelineHistory.end()) {
//             pipelineHistory[ifid.instructionString] = std::vector<std::string>(unsignedCycle + 1, " ");
//         }
//         if (pipelineHistory[ifid.instructionString].size() <= unsignedCycle) {
//             pipelineHistory[ifid.instructionString].resize(unsignedCycle + 1, " ");
//         }
//         pipelineHistory[ifid.instructionString][unsignedCycle] = ifid.isStalled ? "-" : "IF";
//     }
    
//     if (!idex.isEmpty) {
//         if (pipelineHistory.find(idex.instructionString) == pipelineHistory.end()) {
//             pipelineHistory[idex.instructionString] = std::vector<std::string>(unsignedCycle + 1, " ");
//         }
//         if (pipelineHistory[idex.instructionString].size() <= unsignedCycle) {
//             pipelineHistory[idex.instructionString].resize(unsignedCycle + 1, " ");
//         }
//         pipelineHistory[idex.instructionString][unsignedCycle] = idex.isStalled ? "-" : "ID";
//     }
    
//     if (!exmem.isEmpty) {
//         if (pipelineHistory.find(exmem.instructionString) == pipelineHistory.end()) {
//             pipelineHistory[exmem.instructionString] = std::vector<std::string>(unsignedCycle + 1, " ");
//         }
//         if (pipelineHistory[exmem.instructionString].size() <= unsignedCycle) {
//             pipelineHistory[exmem.instructionString].resize(unsignedCycle + 1, " ");
//         }
//         pipelineHistory[exmem.instructionString][unsignedCycle] = exmem.isStalled ? "-" : "EX";
//     }
    
//     if (!memwb.isEmpty) {
//         if (pipelineHistory.find(memwb.instructionString) == pipelineHistory.end()) {
//             pipelineHistory[memwb.instructionString] = std::vector<std::string>(unsignedCycle + 1, " ");
//         }
//         if (pipelineHistory[memwb.instructionString].size() <= unsignedCycle) {
//             pipelineHistory[memwb.instructionString].resize(unsignedCycle + 1, " ");
//         }
//         pipelineHistory[memwb.instructionString][unsignedCycle] = "MEM";
//     }
    
//     // Update WB stage for any instruction that was in MEM in previous cycle
//     for (auto& pair : pipelineHistory) {
//         if (pair.second.size() <= unsignedCycle) {
//             pair.second.resize(unsignedCycle + 1, " ");
//         }
//         if (cycle > 0 && pair.second.size() > static_cast<size_t>(cycle - 1) && pair.second[cycle - 1] == "MEM") {
//             pair.second[unsignedCycle] = "WB";
//         }
//     }
    
//     // Mark stalls for all instructions when we have a stall condition
//     if (stall) {
//         // Find the instruction that's currently in IF stage and mark it with a stall
//         for (auto& pair : pipelineHistory) {
//             if (pair.second.size() > unsignedCycle && pair.second[unsignedCycle] == "IF") {
//                 pair.second[unsignedCycle] = "-";
//                 break;  // Only mark one instruction as stalled
//             }
//         }
//     }
// }

// void NoForwardingProcessor::run(int cycles) {
//     // Reset pipeline state
//     pc = 0;
//     stall = false;
//     ifid.isEmpty = true;
//     ifid.isStalled = false;
//     idex.isEmpty = true;
//     idex.isStalled = false;
//     exmem.isEmpty = true;
//     exmem.isStalled = false;
//     memwb.isEmpty = true;
//     pipelineHistory.clear();
    
//     for (int cycle = 0; cycle < cycles; cycle++) {
//         // WB Stage
//         if (!memwb.isEmpty) {
//             if (memwb.controls.regWrite) {
//                 uint32_t writeData = memwb.controls.memToReg ? memwb.readData : memwb.aluResult;
//                 registers.write(memwb.rd, writeData);
//             }
//         }
        
//         // MEM Stage
//         if (!exmem.isEmpty) {
//             if (exmem.controls.memRead) {
//                 memwb.readData = dataMemory.readWord(exmem.aluResult);
//             }
//             if (exmem.controls.memWrite) {
//                 dataMemory.writeWord(exmem.aluResult, exmem.readData2);
//             }
            
//             // Propagate to WB stage
//             memwb.pc = exmem.pc;
//             memwb.aluResult = exmem.aluResult;
//             memwb.rd = exmem.rd;
//             memwb.controls = exmem.controls;
//             memwb.instructionString = exmem.instructionString;
//             memwb.isEmpty = false;
//         } else {
//             memwb.isEmpty = true;
//         }
        
//         // EX Stage
//         if (!idex.isEmpty) {
//             uint32_t aluOp1 = idex.readData1;
//             uint32_t aluOp2 = idex.controls.aluSrc ? idex.imm : idex.readData2;
            
//             exmem.aluResult = executeALU(aluOp1, aluOp2, idex.controls.aluOp);
            
//             // Propagate to MEM stage
//             exmem.pc = idex.pc;
//             exmem.readData2 = idex.readData2;
//             exmem.rd = idex.rd;
//             exmem.controls = idex.controls;
//             exmem.instructionString = idex.instructionString;
//             exmem.isEmpty = false;
//         } else {
//             exmem.isEmpty = true;
//         }
        
//         // ID Stage
//         if (!ifid.isEmpty) {
//             uint32_t instruction = ifid.instruction;
            
//             // Decode instruction fields
//             uint32_t opcode = instruction & 0x7F;
//             uint32_t rd = (instruction >> 7) & 0x1F;
//             uint32_t rs1 = (instruction >> 15) & 0x1F;
//             uint32_t rs2 = (instruction >> 20) & 0x1F;
            
//             // Extract immediate value
//             uint32_t imm = extractImmediate(instruction, opcode);
            
//             // Check for hazards
//             bool hazard = false;
            
//             if (rs1 != 0) {
//                 if (!idex.isEmpty && idex.controls.regWrite && idex.rd == rs1) {
//                     hazard = true;
//                 }
//                 if (!exmem.isEmpty && exmem.controls.regWrite && exmem.rd == rs1) {
//                     hazard = true;
//                 }
//             }
            
//             if (rs2 != 0) {
//                 if (!idex.isEmpty && idex.controls.regWrite && idex.rd == rs2) {
//                     hazard = true;
//                 }
//                 if (!exmem.isEmpty && exmem.controls.regWrite && exmem.rd == rs2) {
//                     hazard = true;
//                 }
//             }
            
//             // Special case for load instruction followed by instruction using the loaded value
//             if (!idex.isEmpty && idex.controls.memRead && 
//                 (idex.rd == rs1 || idex.rd == rs2)) {
//                 hazard = true;
//             }
            
//             // Handle control hazards (branch/jump)
//             bool takeJump = false;
//             uint32_t jumpTarget = 0;
            
//             if (opcode == 0x63) {  // Branch
//                 uint32_t rs1Value = registers.read(rs1);
//                 uint32_t rs2Value = registers.read(rs2);
//                 bool condition = false;
//                 uint32_t funct3 = (instruction >> 12) & 0x7;
                
//                 switch (funct3) {
//                     case 0x0:  // BEQ
//                         condition = (rs1Value == rs2Value);
//                         break;
//                     case 0x1:  // BNE
//                         condition = (rs1Value != rs2Value);
//                         break;
//                     case 0x4:  // BLT
//                         condition = (static_cast<int32_t>(rs1Value) < static_cast<int32_t>(rs2Value));
//                         break;
//                     case 0x5:  // BGE
//                         condition = (static_cast<int32_t>(rs1Value) >= static_cast<int32_t>(rs2Value));
//                         break;
//                     case 0x6:  // BLTU
//                         condition = (rs1Value < rs2Value);
//                         break;
//                     case 0x7:  // BGEU
//                         condition = (rs1Value >= rs2Value);
//                         break;
//                 }
                
//                 if (condition) {
//                     takeJump = true;
//                     jumpTarget = ifid.pc + imm;
//                 }
//             }
//             else if (opcode == 0x6F) {  // JAL
//                 takeJump = true;
//                 jumpTarget = ifid.pc + imm;
//             }
//             else if (opcode == 0x67) {  // JALR
//                 takeJump = true;
//                 jumpTarget = (registers.read(rs1) + imm) & ~1;
//             }
            
//             if (!hazard) {
//                 // Read registers
//                 idex.readData1 = registers.read(rs1);
//                 idex.readData2 = registers.read(rs2);
                
//                 // Set control signals and propagate to EX stage
//                 idex.pc = ifid.pc;
//                 idex.imm = imm;
//                 idex.rs1 = rs1;
//                 idex.rs2 = rs2;
//                 idex.rd = rd;
//                 idex.controls = decodeControlSignals(instruction);
//                 idex.instructionString = ifid.instructionString;
//                 idex.isEmpty = false;
//                 idex.isStalled = false;
                
//                 // Handle branch/jump
//                 if (takeJump) {
//                     pc = jumpTarget;
//                     ifid.isEmpty = true;  // Flush pipeline
//                 } else {
//                     pc += 4;  // Advance PC normally
//                 }
//             } else {
//                 // Stall the pipeline - mark instruction as stalled
//                 stall = true;
//                 ifid.isStalled = true;  // Mark IF/ID as stalled
//                 idex.isEmpty = true;
//             }
//         } else {
//             idex.isEmpty = true;
//         }
        
//         // IF Stage
//         if (!stall && pc / 4 < instructionMemory.size()) {
//             ifid.instruction = instructionMemory[pc / 4];
//             ifid.pc = pc;
//             ifid.instructionString = instructionStrings[pc / 4];
//             ifid.isEmpty = false;
//             ifid.isStalled = false;
//             pc += 4;  // Advance PC normally
//         } else if (stall) {
//             // Keep current instruction in IF stage during stall (PC doesn't change)
//             // The stall flag will be reset after updatePipelineHistory
//             // to allow the pipeline to continue in the next cycle
//         } else {
//             ifid.isEmpty = true;
//             ifid.isStalled = false;
//         }
        
//         // Update pipeline history for visualization
//         updatePipelineHistory(cycle);
        
//         // Reset stall flag for next cycle
//         if (stall) {
//             stall = false;
//             ifid.isStalled = false;  // Clear stall flag
//         }
//     }
// }

// void NoForwardingProcessor::printPipelineDiagram() {
//     // Create or open output.txt file
//     std::ofstream outFile("output.txt");
//     if (!outFile.is_open()) {
//         std::cerr << "Error: Unable to open output.txt for writing" << std::endl;
//         return;
//     }
    
//     // Print table header with semicolons as separators
//     outFile << "Instruction";
//     for (size_t i = 0; i < 15; i++) { // Assuming 15 cycles maximum
//         outFile << ";" << i;
//     }
//     outFile << ";" << std::endl;
    
//     // Print each instruction with its pipeline stages in program order
//     for (size_t i = 0; i < instructionStrings.size(); i++) {
//         const std::string& instr = instructionStrings[i];
        
//         // Remove any potential newlines or carriage returns from instruction name
//         std::string cleanInstr = instr;
//         cleanInstr.erase(std::remove(cleanInstr.begin(), cleanInstr.end(), '\n'), cleanInstr.end());
//         cleanInstr.erase(std::remove(cleanInstr.begin(), cleanInstr.end(), '\r'), cleanInstr.end());
        
//         // Print instruction name without newline
//         outFile << cleanInstr;
        
//         // Print each stage for this instruction using semicolons
//         if (pipelineHistory.find(instr) != pipelineHistory.end()) {
//             for (size_t j = 0; j < pipelineHistory[instr].size(); j++) {
//                 outFile << ";" << pipelineHistory[instr][j];
//             }
            
//             // Fill remaining columns with empty cells
//             for (size_t j = pipelineHistory[instr].size(); j < 15; j++) {
//                 outFile << ";";
//             }
//         } else {
//             // If instruction wasn't executed, print empty cells for all cycles
//             for (size_t j = 0; j < 15; j++) {
//                 outFile << ";";
//             }
//         }
        
//         outFile << ";" << std::endl;
//     }
    
//     outFile.close();
// }
