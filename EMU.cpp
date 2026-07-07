#include <cstdint>
#include <iostream>

class MMU {
    uint8_t rom[0x8000]{};
    uint8_t vram[0x2000]{};
    uint8_t eram[0x2000]{};
    uint8_t wram[0x2000]{};
    uint8_t oam[0xA0]{};
    uint8_t hram[0x80]{};

public:
    uint8_t read(uint16_t address) const {
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
        if (address >= 0xFF80 && address <= 0xFFFE) {
            return hram[address - 0xFF80];
        }
        return 0x00;
    }

    void write(uint16_t address, uint8_t value) {
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
        } else if (address >= 0xFF80 && address <= 0xFFFE) {
            hram[address - 0xFF80] = value;
        }
    }
};

class Registers{
public:
     uint8_t A;
     uint8_t B;
     uint8_t C;
     uint8_t D;
     uint8_t E;
     uint8_t F;
     uint8_t H;
     uint8_t L;
    uint16_t get_BC() const { return (static_cast<uint16_t>(B) << 8) | C; }
    uint16_t get_DE() const { return (static_cast<uint16_t>(D) << 8) | E; }
    uint16_t get_HL() const { return (static_cast<uint16_t>(H) << 8) | L; }
    uint16_t get_AF() const { return (static_cast<uint16_t>(A) << 8) | F; }
    void set_BC(uint16_t val) { B = val >> 8; C = val & 0xFF; }
    void set_DE(uint16_t val) { D = val >> 8; E = val & 0xFF; }
    void set_HL(uint16_t val) { H = val >> 8; L = val & 0xFF; }
    void set_AF(uint16_t val) { A = val >> 8; F = val & 0xF0; }
    uint16_t PC,SP;
};

class CPU{
private:
     Registers R;
     MMU& Mem;
     uint64_t clock;
     uint8_t opcode;

public :
    CPU(MMU& shared_mem) : Mem(shared_mem), clock(0), opcode(0) {
        reset();
    }
    void reset(){
        R.A = 0x01;
        R.F = 0xB0;
        R.set_BC(0x0013);
        R.set_DE(0x00D8);
        R.set_HL(0x014D);
        R.SP = 0xFFFE;
        R.PC = 0x0100; 
        clock = 0;
    }
    void step(){
        opcode=Mem.read(R.PC++);
        switch(opcode){
            case 0x7F:
               R.A=R.A; clock+=4;
               break;
            case 0x78:
               R.A=R.B;clock+=4;
               break;
            case 0x79:
               R.A=R.C;clock+=4;
               break;
            case 0x7A:
               R.A=R.D;clock+=4;
               break;
            case 0x7B:
               R.A=R.E;clock+=4;
               break;
            case 0x7C:
               R.A=R.H;clock+=4;
               break;
            case 0x7D:
               R.A=R.L;clock+=4;
               break;
            case 0x47:
               R.B=R.A;clock+=4;
               break;
            case 0x40:
               R.B=R.B;clock+=4;
               break;
            case 0x41:
               R.B=R.C;clock+=4;
               break;
            case 0x42:
               R.B=R.D;clock+=4;
               break;
            case 0x43:
               R.B=R.E;clock+=4;
               break;
            case 0x44:
               R.B=R.H;clock+=4;
               break;
            case 0x45:
               R.B=R.L;clock+=4;
               break;
            case 0x4F:
               R.C=R.A;clock+=4;
               break;
            case 0x48:
               R.C=R.B;clock+=4;
               break;
            case 0x49:
               R.C=R.C;clock+=4;
               break;
            case 0x4A:
               R.C=R.D;clock+=4;
               break;
            case 0x4B:
               R.C=R.E;clock+=4;
               break;
            case 0x4C:
               R.C=R.H;clock+=4;
               break;
            case 0x4D:
               R.C=R.L;clock+=4;
               break;
            case 0x57:
               R.D=R.A;clock+=4;
               break;
            case 0x50:
               R.D=R.B;clock+=4;
               break;
            case 0x51:
               R.D=R.C;clock+=4;
               break;
            case 0x52:
               R.D=R.D;clock+=4;
               break;
            case 0x53:
               R.D=R.E;clock+=4;
               break;
            case 0x54:
               R.D=R.H;clock+=4;
               break;
            case 0x55:
               R.D=R.L;clock+=4;
               break;
            case 0x5F:
               R.E=R.A;clock+=4;
               break;
            case 0x58:
               R.E=R.B;clock+=4;
               break;
            case 0x59:
               R.E=R.C;clock+=4;
               break;
            case 0x5A:
               R.E=R.D;clock+=4;
               break;
            case 0x5B:
               R.E=R.E;clock+=4;
               break;
            case 0x5C:
               R.E=R.H;clock+=4;
               break;
            case 0x5D:
               R.E=R.L;clock+=4;
               break;
            case 0x67:
               R.H=R.A;clock+=4;
               break;
            case 0x60:
               R.H=R.B;clock+=4;
               break;
            case 0x61:
               R.H=R.C;clock+=4;
               break;
            case 0x62:
               R.H=R.D;clock+=4;
               break;
            case 0x63:
               R.H=R.E;clock+=4;
               break;
            case 0x64:
               R.H=R.H;clock+=4;
               break;
            case 0x65:
               R.H=R.L;clock+=4;
               break;
            case 0x6F:
               R.L=R.A;clock+=4;
               break;
            case 0x68:
               R.L=R.B;clock+=4;
               break;
            case 0x69:
               R.L=R.C;clock+=4;
               break;
            case 0x6A:
               R.L=R.D;clock+=4;
               break;
            case 0x6B:
               R.L=R.E;clock+=4;
               break;
            case 0x6C:
               R.L=R.H;clock+=4;
               break;
            case 0x6D:
               R.L=R.L;clock+=4;
               break;
            case 0x06:
               R.B=Mem.read(R.PC++);clock+=8;
               break;
            case 0x0E:
               R.C=Mem.read(R.PC++);clock+=8;
               break;
            case 0x16:
               R.D=Mem.read(R.PC++);clock+=8;
               break;
            case 0x1E:
               R.E=Mem.read(R.PC++);clock+=8;
               break;
            case 0x26:
               R.H=Mem.read(R.PC++);clock+=8;
               break;
            case 0x2E:
               R.L=Mem.read(R.PC++);clock+=8;
               break;
            case 0x3E:
               R.A=Mem.read(R.PC++);clock+=8;
               break;
            case 0x46:
               R.B=Mem.read(R.get_HL());clock+=8;
               break;
            case 0x4E:
               R.C=Mem.read(R.get_HL());clock+=8;
               break;
            case 0x56:
               R.D=Mem.read(R.get_HL());clock+=8;
               break;
            case 0x5E:
               R.E=Mem.read(R.get_HL());clock+=8;
               break;
            case 0x66:
               R.H=Mem.read(R.get_HL());clock+=8;
               break;
            case 0x6E:
               R.L=Mem.read(R.get_HL());clock+=8;
               break;
            case 0x7E:
               R.A=Mem.read(R.get_HL());clock+=8;
               break;
            case 0x70:
               Mem.write(R.get_HL(),R.B);clock+=8;
               break;
            case 0x71:
               Mem.write(R.get_HL(),R.C);clock+=8;
               break;
            case 0x72:
               Mem.write(R.get_HL(),R.D);clock+=8;
               break;
            case 0x73:
               Mem.write(R.get_HL(),R.E);clock+=8;
               break;
            case 0x74:
               Mem.write(R.get_HL(),R.H);clock+=8;
               break;
            case 0x75:
               Mem.write(R.get_HL(),R.L);clock+=8;
               break;
            case 0x77:
               Mem.write(R.get_HL(),R.A);clock+=8;
               break;
            case 0x36:
               Mem.write(R.get_HL(),Mem.read(R.PC++));clock+=12;
               break;
            case 0x01:
               R.C=Mem.read(R.PC++);R.B=Mem.read(R.PC++);clock+=12;
               break;
            case 0x11:
               R.E=Mem.read(R.PC++);R.D=Mem.read(R.PC++);clock+=12;
               break;
            case 0x21:
               R.L=Mem.read(R.PC++);R.H=Mem.read(R.PC++);clock+=12;
               break;
            case 0x31:
               R.SP=Mem.read(R.PC++)|(Mem.read(R.PC++)<<8);clock+=12;
               break;
            case 0x0A:
               R.A=Mem.read(R.get_BC());clock+=8;
               break;
            case 0x1A:
               R.A=Mem.read(R.get_DE());clock+=8;
               break;
            case 0x02:
               Mem.write(R.get_BC(),R.A);clock+=8;
               break;
            case 0x12:
               Mem.write(R.get_DE(),R.A);clock+=8;
               break;
            case 0xFA:
               R.A=Mem.read(Mem.read(R.PC++)|(Mem.read(R.PC++)<<8));clock+=16;
               break;
            case 0xEA:
               Mem.write(Mem.read(R.PC++)|(Mem.read(R.PC++)<<8),R.A);clock+=16;
               break;
            case 0xE0:
               Mem.write(Mem.read(R.PC++)+0xFF00,R.A);clock+=12;
               break;
            case 0xF0:
               R.A=Mem.read(Mem.read(R.PC++)+0xFF00);clock+=12;
               break;
            case 0xE2:
               Mem.write(R.C+0xFF00,R.A);clock+=8;
               break;
            case 0xF2:
               R.A=Mem.read(R.C+0xFF00);clock+=8;
               break;
            case 0x22:
               Mem.write(R.get_HL(),R.A);
               R.set_HL(R.get_HL()+1);
               clock+=8;
               break;
            case 0x32:
               Mem.write(R.get_HL(),R.A);
               R.set_HL(R.get_HL()-1);
               clock+=8;
               break;
            case 0x2A:
               R.A=Mem.read(R.get_HL());
               R.set_HL(R.get_HL()+1);
               clock+=8;
               break;
            case 0x3A:
               R.A=Mem.read(R.get_HL());
               R.set_HL(R.get_HL()-1);
               clock+=8;
               break;


              
               

               
        }
    }
};
