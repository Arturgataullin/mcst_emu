# loop with BRNZ: countdown sum 5 + 4 + 3 + 2 + 1
LI R0 0x0000
LI R1 0x0005
LI R2 0x0001

# loop body at byte address 0x0c
ADD R0 R0 R1
SUB R1 R1 R2
BRNZ R1 0xfffe
