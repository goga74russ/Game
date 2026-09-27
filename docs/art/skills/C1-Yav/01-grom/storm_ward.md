# Оберег грозы (storm_ward) — облик v0.1

> Черновик art-director. Цифры — `skills.csv`, строка `storm_ward`. **[предл.]** — решает директор. Сетка руны — `README.md`.

**Механика:** метка «чары», глагол «щит». Ярь 35, КД 18 с. **Щит 25% макс. HP на 5 с**; при пробитии — **разряд R 4 м**, 40 урона. Камень с Козьего вожака (опционально).

## 1. Руна-иконка
- **Рамка:** круг Ø 204, линия 14 px.
- **Узор «щит» — полукруг-чаша:** полуокружность Ø 110, линия 12 px, открыта **вверх** (чаша), концы обрублены горизонтально на уровне центра холста; над чашей, по центру, короткая вертикальная черта 12×30 px (то, что чаша ловит) [предл.; если лишнее — убрать].
- **Знак по краю:** зигзаг по окружности.
- **Дверь:** чаша прорезана широкой бороздой, дно чаши — гладкое углубление.

## 2. Эффект в бою

| Фаза | Время | Что видно |
|---|---|---|
| Наложение | 0,3 с [предл.] | Вокруг героя с земли поднимаются 6 вертикальных ломаных дуг (по шести лучам громового знака) и смыкаются над головой в **полусферу-купол Ø ~2,4 м** [предл.] (у Искры Ø ~1,2 м). |
| Держится | до 5 с (из таблицы) | Купол — не стекло и не гладкий пузырь, а **решётка из тонких ломаных молний**, сплетённая в узор шестилучевого громового знака; ячейки прозрачные. Яркость низкая (≤40% от пика), по решётке медленно бегут искры. По земле под куполом — круг прожилок. **Заряд = плотность решётки:** 100% — сетка частая; по мере урона нити гаснут одна за другой, решётка редеет. Каждое попадание — вспышка-рябь в точке удара Ø 0,5 м. За 1 с до конца 5 с — решётка начинает рваться по краям (предупреждение) |
| Пробитие | 0,1 с | Решётка лопается: все нити одновременно распрямляются наружу — **кольцо дуг по земле до R 4 м**, как у Громового удара, но полный круг; яркая вспышка 2 кадра. На земле — обугленное кольцо R 4 м, 6 с [предл.] |
| Истёк без пробития | 0,5 с [предл.] | Нити тихо гаснут сверху вниз и уходят в землю, без разряда (так видно, что разряда не было) |

**По стадиям.** Размер купола под стадию: Искра — Ø ~1,2 м вокруг огонька; Скелет и Плоть — Ø ~2,4 м. У Скелета тление в груди на время щита ровно на 60%; у Плоти тело не светится. Цвет и решётка одинаковые.

## 3. Читаемость
- **Против требы (главный риск):** купол вокруг героя может напомнить «ореол». Треба — **ровное серебро #CFE3F2 и медленный пульс**; купол — **лаванда, решётка из ломаных линий, мерцание**, никогда не сплошной светящийся шар. Серебра в куполе нет.
- **Против Суда наставника:** Суд — красно-белый импульс по силуэту наставника и круг R 10; купол — мелкая лавандовая решётка вокруг героя.
- **Против молний мира:** купол не бьёт сверху; разряд при пробитии — горизонтальное кольцо от героя.

## 4. Звук
Наложение — нарастающее жужжание, как у пчёл в улье, и сухой щелчок смыкания. Держится — тихое ровное потрескивание. Попадание по куполу — звонкий «тинь» с треском. Пробитие — хруст ломающейся решётки + хлёсткий треск разряда. Истечение — затухающее шипение.

## 5. Промпты

**Концепт эффекта (Плоть, камера за плечом):**
```
Game VFX concept art, painterly photorealism, dark fantasy Slavic myth, over-the-shoulder third-person camera behind a lean man in a coarse undyed linen shirt with rawhide bracers, holding a wood-splitting axe. Around him a hemispherical dome about 2.4 meters wide woven from thin jagged lavender blue lightning threads (#8FA8FF) with pale blue cores (#DDE4FF), the threads forming a six-petal thunder rosette lattice pattern, cells transparent, low glow, a ripple flash where an enemy claw hits it. A faded grey-white humanoid enemy strikes the dome. Branching glowing cracks on dry ground inside the dome. Dry thunderstorm dusk, no rain, warm low side light, background brighter than figures. 16:9, no text.
```

**Концепт пробития:**
```
Same scene, the moment the lightning lattice dome shatters: all threads snap outward into a full ring of low jagged ground arcs spreading 4 meters around the man, enemies thrown back, dust and static, a fresh dark scorch ring on the dry grass. Brightest moment, cold lavender blue, no pure white. 16:9, no text.
```

**Иконка-руна:**
```
Flat game UI rune icon, 256x256, centered on a dark slate (#1E2230) square tile. An outer circle frame, thick carved lavender blue (#8FA8FF) stroke with thin pale blue (#DDE4FF) core line and a thin sharp zigzag around its outside. Inside: a half circle open upward like a bowl, with a short vertical stroke above its center. Chiseled straight line ends, flat vector, no perspective, faint outer glow.
```

**Негатив (сцена):** `red, white, silver glow, soap bubble, glass sphere, hexagon sci-fi force field, neon, cyberpunk, gold, rain, wet, blood, text, logo, cartoon, anime`
