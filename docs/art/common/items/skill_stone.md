# Камень-навык на земле

> Общие правила и негатив — `README.md`. Руна = цвет (стихия) + узор (глагол) + рамка (метка) — `docs/art/skills/README.md`, `docs/systems/skills_demo_v0.1.md` §3. Камни вставляются в панель 1–3, не в оружие (GDD §4–6). Облики конкретных рун — у второго арт-директора в `docs/art/skills/`; здесь только **носитель**.

- **Что это:** предмет, который даёт навык. Один облик-носитель на все камни, отличается только руной на лицевой грани.
- **Форма [предл.]:** **окатанная речная галька-«пуговица»**, плоская, 9×7×3 см, одна грань стёсана в ровную площадку, на ней вырезана руна. Формы галька не меняет — меняется рамка, вырезанная вокруг узора: квадрат (удар), ромб (выстрел), круг (чары). Так рамку видно и на земле, и в HUD.
- **Слои материалов:**
  1. Тело — тёмный плотный камень #3E3C3A, по бокам окатан, матовый.
  2. Лицевая площадка — стёсана, светлее #5A5854, мелкая насечка.
  3. Руна — врезана на 2 мм, **в прорези светится цвет стихии**: для Перуна #8FA8FF, ядро #DDE4FF по дну прорези; свечение не выходит за край камня (без ореола — ореол у требы).
  4. Знак стихии по краю (для дальтоников): у грома — **зигзаг**, врезанный по кольцу вокруг рамки, без свечения.
- **Износ:** сколы по кромке, прожилка кварца #8E8A80 наискось, пыль в насечке.
- **Палитра:** #3E3C3A, #5A5854, #8E8A80, свет руны #8FA8FF / #DDE4FF.
- **Руны демо (для сверки; облик узоров — `docs/art/skills/`):**

| Камень | Рамка | Узор | Где лежит |
|---|---|---|---|
| Громовой удар | квадрат | круг с точкой | Сухоречье |
| Громоотвод | круг | крест в круге | Присяжный камень |
| Сполох | круг | стрела с хвостом | Присяжный камень |
| Цепная искра | ромб | три точки на линии | Жильный узел |
| Оберег грозы | круг | полукруг-чаша | Козий вожак |

- **На земле [предл.]:** лежит лицом вверх, чуть наклонён к игроку; столб #FFC24A (как всякий лут) + **кольцо у основания цвета стихии** #8FA8FF, приглушённого до 60%. Руна в прорези мерцает 1–2 кадрами раз в 3 с (гром мерцает, треба — нет).
- **В руке / при подборе:** камень подлетает к ладони, руна вспыхивает и «перетекает» в слот HUD — тот же узор и рамка в слоте.
- **С камеры за плечом:** с 3 м виден тёмный овал с цветной рамкой — рамка (квадрат/ромб/круг) читается раньше узора.
- **Звук [предл.]:** подбор — треск разряда + низкий каменный стук.

## Промпт — концепт (картинка)
```
Flat river-worn dark stone pebble 9 by 7 by 3 cm, one face ground flat and slightly lighter with fine tooling marks, a simple rune carved 2 mm deep into the flat face: a square frame enclosing a circle with a centre dot, the carved grooves glowing softly with cold lavender-blue light #8FA8FF, brighter pale lavender at the bottom of the grooves, glow stays inside the grooves, no halo; a small carved zigzag border ring around the frame without glow, chipped edges, a diagonal quartz vein. Matte stone. Isolated object, centered, neutral mid-grey seamless background, soft even studio light from upper left, no cast shadow on background, painterly photorealism, dark folk-fantasy, 3/4 view.
```
Для других камней менять только фразу про рамку и узор (`a diamond frame enclosing three dots connected by a line` и т. д. по таблице).
## Meshy
```
Flat worn dark river pebble 9x7x3 cm with one ground flat face bearing a carved square frame and a circle with a centre dot, grooves 2 mm deep, zigzag ring carved around the frame, chipped edges, quartz vein. Emissive mask only in the rune grooves. Single object, PBR textures, no baked lighting, no shadows on texture, clean topology, real-world scale.
```
