# Registers one CTest entry per test ROM found under TEST_ROMS_DIR.
# Tests labelled "known-failure" document current emulator limitations;
# CI runs `ctest -LE known-failure`.

function(rom_test NAME RUNNER ROM)
  cmake_parse_arguments(T "KNOWN_FAILURE" "" "ARGS" ${ARGN})
  set(path "${TEST_ROMS_DIR}/${ROM}")
  if(NOT EXISTS "${path}")
    return()
  endif()
  add_test(NAME ${NAME} COMMAND ${RUNNER} "${path}" ${T_ARGS})
  set(labels "")
  if(T_KNOWN_FAILURE)
    set(labels known-failure)
  endif()
  set_tests_properties(${NAME} PROPERTIES TIMEOUT 300 LABELS "${labels}")
endfunction()

# ---------------------------------------------------------------- NES ------
set(NES nes)
if(EXISTS "${TEST_ROMS_DIR}/nes/other/nestest.nes")
  add_test(NAME nes.nestest COMMAND nestest_cpu
           "${TEST_ROMS_DIR}/nes/other/nestest.nes"
           "${TEST_ROMS_DIR}/nes/other/nestest.log")
  set_tests_properties(nes.nestest PROPERTIES TIMEOUT 120)
endif()

rom_test(nes.instr_test.all_instrs   nes_rom_runner nes/instr_test-v5/all_instrs.nes     ARGS 4000)
rom_test(nes.instr_test.official     nes_rom_runner nes/instr_test-v5/official_only.nes   ARGS 4000)
rom_test(nes.instr_timing            nes_rom_runner nes/instr_timing/instr_timing.nes     ARGS 4000)
rom_test(nes.ppu_read_buffer         nes_rom_runner nes/ppu_read_buffer/test_ppu_read_buffer.nes ARGS 4000)
rom_test(nes.oam_read                nes_rom_runner nes/oam_read/oam_read.nes             ARGS 4000)
rom_test(nes.mmc3.1_clocking         nes_rom_runner nes/mmc3_test/1-clocking.nes          ARGS 4000)
rom_test(nes.mmc3.2_details          nes_rom_runner nes/mmc3_test/2-details.nes           ARGS 4000)
rom_test(nes.mmc3.3_a12_clocking     nes_rom_runner nes/mmc3_test/3-A12_clocking.nes      ARGS 4000)
rom_test(nes.mmc3.5_mmc3             nes_rom_runner nes/mmc3_test/5-MMC3.nes              ARGS 4000)

# Known limitations (PPU/CPU are not advanced on every bus cycle).
rom_test(nes.mmc3.4_scanline_timing  nes_rom_runner nes/mmc3_test/4-scanline_timing.nes  ARGS 4000 KNOWN_FAILURE)
rom_test(nes.instr_misc              nes_rom_runner nes/instr_misc/instr_misc.nes         ARGS 4000 KNOWN_FAILURE)

# ---------------------------------------------------------- Game Boy -------
set(GB_CPU gb/cpu_instrs/individual)
rom_test(gb.cpu_instrs.01_special        gb_rom_runner "${GB_CPU}/01-special.gb"              ARGS 8000)
rom_test(gb.cpu_instrs.03_op_sp_hl       gb_rom_runner "${GB_CPU}/03-op sp,hl.gb"             ARGS 8000)
rom_test(gb.cpu_instrs.04_op_r_imm       gb_rom_runner "${GB_CPU}/04-op r,imm.gb"             ARGS 8000)
rom_test(gb.cpu_instrs.05_op_rp          gb_rom_runner "${GB_CPU}/05-op rp.gb"                ARGS 8000)
rom_test(gb.cpu_instrs.06_ld_r_r         gb_rom_runner "${GB_CPU}/06-ld r,r.gb"               ARGS 8000)
rom_test(gb.cpu_instrs.07_jumps          gb_rom_runner "${GB_CPU}/07-jr,jp,call,ret,rst.gb"   ARGS 8000)
rom_test(gb.cpu_instrs.08_misc           gb_rom_runner "${GB_CPU}/08-misc instrs.gb"          ARGS 8000)
rom_test(gb.cpu_instrs.09_op_r_r         gb_rom_runner "${GB_CPU}/09-op r,r.gb"               ARGS 8000)
rom_test(gb.cpu_instrs.10_bit_ops        gb_rom_runner "${GB_CPU}/10-bit ops.gb"              ARGS 8000)
rom_test(gb.cpu_instrs.11_op_a_hl        gb_rom_runner "${GB_CPU}/11-op a,(hl).gb"            ARGS 8000)

# Known failures: timer/interrupt behaviour and M-cycle-accurate memory timing.
rom_test(gb.cpu_instrs.02_interrupts     gb_rom_runner "${GB_CPU}/02-interrupts.gb"           ARGS 8000 KNOWN_FAILURE)
rom_test(gb.instr_timing                 gb_rom_runner gb/instr_timing/instr_timing.gb        ARGS 8000 KNOWN_FAILURE)
rom_test(gb.mem_timing.01_read           gb_rom_runner gb/mem_timing-2/rom_singles/01-read_timing.gb   ARGS 8000 KNOWN_FAILURE)
rom_test(gb.mem_timing.02_write          gb_rom_runner gb/mem_timing-2/rom_singles/02-write_timing.gb  ARGS 8000 KNOWN_FAILURE)
rom_test(gb.mem_timing.03_modify         gb_rom_runner gb/mem_timing-2/rom_singles/03-modify_timing.gb ARGS 8000 KNOWN_FAILURE)

# ----------------------------------------------------------- CHIP-8 --------
# These ROMs report on screen only, so the final framebuffer hash is compared
# with a golden value (regenerate with: chip8_rom_runner <rom> 900 --ascii).
rom_test(chip8.chip8_logo  chip8_rom_runner chip8/bin/1-chip8-logo.ch8 ARGS 900 9ea572970c9477e5)
rom_test(chip8.ibm_logo    chip8_rom_runner chip8/bin/2-ibm-logo.ch8   ARGS 900 d9f1a1b88a998805)
rom_test(chip8.corax_plus  chip8_rom_runner chip8/bin/3-corax+.ch8     ARGS 900 6cc90ad4073d7a69)
