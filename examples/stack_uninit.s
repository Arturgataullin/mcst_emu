LI R5 0xc000
SCRW SP_TOP R5

LI R5 0x4000
SCRW SP_SIZE R5

ASPI R6 0xfffc

LFI R7 0x11223344
STW R7 R6 0x0

# диапазон 0xbffc...0xbfff снова считается неинициализированным
ASPI R8 0x0004

# намеренно читаю освобождённую память
# --warn=uninit-ram напечатает предупреждение для всех четырёх байтов
LDW R9 R6 0x0
