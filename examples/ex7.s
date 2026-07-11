# Unaligned word access
LI R0 0x0040
LFI R1 0x11223344

# This write touches bytes 0x41..0x44 and two internal 32-bit cells
STW R1 R0 0x1
LDW R2 R0 0x1
LDW R3 R0 0x0
LDW R4 R0 0x4
