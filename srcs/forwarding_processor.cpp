// forwarding_processor.cpp
#include "forwarding_processor.hpp"

ForwardingProcessor::ForwardingProcessor(const std::string& filename) 
    : Processor(filename) {
}

void ForwardingProcessor::advancePipeline() {
    // Apply forwarding logic
    handleHazards();
    
    // Pipeline advancement (from back to front to avoid overwriting)
    writeBack();
    
    // Move data between pipeline registers
    mem_wb = ex_mem;
    ex_mem = id_ex;
    id_ex = if_id;
    
    // Execute pipeline stages
    memoryAccess();
    executeInstruction();
    decodeInstruction();
    fetchInstruction();
    
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

void ForwardingProcessor::handleHazards() {
    // Implement forwarding logic
    
    // Get the register indices for the ID/EX stage
    int rs1 = id_ex.rs1;
    int rs2 = id_ex.rs2;
    
    // Forward from MEM/WB to EX (for rs1)
    if (mem_wb.controlSignals.regWrite && mem_wb.rd != 0 && mem_wb.rd == rs1) {
        // Forward from MEM/WB to rs1
        int32_t forwardValue;
        if (mem_wb.controlSignals.memToReg) {
            forwardValue = mem_wb.memoryData;
        } else {
            forwardValue = mem_wb.aluResult;
        }
        id_ex.rs1Value = forwardValue;
    }
    
    // Forward from EX/MEM to EX (for rs1)
    if (ex_mem.controlSignals.regWrite && ex_mem.rd != 0 && ex_mem.rd == rs1) {
        // Forward from EX/MEM to rs1
        id_ex.rs1Value = ex_mem.aluResult;
    }
    
    // Forward from MEM/WB to EX (for rs2)
    if (mem_wb.controlSignals.regWrite && mem_wb.rd != 0 && mem_wb.rd == rs2) {
        // Forward from MEM/WB to rs2
        int32_t forwardValue;
        if (mem_wb.controlSignals.memToReg) {
            forwardValue = mem_wb.memoryData;
        } else {
            forwardValue = mem_wb.aluResult;
        }
        id_ex.rs2Value = forwardValue;
    }
    
    // Forward from EX/MEM to EX (for rs2)
    if (ex_mem.controlSignals.regWrite && ex_mem.rd != 0 && ex_mem.rd == rs2) {
        // Forward from EX/MEM to rs2
        id_ex.rs2Value = ex_mem.aluResult;
    }
    
    // Handle load-use hazards (requires stall even with forwarding)
    // This happens when a load instruction is immediately followed by an instruction that uses the loaded value
    if (ex_mem.controlSignals.memRead && ex_mem.rd != 0 && 
        (ex_mem.rd == rs1 || ex_mem.rd == rs2)) {
        // Stall the pipeline for one cycle
        // We need to insert a bubble in the pipeline
        id_ex.stall = true;
        if_id.stall = true;
        
        // Don't update PC
        pc -= 4;
    } else {
        id_ex.stall = false;
        if_id.stall = false;
    }
}