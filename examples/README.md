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
- `ex10.s` - переходы `RJMP`, `BRZ`, `BRNZ`, `AJMP`, `CALL`, псевдокоманда `RET`
  и сравнения `EQ`, `NE`, `LT`, `GE`, `SLT`, `SGE`. Ключевой результат:
  `R3=0x7`, `R4=0x5`, `R7=0x1`, `R8=0x0`, `R9=0x1`, `R12=0x1`, `R13=0x0`.
- `call_ret.s` - простой вызов функции через `CALL` с возвратом через `RET`.
  Функция удваивает значение в `R0`. Ключевой результат: `R0=0xe`, `R3=0xe`.
- `recursive_factorial.s` - рекурсивный `fact(4)` с ручным сохранением адреса возврата.
  Перед вложенным `CALL` программа сохраняет `R1` и текущий аргумент в стек, потому что
  `CALL` перезаписывает `R1`. Ключевой результат: `R0=0x18`, `R9=0x18`.
- `loop_brnz.s` - цикл-счётчик через `BRNZ` и отрицательное относительное смещение.
  Ключевой результат: `R0=0xf`, `R1=0x0`.
- `loop_brz_rjmp.s` - цикл вида `while`: выход через `BRZ`, возврат к проверке через `RJMP`.
  Ключевой результат: `R0=0x18`, `R1=0x0`.
- `loop_ajmp.s` - цикл с абсолютным переходом `AJMP` по адресу, заранее загруженному в регистр.
  Ключевой результат: `R0=0xa`, `R1=0x4`, `R6=0x0`.
- `stack.s` - инициализация `SP_TOP`/`SP_SIZE`, выделение через `ASPI`, запись
  значения и освобождение стека. Ключевой результат: `R6=0xbff0`, `R8=0xc000`,
  `R9=0xc000`, `R10=0x4000`.
- `stack_aspr.s` - та же последовательность со знаковым смещением в регистре и
  командами `ASPR`. Ключевой результат: `R6=0xbff0`, `R9=0xc000`,
  `R10=0xc000`, `R11=0x4000`.
- `stack_uninit.s` - намеренное чтение слова после освобождения стека для
  демонстрации дополнительного задания `--warn=uninit-ram`. Без очистки
  `R9=0x11223344`; с `--clear-stack` значение `R9=0x0`, но в обоих случаях
  выводится предупреждение о четырёх освобождённых байтах.

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

Переходы, вызов подпрограммы и сравнения:

```bash
./build/assembler/assembler examples/ex10.s examples/ex10.o
./build/emulator/emulator --trace=disasm examples/ex10.o
```

Обычный вызов функции через `CALL`/`RET`:

```bash
./build/assembler/assembler examples/call_ret.s examples/call_ret.o
./build/emulator/emulator --trace=disasm examples/call_ret.o
```

Рекурсивный вызов с сохранением адреса возврата в стеке:

```bash
./build/assembler/assembler examples/recursive_factorial.s examples/recursive_factorial.o
./build/emulator/emulator --trace=disasm --ram-size=1024 examples/recursive_factorial.o
```

В этом примере `R1` используется как регистр адреса возврата. Каждый вложенный `CALL`
перезаписывает `R1`, поэтому перед рекурсивным вызовом текущий `R1` нужно сохранить
в памяти, а после возврата восстановить перед `RET`.

Три варианта циклов без меток:

```bash
./build/assembler/assembler examples/loop_brnz.s examples/loop_brnz.o
./build/emulator/emulator --trace=disasm examples/loop_brnz.o

./build/assembler/assembler examples/loop_brz_rjmp.s examples/loop_brz_rjmp.o
./build/emulator/emulator --trace=disasm examples/loop_brz_rjmp.o

./build/assembler/assembler examples/loop_ajmp.s examples/loop_ajmp.o
./build/emulator/emulator --trace=disasm examples/loop_ajmp.o
```

Пример стека требует RAM не меньше `0x10000` байт:

```bash
./build/assembler/assembler examples/stack.s examples/stack.o
./build/emulator/emulator --ram-size=65536 examples/stack.o
```

Трассировка очистки освобождённого стека:

```bash
./build/emulator/emulator --ram-size=65536 --clear-stack --trace=disasm,ram-wr examples/stack.o
```

Пример смещения стека из регистра:

```bash
./build/assembler/assembler examples/stack_aspr.s examples/stack_aspr.o
./build/emulator/emulator --ram-size=65536 examples/stack_aspr.o
```

Предупреждение при чтении освобождённой памяти без обнуления:

```bash
./build/assembler/assembler examples/stack_uninit.s examples/stack_uninit.o
./build/emulator/emulator --ram-size=65536 --warn=uninit-ram examples/stack_uninit.o
```

Ожидаемая часть предупреждения:

```text
read 4 byte(s) at 0x0000bffc uninit: 0x0000bffc,0x0000bffd,0x0000bffe,0x0000bfff
```

Тот же пример с предварительным обнулением освобождённого диапазона:

```bash
./build/emulator/emulator --ram-size=65536 --clear-stack --trace=ram-wr --warn=uninit-ram examples/stack_uninit.o
```

## Примеры для профилирования

Benchmark-примеры для `perf` и `callgrind` вынесены в [`prof_examples`](../prof_examples/README.md).
