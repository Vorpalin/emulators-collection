# Native test runners. ROMs are not stored in the repository: point
# TEST_ROMS_DIR at a directory created by tests/fetch_roms.sh (default:
# <build dir>/test-roms). Tests whose ROM is missing are simply not registered.

set(TEST_ROMS_DIR "${CMAKE_BINARY_DIR}/test-roms" CACHE PATH
    "Directory containing the downloaded test ROMs")

add_executable(nes_rom_runner nes/nes_rom_runner.cc)
target_link_libraries(nes_rom_runner PRIVATE nes)

add_executable(nestest_cpu nes/nestest_cpu.cc)
target_link_libraries(nestest_cpu PRIVATE cpu65)

add_executable(gb_rom_runner gameboy/gb_rom_runner.cc)
target_link_libraries(gb_rom_runner PRIVATE gameboy)

add_executable(chip8_rom_runner chip8/chip8_rom_runner.cc)
target_link_libraries(chip8_rom_runner PRIVATE chip8)

include(${CMAKE_CURRENT_LIST_DIR}/RomTests.cmake)
