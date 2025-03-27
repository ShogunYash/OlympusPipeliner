#include "ForwardingProcessor.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Check arguments
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <instruction_file> <num_cycles>" << std::endl;
        return 1;
    }
    
    // Get filename and number of cycles
    std::string filename = argv[1];
    int cycles = std::stoi(argv[2]);
    
    std::cout << "Running with forwarding for " << cycles << " cycles" << std::endl;
    
    // Create forwarding processor
    ForwardingProcessor processor;
    
    // Load instructions
    if (!processor.loadInstructions(filename)) {
        std::cerr << "Failed to load instructions from " << filename << std::endl;
        return 1;
    }
    
    // Run simulation
    processor.run(cycles);
    
    // Print pipeline diagram
    processor.printPipelineDiagram(filename, true);
    
    std::cout << "Forwarding simulation complete. Results written to CSV file." << std::endl;
    return 0;
}
