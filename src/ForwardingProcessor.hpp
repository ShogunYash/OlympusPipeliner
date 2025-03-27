#ifndef FORWARDING_PROCESSOR_HPP
#define FORWARDING_PROCESSOR_HPP

#include "Processor.hpp"
#include <vector>
#include <string>


class ForwardingProcessor : public NoForwardingProcessor {
public:
    ForwardingProcessor();
    ~ForwardingProcessor();
    
    // Override the run method to implement forwarding
    // Make sure this exactly matches the base class signature
    virtual void run(int cycles) override;
};

#endif // FORWARDING_PROCESSOR_HPP
