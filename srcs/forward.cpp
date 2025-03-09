// forward.cpp - Main file for forwarding processor
#include "forwarding_processor.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <instruction_file> <cycle_count>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    int cycles = std::stoi(argv[2]);
    
    ForwardingProcessor processor(filename);
    processor.run(cycles);
    
    return 0;
}