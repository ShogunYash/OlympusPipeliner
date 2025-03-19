#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <string>
#include <cstdint>

// Pipeline registers to hold data between stages
struct IF_ID_Register {
    uint32_t pc;
    uint32_t instruction;
    bool valid;
    std::string stage_display;
    
    IF_ID_Register() : pc(0), instruction(0), valid(false), stage_display("") {}
};

struct ID_EX_Register {
    // Control signals
    bool reg_write;
    bool mem_to_reg;
    bool mem_read;
    bool mem_write;
    bool alu_src;
    bool branch;
    bool jump;
    
    // Data
    uint32_t pc;
    int32_t read_data_1;
    int32_t read_data_2;
    int32_t immediate;
    uint8_t rs1;
    uint8_t rs2;
    uint8_t rd;
    std::string assembly;
    bool valid;
    std::string stage_display;
    
    ID_EX_Register() : reg_write(false), mem_to_reg(false), mem_read(false), mem_write(false),
                       alu_src(false), branch(false), jump(false), pc(0), read_data_1(0), 
                       read_data_2(0), immediate(0), rs1(0), rs2(0), rd(0),
                       assembly(""), valid(false), stage_display("") {}
};

struct EX_MEM_Register {
    // Control signals
    bool reg_write;
    bool mem_to_reg;
    bool mem_read;
    bool mem_write;
    
    // Data
    int32_t alu_result;
    int32_t write_data;
    uint8_t rd;
    std::string assembly;
    bool valid;
    std::string stage_display;
    
    EX_MEM_Register() : reg_write(false), mem_to_reg(false), mem_read(false), mem_write(false),
                        alu_result(0), write_data(0), rd(0), assembly(""), valid(false),
                        stage_display("") {}
};

struct MEM_WB_Register {
    // Control signals
    bool reg_write;
    bool mem_to_reg;
    
    // Data
    int32_t alu_result;
    int32_t read_data;
    uint8_t rd;
    std::string assembly;
    bool valid;
    std::string stage_display;
    
    MEM_WB_Register() : reg_write(false), mem_to_reg(false), alu_result(0), read_data(0), rd(0),
                        assembly(""), valid(false), stage_display("") {}
};

#endif // PIPELINE_HPP
