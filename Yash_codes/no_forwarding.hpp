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
    
public:
    NoForwardingProcessor();
    
    // Execute one cycle
    void step() override;
};

#endif // NO_FORWARDING_HPP
