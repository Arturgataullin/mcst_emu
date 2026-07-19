# Примеры для профилирования

Эти файлы не являются обычными демонстрационными программами. Они специально делают длинные циклы, чтобы эмулятор работал достаточно долго для `perf` и `callgrind`.

Запускайте их без трассировки, иначе профиль будет в основном показывать стоимость вывода в поток.

## Сборка

Профилировать лучше сборку `RelWithDebInfo`: оптимизации включены, но имена функций остаются доступны для `perf` и `callgrind`.

```bash
cmake -S . -B build-profile -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-profile
```

Собрать все `.s` файлы из этой директории в `.o`:

```bash
./assemble_dir.sh prof_examples
```

## Быстрая проверка времени

```bash
time ./build-profile/emulator/emulator prof_examples/prof_arith_loop.o
time ./build-profile/emulator/emulator prof_examples/prof_memory_loop.o
time ./build-profile/emulator/emulator --ram-size=268435456 prof_examples/prof_page_alloc_loop.o
```

## perf

Собрать `perf stat` для всех profiling-примеров:

```bash
./profile_perf_stat.sh profile_res_before
```

Собрать `perf stat` для одного примера:

```bash
./profile_perf_stat.sh profile_res_before prof_memory_loop
```

Для детального интерактивного профиля:

```bash
perf record -g ./build-profile/emulator/emulator prof_examples/prof_arith_loop.o
perf report
```

## callgrind

Собрать `callgrind` и сразу получить `callgrind_annotate`:

```bash
./profile_callgrind.sh profile_res_before
./profile_callgrind.sh profile_res_before prof_memory_loop
```

Первый аргумент скриптов - директория для результатов. Второй аргумент необязательный: если он не задан, обрабатываются все `prof_examples/*.s`; если задано имя, например `prof_memory_loop`, профилируется только этот пример.

## Как выбирать пример

- `prof_arith_loop.s` - базовая стоимость `step()`, `decode()` и `execute()`.
- `prof_branch_loop.s` - условные переходы и частые проверки условия цикла.
- `prof_ajmp_loop.s` - абсолютный переход через регистр.
- `prof_memory_loop.s` - чтение/запись RAM и накладные расходы разреженной памяти.
- `prof_page_alloc_loop.s` - первый проход выделяет 4 КБ блоки RAM, повторные проходы проверяют горячую запись.
- `prof_call_loop.s` - `CALL`, `RET` и сохранение адреса возврата в `R1`.

Для `prof_page_alloc_loop` нужен увеличенный размер RAM. Скрипты `profile_perf_stat.sh` и `profile_callgrind.sh` передают `--ram-size=268435456` автоматически.

Файлы `.o` являются генерируемыми и не хранятся в Git.
