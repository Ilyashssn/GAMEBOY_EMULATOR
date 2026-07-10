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
    void set_flags(bool z, bool n,bool h,bool c){
      F= (z<<7)|(n<<6)|(h<<5)|(c<<4);
    }
    uint16_t PC,SP;
};

class CPU{
private:
     Registers R;
     MMU& Mem;
     uint64_t clock;
     uint8_t opcode;
     void adding(uint8_t v){
          int res=R.A+v;
          bool z=((res & 0xFF) == 0);
          bool n=false;
          bool h=(((R.A & 0x0F) + (v & 0x0F)) > 0x0F);
          bool c=(res > 0xFF);
          R.set_flags(z,n,h,c);
          R.A=(uint8_t)(res & 0xFF);
     }
     void adcing(uint8_t v){
          uint8_t c_in = (R.F >> 4) & 0x01;
          int res = R.A + v + c_in;
          
          bool z = ((res & 0xFF) == 0);
          bool n = false;
          // On inclut c_in dans le calcul du débordement du nibble inférieur
          bool h = (((R.A & 0x0F) + (v & 0x0F) + c_in) > 0x0F);
          bool c = (res > 0xFF);
          
          R.set_flags(z, n, h, c);
          R.A = (uint8_t)(res & 0xFF);
     }

     void subing(uint8_t v){
          bool z = (R.A == v); // Si A == v, le résultat sera forcément 0
          bool n = true;
          bool h = ((R.A & 0x0F) < (v & 0x0F));
          bool c = (R.A < v);
          
          R.set_flags(z, n, h, c);
          R.A -= v;
     }


     void sbcing(uint8_t v){
          uint8_t c_in = (R.F >> 4) & 0x01;
          int res = R.A - v - c_in;
          
          bool z = ((res & 0xFF) == 0);
          bool n = true;
          bool h = ((R.A & 0x0F) < (v & 0x0F) + c_in);
          bool c = (R.A < v + c_in);
          
          R.set_flags(z, n, h, c);
          R.A = (uint8_t)(res & 0xFF);
     }

     void anding(uint8_t v){
          R.A &= v;
          bool z = (R.A == 0);
          R.set_flags(z, false, true, false);
     }

     void xoring(uint8_t v){
          R.A ^= v;
          bool z = (R.A == 0);
          R.set_flags(z, false, false, false);
     }

     void oring(uint8_t v){
          R.A |= v;
          bool z = (R.A == 0);
          R.set_flags(z, false, false, false);
     }

     void cp(uint8_t v){
          bool z = (R.A == v);
          bool n = true;
          bool h = ((R.A & 0x0F) < (v & 0x0F));
          bool c = (R.A < v);
          
          R.set_flags(z, n, h, c);
     }

     uint8_t inc8(uint8_t v){
      bool c=(R.F>>4)&0x01 ;
        bool h = ((v & 0x0F) == 0x0F);
      v++;
      bool z=(v==0);
      bool n=false;
      R.set_flags(z,n,h,c);
      return v;
     }

     uint8_t dec8(uint8_t v){
      bool c=(R.F>>4)&0x01 ;
      bool h = ((v & 0x0F) == 0x00);
      v--;
      bool z=(v==0);
      bool n=true;
      R.set_flags(z,n,h,c);
      return v;
     }

     uint16_t fetch16(){
      uint8_t low = Mem.read(R.PC++);
      uint8_t high = Mem.read(R.PC++);
      return (high << 8) | low;
     }

     uint16_t add16(uint16_t hl, uint16_t rr){
      bool h = ((hl & 0x0FFF) + (rr & 0x0FFF)) > 0x0FFF;
      bool z=(R.F>>7) & 0x01;
      bool n=false;
      bool c=(hl+rr)>0xFFFF;
      R.set_flags(z,n,h,c);
      return hl+rr;
     }

     void bit_test(uint8_t b, uint8_t r){
      bool z=!((r>>b)&0x01);
      bool c=(R.F>>4)&0x01;
      R.set_flags(z,false,true,c);
     }

     uint16_t target_addr;
     uint8_t offset;

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
            case 0x87:
               adding(R.A);clock+=4;
               break;
            case 0x80:
               adding(R.B);clock+=4;
               break;
            case 0x81:
               adding(R.C);clock+=4;
               break;
            case 0x82:
               adding(R.D);clock+=4;
               break;
            case 0x83:
               adding(R.E);clock+=4;
               break;
            case 0x84:
               adding(R.H);clock+=4;
               break;
            case 0x85:
               adding(R.L);clock+=4;
               break;
            case 0x86:
               adding(Mem.read(R.get_HL()));clock+=8;
               break;
            case 0xC6:
               adding(Mem.read(R.PC++));clock+=8;
               break;
            case 0x88:
               adcing(R.B);clock+=4;
               break;
            case 0x89:
               adcing(R.C);clock+=4;
               break;
            case 0x8A:
               adcing(R.D);clock+=4;
               break;
            case 0x8B:
               adcing(R.E);clock+=4;
               break;
            case 0x8C:
               adcing(R.H);clock+=4;
               break;
            case 0x8D:
               adcing(R.L);clock+=4;
               break;
            case 0x8E:
               adcing(Mem.read(R.get_HL()));clock+=8;
               break;
            case 0x8F:
               adcing(R.A);clock+=4;
               break;
            case 0xCE:
               adcing(Mem.read(R.PC++));clock+=8;
               break;
            case 0x90:
               subing(R.B);clock+=4;
               break;
            case 0x91:
               subing(R.C);clock+=4;
               break;
            case 0x92:
               subing(R.D);clock+=4;
               break;
            case 0x93:
               subing(R.E);clock+=4;
               break;
            case 0x94:
               subing(R.H);clock+=4;
               break;
            case 0x95:
               subing(R.L);clock+=4;
               break;
            case 0x96:
               subing(Mem.read(R.get_HL()));clock+=8;
               break;
            case 0x97:
               subing(R.A);clock+=4;
               break;
            case 0xD6:
               subing(Mem.read(R.PC++));clock+=8;
               break;
            case 0x98:
               sbcing(R.B);clock+=4;
               break;
            case 0x99:
               sbcing(R.C);clock+=4;
               break;
            case 0x9A:
               sbcing(R.D);clock+=4;
               break;
            case 0x9B:
               sbcing(R.E);clock+=4;
               break;
            case 0x9C:
               sbcing(R.H);clock+=4;
               break;
            case 0x9D:
               sbcing(R.L);clock+=4;
               break;
            case 0x9E:
               sbcing(Mem.read(R.get_HL()));clock+=8;
               break;
            case 0x9F:
               sbcing(R.A);clock+=4;
               break;
            case 0xDE:
               sbcing(Mem.read(R.PC++));clock+=8;
               break;
            case 0xA0:
               anding(R.B);clock+=4;
               break;
            case 0xA1:
               anding(R.C);clock+=4;
               break;
            case 0xA2:
               anding(R.D);clock+=4;
               break;
            case 0xA3:
               anding(R.E);clock+=4;
               break;
            case 0xA4:
               anding(R.H);clock+=4;
               break;
            case 0xA5:
               anding(R.L);clock+=4;
               break;
            case 0xA6:
               anding(Mem.read(R.get_HL()));clock+=8;
               break;
            case 0xA7:
               anding(R.A);clock+=4;
               break;
            case 0xE6:
               anding(Mem.read(R.PC++));clock+=8;
               break;
            case 0xA8:
               xoring(R.B);clock+=4;
               break;
            case 0xA9:
               xoring(R.C);clock+=4;
               break;
            case 0xAA:
               xoring(R.D);clock+=4;
               break;
            case 0xAB:
               xoring(R.E);clock+=4;
               break;
            case 0xAC:
               xoring(R.H);clock+=4;
               break;
            case 0xAD:
               xoring(R.L);clock+=4;
               break;
            case 0xAE:
               xoring(Mem.read(R.get_HL()));clock+=8;
               break;
            case 0xAF:
               xoring(R.A);clock+=4;
               break;
            case 0xEE:
               xoring(Mem.read(R.PC++));clock+=8;
               break;
            case 0xB0:
               oring(R.B);clock+=4;
               break;
            case 0xB1:
               oring(R.C);clock+=4;
               break;
            case 0xB2:
               oring(R.D);clock+=4;
               break;
            case 0xB3:
               oring(R.E);clock+=4;
               break;
            case 0xB4:
               oring(R.H);clock+=4;
               break;
            case 0xB5:
               oring(R.L);clock+=4;
               break;
            case 0xB6:
               oring(Mem.read(R.get_HL()));clock+=8;
               break;
            case 0xB7:
               oring(R.A);clock+=4;
               break;
            case 0xF6:
               oring(Mem.read(R.PC++));clock+=8;
               break;
            case 0xB8:
               cp(R.B);clock+=4;
               break;
            case 0xB9:
               cp(R.C);clock+=4;
               break;
            case 0xBA:
               cp(R.D);clock+=4;
               break;
            case 0xBB:
               cp(R.E);clock+=4;
               break;
            case 0xBC:
               cp(R.H);clock+=4;
               break;
            case 0xBD:
               cp(R.L);clock+=4;
               break;
            case 0xBE:
               cp(Mem.read(R.get_HL()));clock+=8;
               break;
            case 0xBF:
               cp(R.A);clock+=4;
               break;
            case 0xFE:
               cp(Mem.read(R.PC++));clock+=8;
               break;
            case 0x04:
               R.B = inc8(R.B); clock += 4;
               break;
            case 0x0C:
               R.C = inc8(R.C); clock += 4;
               break;
            case 0x14:
               R.D = inc8(R.D); clock += 4;
               break;
            case 0x1C:
               R.E = inc8(R.E); clock += 4;
               break;
            case 0x24:
               R.H = inc8(R.H); clock += 4;
               break;
            case 0x2C:
               R.L = inc8(R.L); clock += 4;
               break;
            case 0x34:
               { uint8_t val = Mem.read(R.get_HL()); Mem.write(R.get_HL(), inc8(val)); clock += 12; }
               break;
            case 0x3C:
               R.A = inc8(R.A); clock += 4;
               break;
            case 0x05:
               R.B = dec8(R.B); clock += 4;
               break;
            case 0x0D:
               R.C = dec8(R.C); clock += 4;
               break;
            case 0x15:
               R.D = dec8(R.D); clock += 4;
               break;
            case 0x1D:
               R.E = dec8(R.E); clock += 4;
               break;
            case 0x25:
               R.H = dec8(R.H); clock += 4;
               break;
            case 0x2D:
               { R.L = dec8(R.L); clock += 4; }
               break;
            case 0x35:
               { uint8_t val = Mem.read(R.get_HL()); Mem.write(R.get_HL(), dec8(val)); clock += 12; }
               break;
            case 0x3D:
               R.A = dec8(R.A); clock += 4;
               break;
            case 0xC3:
               R.PC=fetch16();clock+=16;
               break;
            case 0xC2:
               target_addr = fetch16(); if (!((R.F >> 7) & 0x01)) { R.PC = target_addr; clock += 16; } else { clock += 12; }
               break;
            case 0xCA:
               target_addr = fetch16(); if (((R.F >> 7) & 0x01)) { R.PC = target_addr; clock += 16; } else { clock += 12; }
               break;
            case 0xD2:
               target_addr = fetch16(); if (!((R.F >> 4) & 0x01)) { R.PC = target_addr; clock += 16; } else { clock += 12; }
               break;
            case 0xDA:
               target_addr = fetch16(); if (((R.F >> 4) & 0x01)) { R.PC = target_addr; clock += 16; } else { clock += 12; }
               break;
            case 0xE9:
               R.PC = R.get_HL(); clock += 4;
               break;
            case 0x18:
               offset = (int8_t)Mem.read(R.PC++); R.PC += offset; clock += 12;
               break;
            case 0x20:
               offset = (int8_t)Mem.read(R.PC++); if (!((R.F >> 7) & 0x01)) { R.PC += offset; clock += 12; } else { clock += 8; }
               break;
            case 0x28:
               offset = (int8_t)Mem.read(R.PC++); if (((R.F >> 7) & 0x01)) { R.PC += offset; clock += 12; } else { clock += 8; }
               break;
            case 0x30:
               offset = (int8_t)Mem.read(R.PC++); if (!((R.F >> 4) & 0x01)) { R.PC += offset; clock += 12; } else { clock += 8; }
               break;
            case 0x38:
               offset = (int8_t)Mem.read(R.PC++); if (((R.F >> 4) & 0x01)) { R.PC += offset; clock += 12; } else { clock += 8; }
               break;
            case 0xC5:
               Mem.write(--R.SP,R.B);Mem.write(--R.SP,R.C);clock+=16;
               break;
            case 0xD5:
               Mem.write(--R.SP, R.D); Mem.write(--R.SP, R.E); clock += 16;
               break;
            case 0xE5:
               Mem.write(--R.SP, R.H); Mem.write(--R.SP, R.L); clock += 16;
               break;
            case 0xF5:
               Mem.write(--R.SP, R.A); Mem.write(--R.SP, R.F); clock += 16;
               break;
            case 0xC1:
               R.C = Mem.read(R.SP++); R.B = Mem.read(R.SP++); clock += 16;
               break;
            case 0xD1:
               R.E = Mem.read(R.SP++); R.D = Mem.read(R.SP++); clock += 16;
               break;
            case 0xE1:
               R.L = Mem.read(R.SP++); R.H = Mem.read(R.SP++); clock += 16;
               break;
            case 0xF1:
               R.F = Mem.read(R.SP++) & 0xF0; R.A = Mem.read(R.SP++); clock += 16;
               break;
            case 0xCD:
               target_addr = fetch16(); Mem.write(--R.SP, (R.PC >> 8) & 0xFF); Mem.write(--R.SP, R.PC & 0xFF); R.PC = target_addr; clock += 24;
               break;
            case 0xC4:
               target_addr = fetch16(); if (!((R.F >> 7) & 0x01)) { Mem.write(--R.SP, (R.PC >> 8) & 0xFF); Mem.write(--R.SP, R.PC & 0xFF); R.PC = target_addr; clock += 24; } else { clock += 12; }
               break;
            case 0xCC:
               target_addr = fetch16(); if (((R.F >> 7) & 0x01)) { Mem.write(--R.SP, (R.PC >> 8) & 0xFF); Mem.write(--R.SP, R.PC & 0xFF); R.PC = target_addr; clock += 24; } else { clock += 12; }
               break;
            case 0xD4:
               target_addr = fetch16(); if (!((R.F >> 4) & 0x01)) { Mem.write(--R.SP, (R.PC >> 8) & 0xFF); Mem.write(--R.SP, R.PC & 0xFF); R.PC = target_addr; clock += 24; } else { clock += 12; }
               break;
            case 0xDC:
               target_addr = fetch16(); if (((R.F >> 4) & 0x01)) { Mem.write(--R.SP, (R.PC >> 8) & 0xFF); Mem.write(--R.SP, R.PC & 0xFF); R.PC = target_addr; clock += 24; } else { clock += 12; }
               break;
            case 0xC9:
               { uint8_t low = Mem.read(R.SP++); uint8_t high = Mem.read(R.SP++); R.PC = (high << 8) | low; clock += 16; }
               break;
            case 0xC0:
               if (!((R.F >> 7) & 0x01)) { uint8_t low = Mem.read(R.SP++); uint8_t high = Mem.read(R.SP++); R.PC = (high << 8) | low; clock += 20; } else { clock += 8; }
               break;
            case 0xC8:
               if (((R.F >> 7) & 0x01)) { uint8_t low = Mem.read(R.SP++); uint8_t high = Mem.read(R.SP++); R.PC = (high << 8) | low; clock += 20; } else { clock += 8; }
               break;
            case 0xD0:
               if (!((R.F >> 4) & 0x01)) { uint8_t low = Mem.read(R.SP++); uint8_t high = Mem.read(R.SP++); R.PC = (high << 8) | low; clock += 20; } else { clock += 8; }
               break;
            case 0xD8:
               if (((R.F >> 4) & 0x01)) { uint8_t low = Mem.read(R.SP++); uint8_t high = Mem.read(R.SP++); R.PC = (high << 8) | low; clock += 20; } else { clock += 8; }
               break;
            case 0x09:
               R.set_HL(add16(R.get_HL(), R.get_BC())); clock += 8;
               break;
            case 0x19:
               R.set_HL(add16(R.get_HL(), R.get_DE())); clock += 8;
               break;
            case 0x29:
               R.set_HL(add16(R.get_HL(), R.get_HL())); clock += 8;
               break;
            case 0x39:
               R.set_HL(add16(R.get_HL(), R.SP)); clock += 8;
               break;
            case 0x03:
               R.set_BC(R.get_BC() + 1); clock += 8;
               break;
            case 0x13:
               R.set_DE(R.get_DE() + 1); clock += 8;
               break;
            case 0x23:
               R.set_HL(R.get_HL() + 1); clock += 8;
               break;
            case 0x33:
               R.SP++; clock += 8;
               break;
            case 0x0B:
               R.set_BC(R.get_BC() - 1); clock += 8;
               break;
            case 0x1B:
               R.set_DE(R.get_DE() - 1); clock += 8;
               break;
            case 0x2B:
               R.set_HL(R.get_HL() - 1); clock += 8;
               break;
            case 0x3B:
               R.SP--; clock += 8;
               break;
            case 0xE8:
               { int8_t e = (int8_t)Mem.read(R.PC++); 
                 R.set_flags(false, false, ((R.SP & 0x0F) + (e & 0x0F)) > 0x0F, ((R.SP & 0xFF) + (e & 0xFF)) > 0xFF); 
                 R.SP += e; clock += 16; }
               break;
            case 0xF8:
               { int8_t e = (int8_t)Mem.read(R.PC++); 
                 R.set_flags(false, false, ((R.SP & 0x0F) + (e & 0x0F)) > 0x0F, ((R.SP & 0xFF) + (e & 0xFF)) > 0xFF); 
                 R.set_HL(R.SP + e); clock += 12; }
               break;
            case 0xCB:{
               uint8_t cb_opcode=Mem.read(R.PC++);
               switch(cb_opcode){
 case 0x40: bit_test(0, R.B); clock += 8; break;
case 0x41: bit_test(0, R.C); clock += 8; break;
case 0x42: bit_test(0, R.D); clock += 8; break;
case 0x43: bit_test(0, R.E); clock += 8; break;
case 0x44: bit_test(0, R.H); clock += 8; break;
case 0x45: bit_test(0, R.L); clock += 8; break;
case 0x46: bit_test(0, Mem.read(R.get_HL())); clock += 12; break;
case 0x47: bit_test(0, R.A); clock += 8; break;
case 0x48: bit_test(1, R.B); clock += 8; break;
case 0x49: bit_test(1, R.C); clock += 8; break;
case 0x4A: bit_test(1, R.D); clock += 8; break;
case 0x4B: bit_test(1, R.E); clock += 8; break;
case 0x4C: bit_test(1, R.H); clock += 8; break;
case 0x4D: bit_test(1, R.L); clock += 8; break;
case 0x4E: bit_test(1, Mem.read(R.get_HL())); clock += 12; break;
case 0x4F: bit_test(1, R.A); clock += 8; break;
case 0x50: bit_test(2, R.B); clock += 8; break;
case 0x51: bit_test(2, R.C); clock += 8; break;
case 0x52: bit_test(2, R.D); clock += 8; break;
case 0x53: bit_test(2, R.E); clock += 8; break;
case 0x54: bit_test(2, R.H); clock += 8; break;
case 0x55: bit_test(2, R.L); clock += 8; break;
case 0x56: bit_test(2, Mem.read(R.get_HL())); clock += 12; break;
case 0x57: bit_test(2, R.A); clock += 8; break;
case 0x58: bit_test(3, R.B); clock += 8; break;
case 0x59: bit_test(3, R.C); clock += 8; break;
case 0x5A: bit_test(3, R.D); clock += 8; break;
case 0x5B: bit_test(3, R.E); clock += 8; break;
case 0x5C: bit_test(3, R.H); clock += 8; break;
case 0x5D: bit_test(3, R.L); clock += 8; break;
case 0x5E: bit_test(3, Mem.read(R.get_HL())); clock += 12; break;
case 0x5F: bit_test(3, R.A); clock += 8; break;
case 0x60: bit_test(4, R.B); clock += 8; break;
case 0x61: bit_test(4, R.C); clock += 8; break;
case 0x62: bit_test(4, R.D); clock += 8; break;
case 0x63: bit_test(4, R.E); clock += 8; break;
case 0x64: bit_test(4, R.H); clock += 8; break;
case 0x65: bit_test(4, R.L); clock += 8; break;
case 0x66: bit_test(4, Mem.read(R.get_HL())); clock += 12; break;
case 0x67: bit_test(4, R.A); clock += 8; break;
case 0x68: bit_test(5, R.B); clock += 8; break;
case 0x69: bit_test(5, R.C); clock += 8; break;
case 0x6A: bit_test(5, R.D); clock += 8; break;
case 0x6B: bit_test(5, R.E); clock += 8; break;
case 0x6C: bit_test(5, R.H); clock += 8; break;
case 0x6D: bit_test(5, R.L); clock += 8; break;
case 0x6E: bit_test(5, Mem.read(R.get_HL())); clock += 12; break;
case 0x6F: bit_test(5, R.A); clock += 8; break;
case 0x70: bit_test(6, R.B); clock += 8; break;
case 0x71: bit_test(6, R.C); clock += 8; break;
case 0x72: bit_test(6, R.D); clock += 8; break;
case 0x73: bit_test(6, R.E); clock += 8; break;
case 0x74: bit_test(6, R.H); clock += 8; break;
case 0x75: bit_test(6, R.L); clock += 8; break;
case 0x76: bit_test(6, Mem.read(R.get_HL())); clock += 12; break;
case 0x77: bit_test(6, R.A); clock += 8; break;
case 0x78: bit_test(7, R.B); clock += 8; break;
case 0x79: bit_test(7, R.C); clock += 8; break;
case 0x7A: bit_test(7, R.D); clock += 8; break;
case 0x7B: bit_test(7, R.E); clock += 8; break;
case 0x7C: bit_test(7, R.H); clock += 8; break;
case 0x7D: bit_test(7, R.L); clock += 8; break;
case 0x7E: bit_test(7, Mem.read(R.get_HL())); clock += 12; break;
case 0x7F: bit_test(7, R.A); clock += 8; break;
               }
            }
            break;
              
            

              
               

               
        }
    }
};
