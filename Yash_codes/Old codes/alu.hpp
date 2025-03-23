#ifndef ALU_HPP
#define ALU_HPP

#include <cstdint>
#include "instruction.hpp"

class ALU {
public:
    static int32_t execute(Operation op, int32_t a, int32_t b, int32_t pc = 0);
};

#endif // ALU_HPP
