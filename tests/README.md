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
| `chip8_rom_runner` | CHIP-8    | Hash of the final framebuffer compared with a golden value        |
| `battery_roundtrip` | GB, NES  | Battery RAM API (change detection, export/restore) on tiny synthetic ROMs, no download needed |

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
