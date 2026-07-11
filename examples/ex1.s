# Store and load one byte through RAM
LI R0 0x0040
LI R1 0xAB
STB R1 R0 0x0
LDB R2 R0 0x0
