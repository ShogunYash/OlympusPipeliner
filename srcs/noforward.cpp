// noforward.cpp - Main file for no forwarding processor
#include "no_forwarding_processor.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <instruction_file> <cycle_count>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    int cycles = std::stoi(argv[2]);
    
    NoForwardingProcessor processor(filename);
    processor.run(cycles);
    
    return 0;
}