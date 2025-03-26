#ifndef FORWARDING_PROCESSOR_HPP
#define FORWARDING_PROCESSOR_HPP

#include "Processor.hpp"
#include <vector>
#include <string>

// Forwarding sources for ALU inputs
enum ForwardSource {
    NO_FORWARDING,  // Use register value
    FORWARD_EX_MEM, // Forward from EX/MEM
    FORWARD_MEM_WB  // Forward from MEM/WB
};

class ForwardingProcessor : public NoForwardingProcessor {
public:
    ForwardingProcessor();
    ~ForwardingProcessor();
    
    // Override the run method to implement forwarding
    // Make sure this exactly matches the base class signature
    virtual void run(int cycles) override;
    virtual void printPipelineDiagram(std::string& InputFile) override;
};

#endif // FORWARDING_PROCESSOR_HPP
