# Building a 32-bit value with LI/LUI and wrap-around arithmetic
LI R0 0xffff
LUI R0 0xffff
LI R1 0x0001
ADD R2 R0 R1
SUB R3 R2 R1
