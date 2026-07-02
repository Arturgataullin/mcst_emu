# Logical and arithmetic shifts
LFI R0 0x80000001
LI R1 0x0001
SLL R2 R1 R1
SRL R3 R0 R1
SRA R4 R0 R1

# Shift amounts are masked with 0x1f, so 0x20 behaves like zero
LI R5 0x0020
SLL R6 R1 R5
