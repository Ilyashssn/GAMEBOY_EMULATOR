#include <iostream>
#include "MMU.hpp"
#include "CPU.hpp"

//TEST
int main() {
    MMU mem;
    CPU cpu(mem);

    if (!mem.load_rom("C:/Users/acer/Downloads/cpu_instrs.gb")) {
        std::cerr << "Failed to load ROM file!" << std::endl;
        return 1;
    }

    while (cpu.get_running()) {
        cpu.step();
    }

    std::cout << "Execution stopped." << std::endl;
    return 0;
}



