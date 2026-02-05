# s21_grep

## Как работает

`src/main.c` отвечает за запуск, сбор шаблонов и компиляцию регулярного выражения.
Логика разнесена в `src/grep.c` и `src/grep.h`:

- `parse_flags` собирает флаги, шаблоны (`-e`) и файлы шаблонов (`-f`).
- `join_patterns` объединяет несколько шаблонов через `|`.
- `process_stream` читает поток построчно, применяет регулярку и учитывает флаги `-v`, `-c`, `-l`, `-n`, `-o`.

Поддерживаются флаги: `-e`, `-i`, `-v`, `-c`, `-l`, `-n`, `-h`, `-s`, `-f`, `-o` и их комбинации.

## Как проверить

Сборка:

```sh
make
```

Примеры запуска (используется тестовый файл `tests/sample.txt`):

```sh
./s21_grep hello tests/sample.txt
./s21_grep -in hello tests/sample.txt
./s21_grep -c -i foo tests/sample.txt
./s21_grep -n -o "o" tests/sample.txt
```
