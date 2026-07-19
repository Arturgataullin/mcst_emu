# длинный цикл чтения и записи RAM
# этот пример сильнее нагружает Memory::read32/write32 и разреженную память
LI R1 0xffff
LUI R1 0x00ff
LI R2 0x0001
LI R3 0x0100
LI R4 0x0001

# тело цикла начинается по байтовому адресу 0x14
STW R4 R3 0x0
LDW R5 R3 0x0
ADD R4 R4 R5
SUB R1 R1 R2
BRNZ R1 0xfffc

MOV R9 R4
