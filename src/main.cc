#include "chip8.hh"
#include "emulator.hh"

#include <iostream>
#include <string>
#include <memory>

int main()
{
    std::unique_ptr<Emulator> emulator =
        std::make_unique<Chip8>();
    
    std::string filename;
    std::cout << "Enter the name of the game you want to play: ";
    std::getline(std::cin, filename);

    emulator->loadProgram(filename);

    emulator->run();
    return 0;
}
