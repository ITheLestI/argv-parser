# Парсер аргументов командной строки

## Задача

Спроектировать и реализовать класс для парсинга аргументов командной строки.

## Примечания

Изначально парсер умеет обрабатывать только строки, целочисленные и булевые аргументы. Также есть возможность добавить произвольные типы и расширить функционал.

Реализация находится в библиотеке [argparser](lib/CMakeLists.txt). 

### Тесты

Проект покрыт тестами с использованием *google test*

### Примеры запуска

Пример программы с использованием парсера находится в [bin](bin/main.cc). Программа умеет складывать или умножать переданные ей аргументы.

`labwork4 --sum 1 2 3 4 5`

`labwork4 --mult 1 2 3 4 5`

Типы и поведение аргументов задаются с помощью соответствующих методов класса `ExactArgument`