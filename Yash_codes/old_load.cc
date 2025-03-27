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