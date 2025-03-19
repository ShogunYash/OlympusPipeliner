#ifndef FORWARDING_HPP
#define FORWARDING_HPP

#include "processor.hpp"

class ForwardingProcessor : public Processor {
private:
    // Pipeline stage implementations
    void fetch() override;
    void decode() override;
    void execute() override;
    void memory_access() override;
    void write_back() override;
    
    // Hazard detection for load-use only (since other RAW hazards can be forwarded)
    bool detectLoadUseHazard() const;
    
    // Forwarding logic
    enum class ForwardingSource { NONE, EX_MEM, MEM_WB };
    
    // Determine forwarding source for each operand
    ForwardingSource getForwardingSource1() const;
    ForwardingSource getForwardingSource2() const;
    
    // Structure to track instruction state in pipeline for diagram
    struct PipelineState {
        std::string assembly;
        std::string stage;
    };
    
    // Record of pipeline state at each cycle
    std::vector<std::vector<PipelineState>> pipeline_history;
    
    // Count of instructions that completed execution
    int executed_instructions = 0;
    
public:
    ForwardingProcessor(const std::vector<uint32_t>& program);
    
    // Execute one cycle
    void step() override;
    
    // Run the simulation for specified cycles
    void run(int cycle_count);
    
    // Print pipeline diagram output
    void printPipelineDiagram();
};

#endif // FORWARDING_HPP
