#pragma once
#include <cstdint>
#include <vector>

class MMU;
class CPU;

class PPU{
    private:
       MMU& Mem;
       CPU& cpu;

       int ppu_cycles=0;
       uint8_t lcdc = 0x91; 
       uint8_t stat = 0x85; 
       uint8_t ly = 0;     
       uint8_t lyc = 0;    
       uint32_t screen_buffer[160*144];
       
       uint8_t get_mode() const;
       void set_mode(uint8_t mode);
       bool is_lcd_enabled() const;
       void check_lyc();
       void render_background();
       void render_sprites();

    public:
       PPU(MMU& shared_mem, CPU& shared_cpu);
    ~PPU() = default;
    bool frame_ready=false;
    void reset();
    void step(int cycles);

    const uint32_t* get_screen_buffer() const { return screen_buffer; }
};