# Сполох (flare) — облик v0.1

> Черновик art-director. Цифры — `skills.csv`, строка `flare`. **[предл.]** — решает директор. Сетка руны — `README.md`.

**Механика:** метка «чары», глагол «рывок». Ярь 25, КД 7 с. **Рывок 6 м сквозь врагов**, **i-frames 0,25 с**, разряд по пути 60% (15 урона). Время самого рывка в таблице не задано — [предл.] 0,2 с (укладывается в i-frames).

## 1. Руна-иконка
- **Рамка:** круг Ø 204, линия 14 px.
- **Узор «рывок» — стрела с хвостом:** горизонтальная стрела слева направо, общая длина 124 px, древко 12 px; наконечник — открытый угол 60°, стороны по 34 px; хвост — три параллельные короткие черты 12 px длиной 26, 20, 14 px, отстоят друг от друга на 8 px и тянутся за древком влево, укорачиваясь [предл.]. Стрела чуть наклонена вверх на 10° [предл.].
- **Знак по краю:** зигзаг по окружности.
- **Дверь:** стрела прорезана, хвостовые черты — мелкими насечками разной глубины.

## 2. Эффект в бою

| Фаза | Время | Что видно |
|---|---|---|
| Старт | 0,05 с [предл.] | На месте героя короткая вспышка-хлопок: сжатая сфера искр Ø ~1 м #8FA8FF, фигура героя на миг распадается на ломаные линии |
| Рывок | 0,2 с [предл.]; неуязвимость 0,25 с | Героя почти не видно: вместо тела — **лента-разрыв** шириной с плечи (~0,5 м) и высотой роста, прямая, 6 м, как след от ногтя по воздуху. Внутри ленты — 3–4 продольные ломаные дуги #DDE4FF/#8FA8FF. Враги на пути: в момент касания — вспышка-«укол» Ø 0,4 м на корпусе врага и короткая дуга от ленты к нему |
| Финиш | 0,1 с | Герой проявляется в конечной точке, вокруг ног — кольцо искр Ø 1,5 м, пыль |
| Затухание следа | 0,6 с [предл.] | Лента гаснет от начала к концу (хвост тает первым), оставляя в воздухе тонкую дрожащую линию и тающие искры; на земле вдоль пути — прерывистая полоса пыли и мелких прожилок, 3 с |

**По стадиям.**
- **Искра:** у неё и так телепорт; Сполох отличается тем, что след **виден весь путь**: цепочка из 5–6 затухающих копий огонька (каждая меньше и бледнее предыдущей), соединённых нитью молнии [предл.].
- **Скелет:** в ленте на миг видны 2 полупрозрачных «отпечатка» скелета (начало и середина пути), кости выбелены светом; стук костей при финише.
- **Плоть:** 2 отпечатка силуэта тела, матовые, в цвет ленты, без свечения кожи; при финише герой гасит инерцию, чуть проскальзывая.

## 3. Читаемость
- **Против наставника:** у наставника рывков нет; красно-белого нет.
- **Против молний мира:** горизонталь, длина ровно 6 м, начало и конец у героя. Мировая молния — вертикаль с телеграфом.
- **Против уклонения (идеальное уклонение):** у идеального уклонения замедление и вспышка «точно!», у Сполоха — лента-разрыв и нет замедления. Если Сполох совпадёт с идеальным уклонением, играют оба: сначала вспышка «точно!», потом лента.

## 4. Звук
Старт — сухой хлопок, как рвут плотную ткань. Рывок — короткий свист-шипение (0,2 с). Каждый задетый враг — отдельный щелчок-укол (так слышно, сколько задел). Финиш — мягкий удар пыли; у Скелета — стук костей.

## 5. Промпты

**Концепт эффекта (Плоть, камера за плечом):**
```
Game VFX concept art, painterly photorealism, dark fantasy Slavic myth, over-the-shoulder third-person camera. A lean man in a coarse undyed linen shirt dashes forward 6 meters in a straight line through a group of faded grey-white humanoid enemies. His path is a tall narrow tear in the air, shoulder-wide and man-high, filled with 3 long jagged lavender blue lightning streaks (#8FA8FF) with pale blue cores (#DDE4FF), two translucent matte afterimages of his silhouette inside the streak, small bright sparks where the streak touches each enemy. He reappears at the far end with a ring of sparks and dust at his feet. The tail end of the streak already dissolving. Dry thunderstorm dusk, no rain, warm low side light on ochre dry grass, background brighter than figures. 16:9, no text.
```

**Иконка-руна:**
```
Flat game UI rune icon, 256x256, centered on a dark slate (#1E2230) square tile. An outer circle frame, thick carved lavender blue (#8FA8FF) stroke with a thin pale blue (#DDE4FF) core line and a thin sharp zigzag around its outside. Inside: a horizontal arrow pointing right, tilted slightly upward, with an open 60-degree arrowhead and three short parallel speed strokes of decreasing length trailing behind it. Chiseled straight line ends, flat vector, no perspective, faint outer glow.
```

**Негатив (сцена):** `red, white, pure white, gold, neon, cyberpunk, motion blur of a sports car, teleport sci-fi portal, rain, wet, blood, text, logo, cartoon, anime`
