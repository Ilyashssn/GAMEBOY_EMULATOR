#include <SDL2/SDL.h>
#include <iostream>
#include <string>

#include "CPU.hpp"
#include "MMU.hpp"
#include "PPU.hpp"

const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 144;
const int WINDOW_SCALE = 4;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_rom>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string rom_path = argv[1];

    MMU mmu;
    if (!mmu.load_rom(rom_path)) {
        std::cerr << "Erreur : Impossible de charger la ROM : " << rom_path << std::endl;
        return EXIT_FAILURE;
    }

    CPU cpu(mmu);
    PPU ppu(mmu, cpu);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Erreur SDL_Init : " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }

    SDL_Window* window = SDL_CreateWindow(
        "GB Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH * WINDOW_SCALE, SCREEN_HEIGHT * WINDOW_SCALE,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Erreur Window : " << SDL_GetError() << std::endl;
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        std::cerr << "Erreur Renderer : " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT
    );

    if (!texture) {
        std::cerr << "Erreur Texture : " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    bool quit = false;
    SDL_Event event;

    while (cpu.get_running() && !quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        int previous_cycles = cpu.get_clock();
        cpu.step();
        int cycles_executed = cpu.get_clock() - previous_cycles;

        if (cycles_executed > 0) {
            ppu.step(cycles_executed);
        }

        if (ppu.frame_ready) {
            ppu.frame_ready = false;

            SDL_UpdateTexture(
                texture, 
                nullptr, 
                ppu.get_screen_buffer(), 
                SCREEN_WIDTH * sizeof(uint32_t)
            );

            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}