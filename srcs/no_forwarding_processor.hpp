// no_forwarding_processor.hpp
#ifndef NO_FORWARDING_PROCESSOR_HPP
#define NO_FORWARDING_PROCESSOR_HPP

#include "processor.hpp"

class NoForwardingProcessor : public Processor {
public:
    NoForwardingProcessor(const std::string& filename);
    ~NoForwardingProcessor() = default;
    
    void advancePipeline() override;
    void handleHazards() override;
};

#endif // NO_FORWARDING_PROCESSOR_HPP