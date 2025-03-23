#include "Processor.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <cycle_count>" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    int cycleCount = std::stoi(argv[2]);
    
    NoForwardingProcessor processor;
    
    if (!processor.loadInstructions(inputFile)) {
        std::cerr << "Failed to load instructions from file: " << inputFile << std::endl;
        return 1;
    }
    
    processor.run(cycleCount);
    
    // Print pipeline diagram to file only
    processor.printPipelineDiagram();
    
    return 0;
}
