// no_forwarding_processor.cpp
#include "no_forwarding_processor.hpp"

NoForwardingProcessor::NoForwardingProcessor(const std::string& filename) 
    : Processor(filename) {
}

void NoForwardingProcessor::advancePipeline() {
    // Check for hazards first
    handleHazards();
    
    // Pipeline advancement (from back to front to avoid overwriting)
    writeBack();
    
    // Move data between pipeline registers
    mem_wb = ex_mem;
    ex_mem = id_ex;
    
    // If no stall, continue normal operation
    if (!id_ex.stall) {
        id_ex = if_id;
        // Only fetch new instruction if not stalled
        fetchInstruction();
    } else {
        // Insert a bubble in the ID/EX register
        id_ex = PipelineRegister();
    }
    
    // Execute pipeline stages
    memoryAccess();
    executeInstruction();
    decodeInstruction();
    
    // Update visualization
    for (size_t i = 0; i < instructions.size(); i++) {
        if (if_id.instructionString == instructions[i].second) {
            // Record the pipeline stage in the visualization
            for (size_t j = 0; j < pipelineVisualization.size(); j++) {
                if (j > pipelineVisualization[i].size()) continue;
                
                if (mem_wb.instructionString == instructions[i].second) {
                    pipelineVisualization[i][j] = "WB";
                } else if (ex_mem.instructionString == instructions[i].second) {
                    pipelineVisualization[i][j] = "MEM";
                } else if (id_ex.instructionString == instructions[i].second) {
                    pipelineVisualization[i][j] = "EX";
                } else if (if_id.instructionString == instructions[i].second) {
                    pipelineVisualization[i][j] = "ID";
                } else {
                    pipelineVisualization[i][j] = "IF";
                }
            }
        }
    }
}

void NoForwardingProcessor::handleHazards() {
    // Check for data hazards (RAW)
    bool hazardDetected = false;
    
    // Check if instruction in ID stage needs a register value being written in later stages
    if (if_id.instruction != 0) {
        int rs1 = (if_id.instruction >> 15) & 0x1F;
        int rs2 = (if_id.instruction >> 20) & 0x1F;
        
        // Check for hazards with EX stage
        if (id_ex.controlSignals.regWrite && id_ex.rd != 0) {
            if (rs1 == id_ex.rd || rs2 == id_ex.rd) {
                hazardDetected = true;
            }
        }
        
        // Check for hazards with MEM stage
        if (ex_mem.controlSignals.regWrite && ex_mem.rd != 0) {
            if (rs1 == ex_mem.rd || rs2 == ex_mem.rd) {
                hazardDetected = true;
            }
        }
        
        // Check for hazards with WB stage
        if (mem_wb.controlSignals.regWrite && mem_wb.rd != 0) {
            if (rs1 == mem_wb.rd || rs2 == mem_wb.rd) {
                hazardDetected = true;
            }
        }
    }
    
    // Stall the pipeline if hazard detected
    if (hazardDetected) {
        id_ex.stall = true;
    } else {
        id_ex.stall = false;
    }
}