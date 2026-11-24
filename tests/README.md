# Tests

Native test runners that execute public test ROMs against the emulator cores
and report the result through CTest. The ROMs are **not** stored in the
repository: `fetch_roms.sh` downloads them at pinned commits.

```sh
sh tests/fetch_roms.sh build-tests/test-roms
cmake -S . -B build-tests -G Ninja -DBUILD_TESTS=ON
cmake --build build-tests
cd build-tests
ctest -LE known-failure --output-on-failure   # what CI runs
ctest -L known-failure                        # documented limitations
```

Use `-DTEST_ROMS_DIR=<dir>` to point CMake at another ROM directory. Tests whose
ROM is missing are simply not registered.

| Runner             | System    | How the result is read                                           |
| ------------------ | --------- | ---------------------------------------------------------------- |
| `nestest_cpu`      | NES       | Diffs the 6502 core with `nestest.log` (registers, flags, cycles) |
| `nes_rom_runner`   | NES       | Blargg's `$6000` protocol                                         |
| `gb_rom_runner`    | Game Boy  | Serial output (`Passed`/`Failed`) or Blargg's `$A000` RAM protocol |
| `gb_frame_runner`  | Game Boy / Color | Hash of the final framebuffer compared with a golden value (cgb-acid2) |
| `cgb_features`     | Game Boy Color | Speed switch, WRAM/VRAM banking, HDMA and palettes on synthetic ROMs, no download needed |
| `chip8_rom_runner` | CHIP-8    | Hash of the final framebuffer compared with a golden value        |
| `battery_roundtrip` | GB, NES  | Battery RAM API (change detection, export/restore) on tiny synthetic ROMs, no download needed |
| `z80_core`         | Z80       | Results, flags, T-state timings, interrupts and undocumented opcodes on hand-assembled programs, no download needed |
| `z80_cpm_runner`   | Z80       | Runs the CP/M `prelim`, `zexdoc` and `zexall` exercisers and fails on any `ERROR` in the output |
| `sega_vdp_core`    | Master System / Game Gear | Mode 4 rendering (tiles, scroll, sprites, priority, masks), ports, flags, interrupts and counters through the CPU-visible ports, no download needed |

## Known failures

Tests labelled `known-failure` fail today and are excluded from CI. They
document current limitations; when one starts passing, remove its
`KNOWN_FAILURE` flag in `RomTests.cmake`. Current list: NES `mmc3_test`
4-scanline_timing and `instr_misc` (see `nes/README.md`), Game Boy
`cpu_instrs/02-interrupts`, `instr_timing` and `mem_timing-2`.

## Adding a test

Add a `rom_test(...)` line to `RomTests.cmake`. For on-screen-only ROMs
(CHIP-8), check the screen by eye first, then record the hash printed by
`chip8_rom_runner <rom> 900 --ascii` as the golden value.
