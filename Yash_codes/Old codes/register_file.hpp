#ifndef REGISTER_FILE_HPP
#define REGISTER_FILE_HPP

#include <array>
#include <cstdint>
#include <string>

class RegisterFile {
private:
    std::array<int32_t, 32> registers;
    
public:
    RegisterFile();
    
    // Read register value
    int32_t read(uint8_t reg) const;
    
    // Write register value (x0 is hardwired to 0)
    void write(uint8_t reg, int32_t value);
    
    // Dump all register values (for debugging)
    void dump() const;
};

#endif // REGISTER_FILE_HPP
