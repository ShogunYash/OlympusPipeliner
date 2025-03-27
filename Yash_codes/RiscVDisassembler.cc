#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

// Function declarations
std::string disassembleInstruction(uint32_t instruction);
int32_t extractImmediate(uint32_t instruction, uint32_t opcode);
std::string getRegisterName(uint32_t regNum);

// Register names
const std::vector<std::string> registerNames = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0/fp", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

// Helper function to get register name
std::string getRegisterName(uint32_t regNum) {
    if (regNum < registerNames.size()) {
        return registerNames[regNum];
    }
    return "x" + std::to_string(regNum);
}

// Helper function to extract immediate values from different instruction formats
int32_t extractImmediate(uint32_t instruction, uint32_t opcode) {
    int32_t imm = 0;
    // I-type
    if (opcode == 0x13 || opcode == 0x03 || opcode == 0x67) {
        imm = instruction >> 20;
        if (imm & 0x800) imm |= 0xFFFFF000;  // Sign extension
    }
    // S-type
    else if (opcode == 0x23) {
        imm = ((instruction >> 25) & 0x7F) << 5;
        imm |= (instruction >> 7) & 0x1F;
        if (imm & 0x800) imm |= 0xFFFFF000;  // Sign extension
    }
    // B-type
    else if (opcode == 0x63) {
        imm = ((instruction >> 31) & 0x1) << 12;
        imm |= ((instruction >> 7) & 0x1) << 11;
        imm |= ((instruction >> 25) & 0x3F) << 5;
        imm |= ((instruction >> 8) & 0xF) << 1;
        if (imm & 0x1000) imm |= 0xFFFFE000;  // Sign extension
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
        if (imm & 0x100000) imm |= 0xFFF00000;  // Sign extension
    }
    return imm;
}

// Main disassembly function
std::string disassembleInstruction(uint32_t instruction) {
    std::ostringstream result;
    
    // Extract opcode, registers, and function codes
    uint32_t opcode = instruction & 0x7F;
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t funct7 = (instruction >> 25) & 0x7F;
    
    // Extract immediate value based on opcode
    int32_t imm = extractImmediate(instruction, opcode);
    
    // Disassemble based on opcode
    switch (opcode) {
        case 0x33: {  // R-type ALU operations
            if (funct7 == 0x00) {
                switch (funct3) {
                    case 0x0: result << "add " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x1: result << "sll " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x2: result << "slt " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x3: result << "sltu " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x4: result << "xor " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x5: result << "srl " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x6: result << "or " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x7: result << "and " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    default: result << "UNKNOWN R-type (funct3=" << funct3 << ")";
                }
            } else if (funct7 == 0x20) {
                switch (funct3) {
                    case 0x0: result << "sub " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x5: result << "sra " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    default: result << "UNKNOWN R-type (funct3=" << funct3 << ", funct7=" << funct7 << ")";
                }
            } else if (funct7 == 0x01) {  // M-extension
                switch (funct3) {
                    case 0x0: result << "mul " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x1: result << "mulh " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x2: result << "mulhsu " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x3: result << "mulhu " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x4: result << "div " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x5: result << "divu " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x6: result << "rem " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    case 0x7: result << "remu " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << getRegisterName(rs2); break;
                    default: result << "UNKNOWN M-extension (funct3=" << funct3 << ")";
                }
            } else {
                result << "UNKNOWN R-type (funct7=" << funct7 << ")";
            }
            break;
        }
        
        case 0x13: {  // I-type ALU operations
            switch (funct3) {
                case 0x0: result << "addi " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << imm; break;
                case 0x1: {
                    uint32_t shamt = (instruction >> 20) & 0x1F;
                    result << "slli " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << shamt;
                    break;
                }
                case 0x2: result << "slti " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << imm; break;
                case 0x3: result << "sltiu " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << imm; break;
                case 0x4: result << "xori " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << imm; break;
                case 0x5: {
                    uint32_t shamt = (instruction >> 20) & 0x1F;
                    if ((instruction >> 25) & 0x7F)
                        result << "srai " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << shamt;
                    else
                        result << "srli " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << shamt;
                    break;
                }
                case 0x6: result << "ori " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << imm; break;
                case 0x7: result << "andi " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << imm; break;
                default: result << "UNKNOWN I-type (funct3=" << funct3 << ")";
            }
            break;
        }
        
        case 0x03: {  // Load instructions
            switch (funct3) {
                case 0x0: result << "lb " << getRegisterName(rd) << ", " << imm << "(" << getRegisterName(rs1) << ")"; break;
                case 0x1: result << "lh " << getRegisterName(rd) << ", " << imm << "(" << getRegisterName(rs1) << ")"; break;
                case 0x2: result << "lw " << getRegisterName(rd) << ", " << imm << "(" << getRegisterName(rs1) << ")"; break;
                case 0x4: result << "lbu " << getRegisterName(rd) << ", " << imm << "(" << getRegisterName(rs1) << ")"; break;
                case 0x5: result << "lhu " << getRegisterName(rd) << ", " << imm << "(" << getRegisterName(rs1) << ")"; break;
                default: result << "UNKNOWN load (funct3=" << funct3 << ")";
            }
            break;
        }
        
        case 0x23: {  // Store instructions
            switch (funct3) {
                case 0x0: result << "sb " << getRegisterName(rs2) << ", " << imm << "(" << getRegisterName(rs1) << ")"; break;
                case 0x1: result << "sh " << getRegisterName(rs2) << ", " << imm << "(" << getRegisterName(rs1) << ")"; break;
                case 0x2: result << "sw " << getRegisterName(rs2) << ", " << imm << "(" << getRegisterName(rs1) << ")"; break;
                default: result << "UNKNOWN store (funct3=" << funct3 << ")";
            }
            break;
        }
        
        case 0x63: {  // Branch instructions
            switch (funct3) {
                case 0x0: result << "beq " << getRegisterName(rs1) << ", " << getRegisterName(rs2) << ", " << imm; break;
                case 0x1: result << "bne " << getRegisterName(rs1) << ", " << getRegisterName(rs2) << ", " << imm; break;
                case 0x4: result << "blt " << getRegisterName(rs1) << ", " << getRegisterName(rs2) << ", " << imm; break;
                case 0x5: result << "bge " << getRegisterName(rs1) << ", " << getRegisterName(rs2) << ", " << imm; break;
                case 0x6: result << "bltu " << getRegisterName(rs1) << ", " << getRegisterName(rs2) << ", " << imm; break;
                case 0x7: result << "bgeu " << getRegisterName(rs1) << ", " << getRegisterName(rs2) << ", " << imm; break;
                default: result << "UNKNOWN branch (funct3=" << funct3 << ")";
            }
            break;
        }
        
        case 0x67:  // JALR
            result << "jalr " << getRegisterName(rd) << ", " << getRegisterName(rs1) << ", " << imm;
            break;
            
        case 0x6F:  // JAL
            result << "jal " << getRegisterName(rd) << ", " << imm;
            break;
            
        case 0x37:  // LUI
            result << "lui " << getRegisterName(rd) << ", " << (imm >> 12);
            break;
            
        case 0x17:  // AUIPC
            result << "auipc " << getRegisterName(rd) << ", " << (imm >> 12);
            break;
            
        default:
            result << "UNKNOWN opcode: 0x" << std::hex << opcode;
    }
    
    return result.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [output_file]" << std::endl;
        std::cerr << "  input_file: File containing hex machine code (one instruction per line)" << std::endl;
        std::cerr << "  output_file: (Optional) File to write disassembled instructions" << std::endl;
        std::cerr << "              If not provided, output is sent to stdout" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = (argc > 2) ? argv[2] : "";
    
    // Open input file
    std::ifstream inFile(inputFile);
    if (!inFile) {
        std::cerr << "Error: Cannot open input file " << inputFile << std::endl;
        return 1;
    }
    
    // Prepare output
    std::ofstream outFile;
    if (!outputFile.empty()) {
        outFile.open(outputFile);
        if (!outFile) {
            std::cerr << "Error: Cannot open output file " << outputFile << std::endl;
            return 1;
        }
    }
    
    // Process input file line by line
    std::string line;
    uint32_t address = 0;
    while (std::getline(inFile, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }
        
        // Remove comments
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        try {
            // Parse hex instruction
            uint32_t instruction;
            if (line.substr(0, 2) == "0x" || line.substr(0, 2) == "0X") {
                instruction = std::stoul(line.substr(2), nullptr, 16);
            } else {
                instruction = std::stoul(line, nullptr, 16);
            }
            
            // Disassemble instruction
            std::string disassembled = disassembleInstruction(instruction);
            
            // Format output
            std::ostringstream output;
            output << "0x" << std::setfill('0') << std::setw(8) << std::hex << instruction 
                   << " ; " << disassembled;
            
            // Write to output
            if (!outputFile.empty()) {
                outFile << output.str() << std::endl;
            } else {
                std::cout << output.str() << std::endl;
            }
            
            address += 4;  // Move to next instruction address
        } catch (const std::exception& e) {
            std::cerr << "Error processing line: " << line << " - " << e.what() << std::endl;
        }
    }
    
    // Close files
    inFile.close();
    if (!outputFile.empty()) {
        outFile.close();
        std::cout << "Disassembly completed. Output written to " << outputFile << std::endl;
    }
    
    return 0;
}
