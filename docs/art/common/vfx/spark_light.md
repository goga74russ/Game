# Свет Искры по 8 стихиям — v0.1

**Из документов:** цвет Искры = стихия стартового бога; яркая плазма у Искры, тусклое тление у Скелета, у Плоти свечения нет (GDD §4). Ядро 3–4 см, оболочка Ø 0,35–0,5 м, парение на высоте ~1,1 м и выше (`hero_v0.1.md`). Цвета стихий — `style_v0.1.md` §4; знаки по краю — словарь рун. Параметр `SparkColor` в коде.

## Общее строение (одинаково для всех стихий)
- **Ядро** Ø 3–4 см, самая яркая точка, цвет «ядра» стихии. Никогда не чистый #FFFFFF.
- **Оболочка** Ø 0,35–0,5 м, мягкий шар плазмы, цвет «тела»; к краю прозрачна.
- **Язык** — оболочка вытянута на 20–30% против движения, как пламя свечи на сквозняке [предл.].
- **Шлейф** — 0,5–1 м, частицы формы стихии (ниже), живут 0,3–0,5 с [предл.].
- **Тёмная виньетка** внутри оболочки (кольцо на 70% радиуса, −20% яркости) — чтобы огонёк не терялся на светлом небе #A9C4D6 [предл., из hero_v0.1].
- **Скелет:** тот же цвет в черепе (огонёк «как в фонаре») и в центре груди, ~30% яркости Искры; в суставах едва тлеет.
- **Плоть:** свечения нет.

## Таблица стихий

| Стихия (бог) | Ядро | Оболочка | Форма частиц шлейфа | Знак по краю (для рун) | Особенность Искры [предл.] |
|---|---|---|---|---|---|
| Гром (Перун) — **демо** | #DDE4FF | #8FA8FF | ломаные микродуги 2–5 см, 1–2 кадра | зигзаг | по оболочке раз в 1–2 с пробегает дуга |
| Ветер (Стрибог) | #DDF2EA [предл.] | #A8D8C8, почти прозрачная | длинные тонкие штрихи, завихрения воздуха | завиток | оболочка дрожит, как марево |
| Солнце (Дажьбог) | #FFD9A0 [предл.] | #FFA62B | короткие горизонтальные лучи-лезвия веером | лучи | лучи только горизонтально (не столб — канал лута) |
| Жар (Сварог) | #FF6A1A (без светлого ядра) | #7A2A10 → #FF6A1A | тяжёлые искры, падают вниз по дуге | зубцы-язычки | без белого ядра (style §4) |
| Ртуть (Велес) | #D8DEE0 [предл.] | #B8C2C6 с отливом #8FA89A | тяжёлые капли, текут **вверх** | капли | оболочка — жидкий металл, рябь |
| Нити (Мокошь) | #EDE2C4 [предл.] | #D9C9A0 | тонкие линии-волоски, натянутые (всегда линии, не точки) | переплёт | вокруг огонька 3 тонкие нити уходят в стороны |
| Порча (Чернобог) | #6B6B2A | #2A2230 (тёмная!) | дым, осыпающийся чёрный песок | пятна | единственная «тёмная» Искра: светится только ядро, оболочка — дымный сгусток; для читаемости — тонкий оливковый край |
| Ярь-кровь (Ярило) | #8A1020 (без светлого ядра) | тёмная вишня #5E0E1A [предл.] | пульсирующие жилки, лепестки, пар | колосья | пульс сердцебиением ~1 Гц, **не** строб; уводить глубже в вишню при путанице (решение 2026-09-26) |

**Запреты:** у Жара и Яри нет светлого ядра (соседи канала 1); у Солнца нет вертикали (сосед лута); у Грома нет ровного пульса (сосед требы).

## Звук (словами)
Гром — сухое потрескивание; Ветер — тонкий свист; Солнце — высокий звон; Жар — шипение углей; Ртуть — густое бульканье; Нити — скрип струны; Порча — шорох песка; Ярь — приглушённое сердцебиение.

## Промпты

**Лист концептов (8 огоньков):**
```
Game VFX concept sheet, painterly photorealism, dark slate background, eight small floating soul-flames in a row, each a 4 cm bright core inside a soft translucent plasma sphere about 40 cm wide with a flame tongue trailing to one side and a short trail of particles: 1) cold lavender blue with tiny jagged micro-lightning, 2) pale sea-green almost transparent with long wind streaks, 3) amber with short horizontal blade rays, 4) orange-rust sparks falling in arcs with no white core, 5) silver-green liquid metal with heavy droplets flowing upward, 6) undyed linen beige with three thin taut threads, 7) tar-black smoke ball with an olive glowing core and falling black sand, 8) deep dark cherry with pulsing veins and steam, no white core. Each labeled only by position, even spacing, soft glow, no pure white anywhere. 16:9, no text.
```

**Искра Перуна в кадре (камера за плечом):**
```
Game character VFX concept, painterly photorealism, over-the-shoulder camera behind a small floating soul-flame hovering 1.1 meters above dry ochre grass: a 4 cm pale blue-white core (#DDE4FF) inside a 45 cm soft lavender blue plasma sphere (#8FA8FF) with a slightly darker inner ring, a flame tongue trailing backward, tiny jagged micro-arcs crawling over its surface. Ahead, a dry thundery Slavic highland village at dusk, pale storm-blue sky, colossal oak with a copper spire far away. 16:9, no text.
```

**Негатив:** общий (`README.md`) + `fairy, wisp with face, eyes, cute, gold sparkles`.
