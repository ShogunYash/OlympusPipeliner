#ifndef FORWARDINGPROCESSOR_HPP
#define FORWARDINGPROCESSOR_HPP

#include "Processor.hpp"

enum ForwardingSource {
    NO_FORWARD = 0,
    EX_MEM_FORWARD = 1,
    MEM_WB_FORWARD = 2
};

class ForwardingProcessor : public NoForwardingProcessor {
private:
    // Forwarding control signals
    struct ForwardingUnit {
        bool forwardA = false;       // Forward to first ALU operand
        bool forwardB = false;       // Forward to second ALU operand
        ForwardingSource forwardASource = NO_FORWARD; 
        ForwardingSource forwardBSource = NO_FORWARD;
    };

    // Override the run method to implement forwarding
    

    // Check for forwarding conditions
    ForwardingUnit checkForwarding();

public:
    virtual void run(int cycles) override;
    ForwardingProcessor();
    virtual ~ForwardingProcessor() = default;
};

#endif // FORWARDINGPROCESSOR_HPP