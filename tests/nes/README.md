# NES test tools

Two small native programs check the NES core against public test ROMs. They
are built with `cmake -B build-tests -DBUILD_TESTS=ON` and run by CTest in CI
(see `../README.md`); the binaries end up in `build-tests/tests/`.

| Tool             | Purpose                                                        |
| ---------------- | -------------------------------------------------------------- |
| `nestest_cpu`    | Diffs the 6502 core with `nestest.log` (registers, flags, cycles) |
| `nes_rom_runner` | Runs a ROM headless and prints blargg's `$6000` result protocol |

The ROMs are not stored in the repository. They come from
<https://github.com/christopherpow/nes-test-roms>.

```sh
nestest_cpu nestest.nes nestest.log
nes_rom_runner instr_test-v5/all_instrs.nes          # exit 0 = pass
nes_rom_runner some.nes 600 screenshot.ppm           # frames limit + PPM dump
```

Older ROMs (2005 sprite-hit / PPU tests) report on screen only: dump a frame
and look at it (`$01` means *passed* for those).

## Current results

| ROM                                   | Result                                    |
| ------------------------------------- | ----------------------------------------- |
| nestest (8991 instr., incl. cycles)   | pass                                      |
| instr_test-v5 official_only / all_instrs | pass (16/16)                           |
| instr_timing                          | pass                                      |
| instr_misc                            | fails dummy reads of `(zp),Y` (test 3)    |
| ppu_read_buffer, oam_read             | pass                                      |
| sprite_hit_tests basics/alignment/timing | pass                                   |
| blargg_ppu_tests palette/sprite/vram  | pass                                      |
| mmc3_test 1, 2, 3, 5                  | pass                                      |
| mmc3_test 4 (scanline timing)         | fails: needs dot-exact PPU/CPU alignment  |
| ppu_vbl_nmi 2.., vbl_clear_time       | fail: need dot-exact PPU/CPU alignment    |
| apu_test 4-jitter, cpu_interrupts 2.. | fail: cycle-exact interrupt/APU timing    |

The PPU and APU are advanced after each whole CPU instruction (not on every
CPU bus cycle), which is why the cycle-exact tests fail. Commercial games do
not depend on that.
