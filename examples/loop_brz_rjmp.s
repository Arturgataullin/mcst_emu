# loop with BRZ + RJMP: factorial 4!
LI R0 0x0001
LI R1 0x0004
LI R2 0x0001

# loop test at byte address 0x0c
BRZ R1 0x0004
MUL R0 R0 R1
SUB R1 R1 R2
RJMP 0xfffd
