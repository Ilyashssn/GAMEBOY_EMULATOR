#include "MMU.hpp"
#include <fstream>
#include <iostream>

MMU::MMU() {}

uint8_t MMU::read(uint16_t address) const {
    if (address <= 0x7FFF) {
        return rom[address];
    }
    if (address >= 0x8000 && address <= 0x9FFF) {
        return vram[address - 0x8000];
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
        return eram[address - 0xA000];
    }
    if (address >= 0xC000 && address <= 0xDFFF) {
        return wram[address - 0xC000];
    }
    if (address >= 0xFE00 && address <= 0xFE9F) {
        return oam[address - 0xFE00];
    }
    if (address >= 0xFF00 && address <= 0xFF7F) {
        return io[address - 0xFF00];
    }
    if (address >= 0xFF80 && address <= 0xFFFE) {
        return hram[address - 0xFF80];
    }
    return 0x00;
}

void MMU::write(uint16_t address, uint8_t value) {
    if (address <= 0x7FFF) {
        return;
    } else if (address >= 0x8000 && address <= 0x9FFF) {
        vram[address - 0x8000] = value;
    } else if (address >= 0xA000 && address <= 0xBFFF) {
        eram[address - 0xA000] = value;
    } else if (address <= 0xDFFF) {
        wram[address - 0xC000] = value;
    } else if (address >= 0xFE00 && address <= 0xFE9F) {
        oam[address - 0xFE00] = value;
    } else if(address >= 0xFF00 && address <= 0xFF7F){
        if(address==0xFF04){
            io[0xFF04-0xFF00]=0x00;
        }
        else{
            io[address - 0xFF00]=value;
        }
        if(address==0xFF02 && value==0x81){
            char character = static_cast<char>(io[0xFF01 - 0xFF00]);
            std::cout << character << std::flush;
            io[0xFF02-0xFF00]=0x00;
        }
    } else if (address >= 0xFF80 && address <= 0xFFFE) {
        hram[address - 0xFF80] = value;
    }
}



bool MMU::load_rom(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    file.read(reinterpret_cast<char*>(rom), 0x8000); 
    return true;
}

void MMU::write_hardware(uint16_t address, uint8_t value) {
    if (address >= 0xFF00 && address <= 0xFF7F) {
        io[address - 0xFF00] = value;
    }
}