#pragma once
#include <array>
#include <cstdint>

class RegisterFile {
private:
    std::array<int32_t, 32> registers;  // Changed from uint32_t to int32_t

public:
    RegisterFile();
    int32_t read(uint32_t index) const;  // Changed return type to int32_t
    void write(uint32_t index, int32_t value);  // Changed parameter type to int32_t
};
