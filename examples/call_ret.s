# function call with CALL and RET
LI R5 0x0014
LI R0 0x0007
CALL R5
MOV R3 R0
RJMP 0x0003

# double(x): R0 = R0 * 2
ADD R0 R0 R0
RET
