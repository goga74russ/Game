# Навыки: механика (таблицы)

**Источник правды по цифрам — CSV в этой папке** (удобно вести таблицей, открывается в Excel/Google Sheets; потом импортируется в Unreal как DataTable). Описание замысла и обоснование — `docs/systems/skills_demo_v0.1.md`. Облик и руны — `docs/art/skills/`.

Разбивка по главам: `C1-Yav/`, `C2-Nav/`, `C3-Prav/`; в каждой `skills.csv` (камни) и `supports.csv` (поддержки). Колонка `biome` — биом бога, откуда камень (`01-grom` … `08-zhar`).

## Колонки `skills.csv`
| Колонка | Что | Пример |
|---|---|---|
| id | код камня (латиница) | thunder_strike |
| name_ru | название | Громовой удар |
| chapter / biome | где падает | C1-Yav / 01-grom |
| element | стихия | гром |
| source | откуда | Перун (старт), Жильный узел, полубог… |
| tag | метка: strike (удар) / shot (выстрел) / spell (чары) | strike |
| rune_verb | узор руны (глагол): площадь, обездвиживание, цепь, рывок, щит… | площадь |
| rank | D / C / B / A / S | D |
| yar_cost | цена в Яри | 20 |
| cooldown_s | перезарядка, с | 4 |
| damage | урон | 1.8 |
| damage_basis | от чего урон: weapon_melee (доля урона ближнего оружия), weapon_ranged, flat (абсолют) | weapon_melee |
| radius_m / range_m / arc_deg | радиус, дальность, дуга | 4 / – / 120 |
| delay_s | задержка до срабатывания | 1.5 |
| duration_s | длительность эффекта | 5 |
| stun_s | оглушение | 0.8 |
| ammo_cost | патроны | 1 |
| iframes_s | неуязвимость | 0.25 |
| notes | особенности по стадиям и прочее | |
| status | idea / draft / in_game | in_game |

Все числа — допущения [Д] до плейтеста. Правило: меняем цифру — сначала здесь, потом в коде (`Source/FadedNav/Private/FNSkills.cpp`, `FNCharacter.cpp::CastSkill`).
