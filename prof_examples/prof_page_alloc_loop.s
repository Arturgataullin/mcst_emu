# длинный цикл записи в разные 4 КБ страницы памяти
# первый проход выделяет блоки RAM, следующие проходы пишут в уже выделенные блоки

LI R7 0x0001
LI R8 0x1000
LI R4 0x1000
LI R5 0x00a5

# первый проход по страницам: каждая STW создает новый 4 КБ блок
LI R1 0xffff
MOV R3 R8
STW R5 R3 0x0
ADD R3 R3 R4
ADD R5 R5 R7
SUB R1 R1 R7
BRNZ R1 0xfffc

# повторные проходы по тем же адресам проверяют горячий путь без выделения блоков
LI R6 0x00ff
LI R1 0xffff
MOV R3 R8
STW R5 R3 0x0
ADD R3 R3 R4
ADD R5 R5 R7
SUB R1 R1 R7
BRNZ R1 0xfffc
SUB R6 R6 R7
BRNZ R6 0xfff8

MOV R9 R3
