# Померкшая Навь / Faded Nav

Сейчас разрабатываем **соло-демо Грозовых Высей в Яви**, биом Перуна. Прототип — 3D action RPG с изометрической камерой на Unreal Engine 5.8.

## С чего начать

1. [Что уже есть в проекте](docs/overview.md).
2. [Все документы демо Перуна](docs/demo/README.md).
3. [Объём демо](docs/slice_v1.md) и [действующие правила](docs/GDD_v1.md).

[Видение](docs/vision.md) задаёт направление игры. Статусы утверждения лора и предложений указаны внутри документов. [Журнал решений](memories/consensus.md) хранит договорённости.

## Запуск

- [OpenEditor.bat](game/FadedNav/OpenEditor.bat) открывает проект в UE.
- [Play.bat](game/FadedNav/Play.bat) собирает и запускает игру; редактор перед сборкой нужно закрыть.
- [Файл проекта](game/FadedNav/FadedNav.uproject).

Скрипты запуска сейчас используют локальный путь `D:\Epic Games\UE_5.8`. Часть моделей, анимаций и текстур установлена локально и исключена из публичного Git. См. [лицензии](docs/tech/assets_licenses.csv) и [подключение анимаций](docs/tech/hero_animation_v0.1.md).

## Папки

| Папка | Назначение |
|---|---|
| [docs](docs/README.md) | Правила, лор, уровни, арт, механики, технические заметки |
| game/FadedNav | Проект Unreal: Source, Config, Content |
| [tools](tools/README.md) | Скрипты подготовки и проверки |
| [references](references/README.md) | Внешние и общие референсы |
| memories | Журнал решений команды |
| .claude | Настройки и роли агентов |

Правила работы команды — [CLAUDE.md](CLAUDE.md).
