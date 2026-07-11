# Uninitialized RAM warning example
# Run with: emulator --warn=uninit-ram examples/ex9.o
LI R0 0x0080
LDW R1 R0 0x0

LI R2 0x00AA
STB R2 R0 0x1
LDW R3 R0 0x0
