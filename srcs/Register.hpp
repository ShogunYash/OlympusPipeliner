#pragma once
#include <array>
#include <cstdint>

class RegisterFile {
private:
    std::array<uint32_t, 32> registers;

public:
    RegisterFile();
    uint32_t read(uint32_t index) const;
    void write(uint32_t index, uint32_t value);
};
