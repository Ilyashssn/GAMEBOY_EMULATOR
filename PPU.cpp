#include "PPU.hpp"
#include "MMU.hpp"
#include "CPU.hpp"

PPU::PPU(MMU& shared_mem, CPU& shared_cpu) : Mem(shared_mem), cpu(shared_cpu) {
    reset();
}

void PPU::reset() {
    ppu_cycles = 0;
    lcdc = 0x91;
    stat = 0x85;
    ly = 0;
    lyc = 0;
    for (int i = 0; i < 160 * 144; i++) {
        screen_buffer[i] = 0xFFFFFFFF; 
    }
}

uint8_t PPU::get_mode() const {
    return stat & 0x03;
}

void PPU::set_mode(uint8_t mode) {
    stat = (stat & 0xFC) | (mode & 0x03);
}

bool PPU::is_lcd_enabled() const {
    return (lcdc & 0x80) != 0; 
}

void PPU::check_lyc() {
    if (ly == lyc) {
        stat |= 0x04; 
        if (stat & 0x40) {
            cpu.request_interrupt(CPU::INTERRUPT_LCDSTAT); 
        }
    } else {
        stat &= ~0x04; 
    }
}

void PPU::step(int cycles) {
    if (!is_lcd_enabled()) {
        ppu_cycles = 0;
        ly = 0;
        set_mode(0);
        return;
    }

    ppu_cycles += cycles;

    if (ly < 144) {
        if (ppu_cycles < 80) {
            if (get_mode() != 2) {
                set_mode(2);
                if (stat & 0x20) cpu.request_interrupt(CPU::INTERRUPT_LCDSTAT);
            }
        }
        else if (ppu_cycles < 252) {
            if (get_mode() != 3) {
                set_mode(3);
            }
        }
        else if (ppu_cycles < 456) {
            if (get_mode() != 0) {
                set_mode(0);
                if (stat & 0x08) cpu.request_interrupt(CPU::INTERRUPT_LCDSTAT);
                render_background();
                render_sprites();
            }
        }
        else {
            ppu_cycles -= 456;
            ly++;
            check_lyc();
        }
    }
    else {
        if (get_mode() != 1) {
            set_mode(1);
            cpu.request_interrupt(CPU::INTERRUPT_VBLANK);
            if (stat & 0x10) cpu.request_interrupt(CPU::INTERRUPT_LCDSTAT);
        }

        if (ppu_cycles >= 456) {
            ppu_cycles -= 456;
            ly++;
            
            if (ly > 153) {
                ly = 0;
                frame_ready=true;
            }
            
            check_lyc();
        }

        
    }
}

void PPU::render_background() {
    uint8_t scy = Mem.read(0xFF42);
    uint8_t scx = Mem.read(0xFF43);
    uint8_t bgp = Mem.read(0xFF47);
    uint8_t wy = Mem.read(0xFF4A);
    uint8_t wx = Mem.read(0xFF4B);

    bool bg_enabled = (lcdc & 0x01) != 0;
    bool win_enabled = (lcdc & 0x20) != 0 && (ly >= wy);

    if (!bg_enabled) {
        return;
    }

    uint8_t bg_y_pos = scy + ly;
    uint16_t bg_tile_row = (bg_y_pos / 8) * 32;
    uint16_t bg_tile_map_base = (lcdc & 0x08) ? 0x9C00 : 0x9800;
    uint16_t bg_tile_map_line_addr = bg_tile_map_base + bg_tile_row;
    uint8_t bg_tile_line = bg_y_pos % 8;

    uint16_t win_tile_map_base = (lcdc & 0x40) ? 0x9C00 : 0x9800;
    uint16_t win_tile_row = ((ly - wy) / 8) * 32;
    uint16_t win_tile_map_line_addr = win_tile_map_base + win_tile_row;
    uint8_t win_tile_line = (ly - wy) % 8;

    for (int x = 0; x < 160; x++) {
        bool use_window = win_enabled && (x >= (wx - 7));

        uint16_t tile_map_line_addr = 0;
        uint16_t tile_col = 0;
        uint8_t tile_line = 0;
        int bit_shift = 0;

        if (use_window) {
            tile_map_line_addr = win_tile_map_line_addr;
            tile_col = (x - (wx - 7)) / 8;
            tile_line = win_tile_line;
            bit_shift = 7 - ((x - (wx - 7)) % 8);
        } else {
            tile_map_line_addr = bg_tile_map_line_addr;
            uint8_t x_pos = scx + x;
            tile_col = x_pos / 8;
            tile_line = bg_tile_line;
            bit_shift = 7 - (x_pos % 8);
        }

        uint16_t tile_address = tile_map_line_addr + tile_col;
        uint8_t tile_index = Mem.read(tile_address);

        uint16_t tile_data_addr = 0;
        if (lcdc & 0x10) {
            tile_data_addr = 0x8000 + (tile_index * 16);
        } else {
            int8_t signed_index = static_cast<int8_t>(tile_index);
            tile_data_addr = 0x9000 + (signed_index * 16);
        }

        tile_data_addr += (tile_line * 2);

        uint8_t byte1 = Mem.read(tile_data_addr);
        uint8_t byte2 = Mem.read(tile_data_addr + 1);

        uint8_t lsb = (byte1 >> bit_shift) & 0x01;
        uint8_t msb = (byte2 >> bit_shift) & 0x01;
        uint8_t color_index = (msb << 1) | lsb;

        uint8_t color_shade = (bgp >> (color_index * 2)) & 0x03;

        uint32_t rgb_color = 0xFFFFFFFF;
        switch (color_shade) {
            case 0: rgb_color = 0xFFFFFFFF; break;
            case 1: rgb_color = 0xB3B3B3FF; break;
            case 2: rgb_color = 0x5A5A5AFF; break;
            case 3: rgb_color = 0x000000FF; break;
        }

        screen_buffer[ly * 160 + x] = rgb_color;
    }
}

void PPU::render_sprites() {
    bool sprites_enabled = (lcdc & 0x02) != 0;
    if (!sprites_enabled) {
        return;
    }

    bool sprite_size_16 = (lcdc & 0x04) != 0;
    int sprite_height = sprite_size_16 ? 16 : 8;
    int sprites_drawn = 0;

    for (int i = 0; i < 40; i++) {
        uint16_t oam_addr = 0xFE00 + (i * 4);
        int sprite_y = Mem.read(oam_addr) - 16;
        int sprite_x = Mem.read(oam_addr + 1) - 8;
        uint8_t tile_index = Mem.read(oam_addr + 2);
        uint8_t attributes = Mem.read(oam_addr + 3);

        if (ly < sprite_y || ly >= (sprite_y + sprite_height)) {
            continue;
        }

        sprites_drawn++;
        if (sprites_drawn > 10) {
            break;
        }

        if (sprite_x < -7 || sprite_x >= 160) {
            continue;
        }

        bool priority = (attributes & 0x80) != 0;
        bool y_flip = (attributes & 0x40) != 0;
        bool x_flip = (attributes & 0x50) != 0;
        uint16_t palette_addr = (attributes & 0x10) ? 0xFF49 : 0xFF48;
        uint8_t obp = Mem.read(palette_addr);

        int line = ly - sprite_y;
        if (y_flip) {
            line = sprite_height - 1 - line;
        }

        if (sprite_size_16) {
            tile_index &= 0xFE;
            if (line >= 8) {
                tile_index |= 0x01;
                line -= 8;
            }
        }

        uint16_t tile_data_addr = 0x8000 + (tile_index * 16) + (line * 2);
        uint8_t byte1 = Mem.read(tile_data_addr);
        uint8_t byte2 = Mem.read(tile_data_addr + 1);

        for (int tile_x = 0; tile_x < 8; tile_x++) {
            int pixel_x = sprite_x + tile_x;
            if (pixel_x < 0 || pixel_x >= 160) {
                continue;
            }

            int bit_shift = x_flip ? tile_x : 7 - tile_x;
            uint8_t lsb = (byte1 >> bit_shift) & 0x01;
            uint8_t msb = (byte2 >> bit_shift) & 0x01;
            uint8_t color_index = (msb << 1) | lsb;

            if (color_index == 0) {
                continue;
            }

            uint8_t color_shade = (obp >> (color_index * 2)) & 0x03;
            uint32_t rgb_color = 0xFFFFFFFF;
            switch (color_shade) {
                case 0: rgb_color = 0xFFFFFFFF; break;
                case 1: rgb_color = 0xB3B3B3FF; break;
                case 2: rgb_color = 0x5A5A5AFF; break;
                case 3: rgb_color = 0x000000FF; break;
            }

            if (priority) {
                uint32_t current_pixel = screen_buffer[ly * 160 + pixel_x];
                if (current_pixel != 0xFFFFFFFF) {
                    continue;
                }
            }

            screen_buffer[ly * 160 + pixel_x] = rgb_color;
        }
    }
}