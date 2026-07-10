#pragma once
#include <cstdint>
#include <string>

class MMU {
private:
    uint8_t rom[0x8000]{};
    uint8_t vram[0x2000]{};
    uint8_t eram[0x2000]{};
    uint8_t wram[0x2000]{};
    uint8_t oam[0xA0]{};
    uint8_t io[0x80]{};
    uint8_t hram[0x80]{};

public:
    MMU(); 
    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);
    bool load_rom(const std::string& filename);
};