#pragma once
#include "MMU.hpp"


class Registers {
public:
    uint8_t A, B, C, D, E, F, H, L;
    uint16_t get_BC() const { return (static_cast<uint16_t>(B) << 8) | C; }
    uint16_t get_DE() const { return (static_cast<uint16_t>(D) << 8) | E; }
    uint16_t get_HL() const { return (static_cast<uint16_t>(H) << 8) | L; }
    uint16_t get_AF() const { return (static_cast<uint16_t>(A) << 8) | F; }
    void set_BC(uint16_t val) { B = val >> 8; C = val & 0xFF; }
    void set_DE(uint16_t val) { D = val >> 8; E = val & 0xFF; }
    void set_HL(uint16_t val) { H = val >> 8; L = val & 0xFF; }
    void set_AF(uint16_t val) { A = val >> 8; F = val & 0xF0; }
    void set_flags(bool z, bool n, bool h, bool c) {
        F = (z << 7) | (n << 6) | (h << 5) | (c << 4);
    }
    uint16_t PC, SP;
};

class CPU {
private:
    Registers R;
    MMU& Mem;
    uint64_t clock;
    uint8_t opcode;

   
    void adding(uint8_t v);
    void adcing(uint8_t v);
    void subing(uint8_t v);
    void sbcing(uint8_t v);
    void anding(uint8_t v);
    void xoring(uint8_t v);
    void oring(uint8_t v);
    void cp(uint8_t v);
    uint8_t inc8(uint8_t v);
    uint8_t dec8(uint8_t v);
    
   
    uint16_t fetch16();
    uint16_t add16(uint16_t hl, uint16_t rr);
    void bit_test(uint8_t b, uint8_t r);

    uint16_t target_addr;
    uint8_t offset;

public:
    CPU(MMU& shared_mem);
    void reset();
    void step(); 
};