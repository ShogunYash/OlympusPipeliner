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
    
// Inside the NoForwardingProcessor class declaration:
private:
    // Structure to track instruction state in pipeline
    struct PipelineState {
        std::string assembly;  // Assembly representation
        std::string stage;     // Current pipeline stage (IF, ID, EX, MEM, WB, or "-" for stall)
    };
    
    // Record of pipeline state at each cycle
    std::vector<std::vector<PipelineState>> pipeline_history;
    
    // Count of instructions that completed execution
    int executed_instructions = 0;
    
public:
    NoForwardingProcessor();
    
    // Execute one cycle
    void step() override;


};

#endif // NO_FORWARDING_HPP
