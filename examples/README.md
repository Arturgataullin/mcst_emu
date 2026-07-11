# Примеры программ

Каждый пример можно собрать и выполнить командами:

```bash
./build/assembler/assembler examples/ex1.s examples/ex1.o
./build/emulator/emulator examples/ex1.o
```

Для другого примера замените `ex1` на нужный номер.

## Содержание

- `ex1.s` - минимальный пример записи и чтения одного байта через RAM.
  Ключевой результат: `R1=0xab`, `R2=0xab`.
- `ex2.s` - вычитание, умножение, беззнаковое и знаковое деление.
  Ключевой результат: `R2=0x24`, `R3=0xd8`, `R4=0x24`,
  `R5=0xfffffffa`, `R6=0xffffffff`.
- `ex3.s` - `AND`, `OR`, `XOR`, `MOV` и `NOT`.
  Ключевой результат: `R2=0xf00f00`, `R3=0xfff00fff`,
  `R4=0xff0000ff`, `R5=0xffff00`.
- `ex4.s` - логические и арифметический сдвиги, включая маскирование
  величины сдвига. Ключевой результат: `R2=0x2`, `R3=0x40000000`,
  `R4=0xc0000000`, `R6=0x1`.
- `ex5.s` - все добавленные псевдокоманды в одной программе.
  Ключевой результат: `R0=0x12345678`, `R1=0x12345678`,
  `R2=0x12345677`.
- `ex6.s` - `STW`, `LDB`, `LDH`, `LDW`, `STH`, `STB` и little-endian
  порядок байтов. Ключевой результат: `R2=0x44`, `R3=0x2233`,
  `R4=0x11223344`, `R6=0xcdabcd`.
- `ex7.s` - невыровненный `STW`, который меняет байты в двух соседних
  внутренних 32-битных ячейках. Ключевой результат: `R2=0x11223344`,
  `R3=0x22334400`, `R4=0x11`.
- `ex8.s` - `SXT` и `BSWAP` с разными режимами. Ключевой результат:
  `R1=0xffffffff`, `R2=0xffff80ff`, `R5=0x11224433`,
  `R6=0x44332211`.
- `ex9.s` - пример для предупреждений о чтении неинициализированной RAM.
  Запускать полезнее с `--warn=uninit-ram`.

## Полезные запуски

Обычный запуск:

```bash
./build/assembler/assembler examples/ex6.s examples/ex6.o
./build/emulator/emulator examples/ex6.o
```

Трассировка команд:

```bash
./build/emulator/emulator --trace=disasm examples/ex6.o
```

Трассировка записей в RAM с фильтром по байтовым адресам:

```bash
./build/assembler/assembler examples/ex7.s examples/ex7.o
./build/emulator/emulator --trace=ram-wr --trace-ram-addrs=65-68 examples/ex7.o
```

Совместная фильтрация по тактам и адресам:

```bash
./build/emulator/emulator --trace=disasm,ram-wr --trace-ticks=2-4 --trace-ram-addrs=64-68 examples/ex7.o
```

Предупреждения о чтении неинициализированной RAM:

```bash
./build/assembler/assembler examples/ex9.s examples/ex9.o
./build/emulator/emulator --warn=uninit-ram examples/ex9.o
```

Файлы `.o` являются генерируемыми и не хранятся в Git.
