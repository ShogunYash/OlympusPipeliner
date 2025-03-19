#ifndef NO_FORWARDING_HPP
#define NO_FORWARDING_HPP

#include "processor.hpp"

class NoForwardingProcessor : public Processor {
private:
    // Pipeline stage implementations
    void fetch() override;
    void decode() override;
    void execute() override;
    void memory_access() override;
    void write_back() override;
    
    // Hazard detection
    bool detectDataHazard() const;
    
    // No need to redefine pipeline history structure here
    // as we're inheriting it from the Processor class
    
    // For compatibility with the updated implementation:
    void printPipelineDiagram();
    
    // Count of instructions that completed execution
    int executed_instructions = 0;
    
public:
    NoForwardingProcessor();
    
    // Execute one cycle
    void step() override;
};

#endif // NO_FORWARDING_HPP
