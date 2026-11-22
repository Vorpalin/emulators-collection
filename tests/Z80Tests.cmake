# Registers the Z80 CP/M exerciser tests (needs rom_test() from RomTests.cmake,
# so include this file after it).
#
# The process fails if the output contains "ERROR" or "failed". zexdoc covers
# the documented instructions and flags, zexall the undocumented flag bits too.
# They execute ~47 billion T-states (a few minutes natively), hence the long
# timeout.
rom_test(z80.prelim z80_cpm_runner z80/roms/prelim.com)
rom_test(z80.zexdoc z80_cpm_runner z80/roms/zexdoc.cim)
rom_test(z80.zexall z80_cpm_runner z80/roms/zexall.cim)
if(TEST z80.zexdoc)
  set_tests_properties(z80.zexdoc z80.zexall PROPERTIES TIMEOUT 3600)
endif()
