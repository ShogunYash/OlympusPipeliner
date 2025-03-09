// forwarding_processor.hpp
#ifndef FORWARDING_PROCESSOR_HPP
#define FORWARDING_PROCESSOR_HPP

#include "processor.hpp"

class ForwardingProcessor : public Processor {
public:
    ForwardingProcessor(const std::string& filename);
    ~ForwardingProcessor() = default;
    
    void advancePipeline() override;
    void handleHazards() override;
};

#endif // FORWARDING_PROCESSOR_HPP