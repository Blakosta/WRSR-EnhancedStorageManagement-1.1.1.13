# WRSR Enhanced Storage Management — адаптация для 1.1.1.13

Этот репозиторий предназначен для отдельной разработки и проверки адаптации мода Enhanced Storage Management для Workers & Resources: Soviet Republic версии 1.1.1.13.

## Статус

- Оригинальный upstream: [Ultimate-Universe/WRSR-EnhancedStorageManagement](https://github.com/Ultimate-Universe/WRSR-EnhancedStorageManagement)
- Исходная совместимость upstream: игра 1.1.1.9
- Целевая версия: игра 1.1.1.13
- Адаптация: примерно 70%; основные игровые хуки и значительная часть структур подтверждены по бинарникам 1.1.1.13. Generic storage UI найден по ABI вызывающего кода.
- Повторная Windows-сборка после переноса generic storage UI hook на `0x706DA0` прошла CI (run [45494164122](https://github.com/Blakosta/WRSR-EnhancedStorageManagement-1.1.1.13/actions/runs/45494164122)). Это проверка сборки, не совместимости с игрой.
- Исправлена проверка рецептов: у входных записей rate находится на `+0x08`, у выходных — на `+0x0C`; сборка после изменения прошла CI (run [45551620139](https://github.com/Blakosta/WRSR-EnhancedStorageManagement-1.1.1.13/actions/runs/45551620139)).
- Подтверждены поля allocation-записей и таблица resource coefficients; подтверждён ID здания `+0x5F4`.
- Анализ бинарников и SHA-256 собранного кандидата: [docs/BINARY_ANALYSIS_1.1.1.13.md](docs/BINARY_ANALYSIS_1.1.1.13.md).
- Ещё проверяются вспомогательные хуки, allocation-vector end `+0x20`, координаты зданий и другие оставшиеся поля. После этого — манифест и тест в игре.
- Непроверенные адреса не должны использоваться в runtime-патчах.

## Принцип работы

Оригинальная реализация сохраняется как отправная точка. Изменения для 1.1.1.13 будут фиксироваться отдельными коммитами. Релизная DLL не считается готовой до успешной сборки и проверки в игре.

## Лицензия

Исходный проект распространяется под GNU GPL v3. См. файл LICENSE после импорта исходников.
