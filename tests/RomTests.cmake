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

# Baseline, not triaged yet: APU and PPU vblank/NMI accuracy ROMs (blargg, report
# through $6000 like the tests above). They have not been run against the emulator:
# drop KNOWN_FAILURE from every test that passes, so that it guards against
# regressions in CI, and keep it on the others as a to-do list.
set(NES_APU nes/apu_test/rom_singles)
rom_test(nes.apu_test.1_len_ctr          nes_rom_runner "${NES_APU}/1-len_ctr.nes"          ARGS 4000 KNOWN_FAILURE)
rom_test(nes.apu_test.2_len_table        nes_rom_runner "${NES_APU}/2-len_table.nes"        ARGS 4000 KNOWN_FAILURE)
rom_test(nes.apu_test.3_irq_flag         nes_rom_runner "${NES_APU}/3-irq_flag.nes"         ARGS 4000 KNOWN_FAILURE)
rom_test(nes.apu_test.4_jitter           nes_rom_runner "${NES_APU}/4-jitter.nes"           ARGS 4000 KNOWN_FAILURE)
rom_test(nes.apu_test.5_len_timing       nes_rom_runner "${NES_APU}/5-len_timing.nes"       ARGS 4000 KNOWN_FAILURE)
rom_test(nes.apu_test.6_irq_flag_timing  nes_rom_runner "${NES_APU}/6-irq_flag_timing.nes"  ARGS 4000 KNOWN_FAILURE)
rom_test(nes.apu_test.7_dmc_basics       nes_rom_runner "${NES_APU}/7-dmc_basics.nes"       ARGS 4000 KNOWN_FAILURE)
rom_test(nes.apu_test.8_dmc_rates        nes_rom_runner "${NES_APU}/8-dmc_rates.nes"        ARGS 4000 KNOWN_FAILURE)

set(NES_VBL nes/ppu_vbl_nmi/rom_singles)
rom_test(nes.ppu_vbl_nmi.01_vbl_basics       nes_rom_runner "${NES_VBL}/01-vbl_basics.nes"       ARGS 4000 KNOWN_FAILURE)
rom_test(nes.ppu_vbl_nmi.02_vbl_set_time     nes_rom_runner "${NES_VBL}/02-vbl_set_time.nes"     ARGS 4000 KNOWN_FAILURE)
rom_test(nes.ppu_vbl_nmi.03_vbl_clear_time   nes_rom_runner "${NES_VBL}/03-vbl_clear_time.nes"   ARGS 4000 KNOWN_FAILURE)
rom_test(nes.ppu_vbl_nmi.04_nmi_control      nes_rom_runner "${NES_VBL}/04-nmi_control.nes"      ARGS 4000 KNOWN_FAILURE)
rom_test(nes.ppu_vbl_nmi.05_nmi_timing       nes_rom_runner "${NES_VBL}/05-nmi_timing.nes"       ARGS 4000 KNOWN_FAILURE)
rom_test(nes.ppu_vbl_nmi.06_suppression      nes_rom_runner "${NES_VBL}/06-suppression.nes"      ARGS 4000 KNOWN_FAILURE)
rom_test(nes.ppu_vbl_nmi.07_nmi_on_timing    nes_rom_runner "${NES_VBL}/07-nmi_on_timing.nes"    ARGS 4000 KNOWN_FAILURE)
rom_test(nes.ppu_vbl_nmi.08_nmi_off_timing   nes_rom_runner "${NES_VBL}/08-nmi_off_timing.nes"   ARGS 4000 KNOWN_FAILURE)
rom_test(nes.ppu_vbl_nmi.09_even_odd_frames  nes_rom_runner "${NES_VBL}/09-even_odd_frames.nes"  ARGS 4000 KNOWN_FAILURE)
rom_test(nes.ppu_vbl_nmi.10_even_odd_timing  nes_rom_runner "${NES_VBL}/10-even_odd_timing.nes"  ARGS 4000 KNOWN_FAILURE)

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

# Baseline, not triaged yet (same convention as the NES block above): halt/interrupt
# timing and the DMG sound tests, which are the first accuracy checks for the APU.
rom_test(gb.halt_bug                     gb_rom_runner gb/halt_bug.gb                         ARGS 8000 KNOWN_FAILURE)
rom_test(gb.interrupt_time               gb_rom_runner gb/interrupt_time/interrupt_time.gb    ARGS 8000 KNOWN_FAILURE)

set(GB_SOUND gb/dmg_sound/rom_singles)
rom_test(gb.dmg_sound.01_registers          gb_rom_runner "${GB_SOUND}/01-registers.gb"          ARGS 8000 KNOWN_FAILURE)
rom_test(gb.dmg_sound.02_len_ctr            gb_rom_runner "${GB_SOUND}/02-len ctr.gb"            ARGS 8000 KNOWN_FAILURE)
rom_test(gb.dmg_sound.03_trigger            gb_rom_runner "${GB_SOUND}/03-trigger.gb"            ARGS 8000 KNOWN_FAILURE)
rom_test(gb.dmg_sound.04_sweep              gb_rom_runner "${GB_SOUND}/04-sweep.gb"              ARGS 8000 KNOWN_FAILURE)
rom_test(gb.dmg_sound.05_sweep_details      gb_rom_runner "${GB_SOUND}/05-sweep details.gb"      ARGS 8000 KNOWN_FAILURE)
rom_test(gb.dmg_sound.06_overflow_on_trig   gb_rom_runner "${GB_SOUND}/06-overflow on trigger.gb" ARGS 8000 KNOWN_FAILURE)
rom_test(gb.dmg_sound.07_len_sweep_sync     gb_rom_runner "${GB_SOUND}/07-len sweep period sync.gb" ARGS 8000 KNOWN_FAILURE)
rom_test(gb.dmg_sound.08_len_ctr_power      gb_rom_runner "${GB_SOUND}/08-len ctr during power.gb" ARGS 8000 KNOWN_FAILURE)
rom_test(gb.dmg_sound.09_wave_read_on       gb_rom_runner "${GB_SOUND}/09-wave read while on.gb" ARGS 8000 KNOWN_FAILURE)
rom_test(gb.dmg_sound.10_wave_trigger_on    gb_rom_runner "${GB_SOUND}/10-wave trigger while on.gb" ARGS 8000 KNOWN_FAILURE)
rom_test(gb.dmg_sound.11_regs_after_power   gb_rom_runner "${GB_SOUND}/11-regs after power.gb" ARGS 8000 KNOWN_FAILURE)
rom_test(gb.dmg_sound.12_wave_write_on      gb_rom_runner "${GB_SOUND}/12-wave write while on.gb" ARGS 8000 KNOWN_FAILURE)

# ----------------------------------------------------------- CHIP-8 --------
# These ROMs report on screen only, so the final framebuffer hash is compared
# with a golden value (regenerate with: chip8_rom_runner <rom> 900 --ascii).
rom_test(chip8.chip8_logo  chip8_rom_runner chip8/bin/1-chip8-logo.ch8 ARGS 900 9ea572970c9477e5)
rom_test(chip8.ibm_logo    chip8_rom_runner chip8/bin/2-ibm-logo.ch8   ARGS 900 d9f1a1b88a998805)
rom_test(chip8.corax_plus  chip8_rom_runner chip8/bin/3-corax+.ch8     ARGS 900 6cc90ad4073d7a69)
