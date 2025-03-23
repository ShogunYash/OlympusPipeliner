#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include "no_forwarding.hpp"

// Function to load instructions from file.
// Each line should contain two fields separated by whitespace:
// an address and a 32-bit hexadecimal machine code.
std::vector<uint32_t> loadInstructionsFromFile(const std::string& filename) {
    std::vector<uint32_t> instructions;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        return instructions;
    }
    
    std::cout << "Reading instructions from file: " << filename << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Address     | Hex Instruction | Assembly" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    std::string line;
    uint32_t index = 0;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // Remove any comments from the line
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        
        // Skip if line is empty after removing comments
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string address, hex_instr;
        
        // Extract the address and instruction (machine code)
        if (!(iss >> address >> hex_instr)) {
            std::cerr << "Error: Could not parse line: " << line << std::endl;
            continue;
        }
        
        try {
            // Convert the hex string to an unsigned 32-bit integer
            uint32_t instruction = std::stoul(hex_instr, nullptr, 16);
            instructions.push_back(instruction);
            
            // Create an instruction object to decode the assembly
            Instruction instr(instruction);
            
            // Print debug information
            std::cout << address << " | " << hex_instr << " | " << instr.getAssembly() << std::endl;
            
            index++;
        } catch (const std::exception& e) {
            std::cerr << "Conversion error for instruction " << hex_instr 
                      << " (" << e.what() << ")" << std::endl;
        }
    }
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Total instructions read: " << instructions.size() << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    file.close();
    return instructions;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <inputfile> [cycleCount]" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    int cycleCount = 0;  // 0 indicates run until completion
    
    if (argc > 2) {
        // If a cycle count is provided, use it as a limit
        cycleCount = std::stoi(argv[2]);
    }
    
    // Load the RISC-V machine code instructions from file.
    std::vector<uint32_t> instructions = loadInstructionsFromFile(inputFile);
    if (instructions.empty()) {
        std::cerr << "No instructions loaded from file." << std::endl;
        return 1;
    }
    
    // Create an instance of the no-forwarding processor simulator.
    NoForwardingProcessor simulator(instructions);
    
    // Run the simulation until natural completion or specified cycle count
    simulator.run(cycleCount);
    
    // Output the pipeline diagram to stdout.
    simulator.printPipelineDiagram();
    
    return 0;
}
