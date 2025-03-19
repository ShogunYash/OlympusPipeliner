#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include "forwarding.hpp"

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
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string address, hex_instr;
        
        // Extract the address and instruction (machine code)
        if (!(iss >> address >> hex_instr)) {
            std::cerr << "Error: Could not parse line: " << line << std::endl;
            continue;
        }
        
        try {
            // Convert the hex string to an unsigned 32-bit integer.
            uint32_t instruction = std::stoul(hex_instr, nullptr, 16);
            instructions.push_back(instruction);
        } catch (const std::exception& e) {
            std::cerr << "Conversion error for instruction " << hex_instr 
                      << " (" << e.what() << ")" << std::endl;
        }
    }
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
    
    // Create an instance of the forwarding processor simulator.
    ForwardingProcessor simulator(instructions);
    
    // Run the simulation for the specified number of cycles.
    simulator.run(cycleCount);
    
    // Output the pipeline diagram to stdout.
    simulator.printPipelineDiagram();
    
    return 0;
}
