# Громоотвод (lightning_rod) — облик v0.1

> Черновик art-director. Цифры — `skills.csv`, строка `lightning_rod`. **[предл.]** — решает директор. Сетка руны — `README.md`.

**Механика:** метка «чары», глагол «обездвиживание». Ярь 30, КД 10 с. Стержень в точку прицела, **дальность 8 м**; через **1,5 с** удар молнии **R 3 м**, 45 урона, **оглушение 0,8 с**. Боссу — стак оглушения 30% (не сделано), иммунитет 15 с. Замысел: «наставник бьёт молнией в точку, Громоотвод — та же атака в руках игрока» (`skills_demo_v0.1.md`).

## 1. Руна-иконка
- **Рамка:** круг Ø 204, линия 14 px.
- **Узор «обездвиживание» — крест в круге:** внутренняя окружность Ø 110, линия 12 px; внутри косой крест (X) из двух линий 12 px, концы не доходят до окружности на 10 px [предл.: косой, а не прямой — чтобы не читался как прицел или церковный крест; вопрос директору].
- **Знак по краю:** зигзаг по окружности (18 px шаг ≈ 36 зубцов).
- **Цвет, состояния:** общие. Дверь: X прорезан глубже окружности, в точке пересечения — медная шпилька-вставка Ø 8 мм, матовая патина #6FA890 [предл.] (отсылка к стержню).

## 2. Эффект в бою

| Фаза | Время | Что видно |
|---|---|---|
| Бросок / вбивание | 0,3 с [предл.] | Герой делает жест «сверху вниз» к точке прицела. В точке (до 8 м) из земли с сухим стуком встаёт **стержень**: кованый железный штырь высотой ~1,2 м [предл.], чёрно-бурый #2E2620 с медной патиной #6FA890 на навершии; навершие — маленький шестилучевой громовой знак Ø 12 см. Стержень чуть наклонён (5–8°), вокруг основания трескается сухая земля |
| Телеграф | 1,5 с (из таблицы) | От основания стержня по земле **расходится наружу** кольцо до **R 3 м**. Кольцо — не сплошная линия, а цепочка из 12 коротких резных насечек-зигзагов #8FA8FF шириной ~8 см, между ними — тонкая линия 50%. Внутри круга по земле 4 тонкие радиальные прожилки ползут от стержня к краю (рисунок «крест в круге» проступает на земле [предл.]). По стержню снизу вверх бегут искры, навершие наливается светом от 20% до 100% за 1,5 с. Яркость телеграфа низкая (≤50% пика эффекта), чтобы не спорить с атаками врагов |
| Удар | 0,1 с вспышка | С неба (высота 25–40 м, из облака) **в навершие стержня** бьёт вертикальная ветвистая молния толщиной ~0,3 м в стволе, #DDE4FF ядро / #8FA8FF тело. От стержня по земле кольцом за 0,1 с разбегаются дуги до края R 3 м. Самая яркая точка кадра на 2 кадра, но без белого сигнала и без импульса по силуэту (это приём канала 1) |
| Оглушение и затухание | 0,8 с оглушение; следы 1,5 с [предл.] | Оглушённые враги: по контуру тела медленно ползают 2–3 лавандовые искры, враг застывает в позе «одёрнули за жилы». Стержень раскалён до тусклой лаванды на 1,5 с, потом рассыпается в ржавую труху [предл.]. На земле — круг-стекло: выжженное кольцо R 3 м, в центре фульгуритовое пятно #8C9A8E с трубками-стекляшками, держится 8 с [предл.] |

**По стадиям.** Облик одинаковый (чары от оружия не зависят). Отличие только в жесте: Искра — огонёк на миг «стекает» вниз к точке тонкой нитью света; Скелет — тычок пальцем вниз, суставы вспыхивают до 60%; Плоть — ладонь вниз, жест без свечения [предл.].

## 3. Читаемость
- **Против молний наставника и жил (самый опасный конфликт):** та же форма «круг + удар сверху». Различаем 4 каналами:
  1. **Движение круга:** наш — **расходится наружу** от стержня; вражеский (см. `common/vfx/lightning_telegraph.md`) — **сжимается к центру**.
  2. **Предмет в центре:** у нас всегда стоит стержень; у врага центр пуст (выжженная точка).
  3. **Цвет:** наш — лаванда #8FA8FF с насечками; вражеский — стальной голубой #6E9CC8 сплошным кольцом [предл.].
  4. **Звук:** у нас нарастающий гул в металле; у врага — сухой щелчок-отсчёт.
- **Против требы:** стержень не светится ровным серебром и не стоит дольше 1,5 с после удара.

## 4. Звук
Вбивание — глухой стук железа в сухую землю. Телеграф — нарастающий металлический гул, как ветер в натянутом тросе, 1,5 с вверх по тону. Удар — один сухой раскат «кррак» с коротким хвостом (≤0,5 с); **возле наставника — удар без раската, только треск** (решение «гром немой»). Оглушение — тонкий звон в ушах врага.

## 5. Промпты

**Концепт эффекта (Скелет, камера за плечом):**
```
Game VFX concept art, painterly photorealism, dark fantasy Slavic myth, over-the-shoulder third-person camera behind a clean pale human skeleton with faint lavender embers in its joints and skull, pointing down at a spot 8 meters ahead. There a forged black iron rod about 1.2 meters tall stands in cracked dry earth, slightly tilted, with a small verdigris copper six-petal thunder rosette finial. A single branching lightning bolt strikes from dark clouds straight into the rod's finial, cold lavender blue (#8FA8FF) with pale blue-white core (#DDE4FF). From the rod base a ring of jagged ground arcs spreads outward to a 3 meter radius, a notched zigzag circle burned into the dry grass. Three faded grey-white humanoid enemies inside the circle frozen mid-step, small lavender sparks crawling over their outlines. Dry thunderstorm dusk, no rain, warm low side light, background brighter than figures. 16:9, no text.
```

**Концепт телеграфа (кадр до удара):**
```
Same scene one second earlier: the iron rod stands in the ground, its copper finial glowing softly lavender, a dotted ring of carved zigzag notches glowing faint lavender blue on the dry grass expanding outward around the rod to 3 meters, four thin radial glowing cracks from the rod to the ring forming a diagonal cross in a circle, small sparks climbing the rod. Low intensity glow, calm, no lightning yet. 16:9, no text.
```

**Иконка-руна:**
```
Flat game UI rune icon, 256x256, centered on a dark slate (#1E2230) square tile. An outer circle frame with a thick carved lavender blue (#8FA8FF) stroke and thin pale blue (#DDE4FF) core line, a thin sharp zigzag running around its outside. Inside: a smaller circle containing a diagonal X cross whose arms stop short of the inner circle. Chiseled straight line ends, flat vector, no perspective, faint soft outer glow.
```

**Негатив (сцена):** `red, white, red and white, pure white flash, gold, neon, cyberpunk, sci-fi pylon, tesla coil, rain, wet, puddles, blood, text, logo, cartoon, anime, Christian cross, crosshair`
