# Жильный узел (элита 1)

> Часть библии персонажей главы Перуна (v0.1, черновик art-director). Общие правила, перепись, бюджеты и вопросы — `docs/art/C1-Yav/01-grom/README.md`. Пометки: **[предл.]** — предложение, **[пробел]** — нет в документах, **[Д]** — ориентир.

### A1. Жильный узел (элита 1)

- **Что это:** «В нижнем рву, куда стекаются жилы со всего склона, они сплелись в клубок **размером с избу**. Клубок дышит» (T §5). В L — «большой Отрост». Учит парированию выстрелом по светящейся точке; даёт камень 2 (L §2.3).
- **Силуэт:** низкий широкий ком из толстых витков, от него во все стороны уходят жилы в землю, как корни [предл.]. **Спереди:** витки расходятся, в центре щель с точкой. **Сбоку:** горб выше человека, 3–4 м [предл.; «с избу» ≈ 5–6 м в поперечнике].
- **Материалы:** жила — «железные снаружи и жильные внутри, будто кто-то вытащил из земли сухожилия и оковал их» (T §1). Железо тёмное, в окалине; между обручами видна тёмная волокнистая жила.
- **Палитра [предл.]:** железо #2E2C2C, окалина #4A3E36, жила #4A3430 (тёмно-бурая, **не красная**), искры голубые (стихийный акцент ≤15%).
- **Узнаваемая деталь:** **точка в центре** между витками — «то вспыхивает, то гаснет, тем ярче, чем сильнее он собирается ударить» (T §5). Гнилушный #7FE8C0. Это главный телеграф и точка парирования.
- **Телеграф:** точка разгорается + клубок «вдыхает», поднимая тяжёлый виток; затем удар витком. Попадание в точку — узел «вздрагивает всем телом», удар уходит мимо (T §5).
- **Прочее:** из узла сыплются Отросты (T §5). Смерть: распадается на мёртвые витки, в пепле остаётся уголёк-камень (T §5).
- **Звук [предл.]:** скрежет железа, тяжёлое сиплое «дыхание», треск искр.

### Детализация (2026-09-27) [предл.]
- **Размеры:** поперечник 5,5 м, высота горба 3,5 м; витки Ø 45–70 см; 6–8 корней-жил уходят в землю на 4–6 м от тела. Щель в центре — 80×40 см, точка в ней Ø 12 см (ядро), свечение Ø 30 см.
- **Слои (снаружи внутрь):** окалина рыжей коркой #4A3E36 на верхних витках → железные обручи #2E2C2C шириной 12 см с шагом 25 см, внахлёст, как чешуя → в зазорах — тёмная волокнистая жила #4A3430, волокна вдоль витка → в щели — влажно-матовая тёмная глубина #1E1B1A и точка #7FE8C0.
- **Дыхание:** 0,2 Гц в покое, витки расходятся на 5 см; перед ударом — вдох 1 с, щель раскрывается до 120 см, точка разгорается.
- **Износ:** обручи лопнули местами, волокна торчат пучками (сухие, без крови); на земле вокруг — рыжая осыпь окалины и стеклянный песок.
- **Искры:** голубые #8FA8FF бегут по обручам от корней к центру, ≤15% площади.

### Что роняет и как выглядит дроп
Выдача по `docs/systems/skills_demo_v0.1.md` §1 и решению 2026-09-27 п. 9: **камень 4 «Цепная искра»** и поддержка **«Раскат»**.
1. **Смерть:** витки разжимаются и опадают кольцами (2–3 с), обручи лопаются с сухим звоном, из щели выходит последний разряд вверх; остаётся ком мёртвых витков и пепел (T §5).
2. **Камень «Цепная искра»** — «уголёк-камень в пепле» (T §5): носитель — тёмная галька (`docs/art/common/items/skill_stone.md`) с **ромбом** (выстрел) и узором **три точки на линии** (цепь), зигзаг по краю. [предл.] Лежит в центре пепла на месте щели и первые 2 с тлеет, как уголь: камень тёмно-вишнёвый по краям #4A2A20, затем остывает до обычного #3E3C3A — «уголёк» из лора без нарушения каналов. Затем обычная подача лута: столб #FFC24A 2,2 м, кольцо #8FA8FF (60%).
3. **Поддержка «Раскат»** — облик носителя поддержек в документах — [пробел]. [предл.] Поддержка = **кольцо-обруч**, снятый с узла: железное кольцо Ø 12 см, шириной 2 см, с окалиной, на внутренней стороне — выбитый узор поддержки (для Раската — два концентрических круга, как иконка «ring» древа). Камень — галька, поддержка — кольцо: разные силуэты, одна логика («вставляется к камню»). Лежит рядом с камнем в 1 м, столб #FFC24A, кольцо у основания охра #9C7A3A.
4. **Кадр для камеры:** два золотых столба в пепельном кратере среди мёртвых витков, дым сизый #8A847C.

## Промпт для Meshy

Общие условия и негатив — в `README.md` (раздел «Промпты: общие условия»).

**A1 Жильный узел**
```
Huge biomechanical knot of thick cables, 5.5 m across and 3.5 m tall, dark scaled iron hoops 12 cm wide overlapping like scales over dark brown fibrous sinew, coils 45-70 cm thick tangled into a low breathing mound, six to eight cable roots running into the ground, rusty scale crust on the upper coils, some hoops burst with dry fibres sticking out. A narrow slit in the centre between the coils with one small pale cyan-green glowing point. Faint blue sparks along the hoops. Neutral grey background, front view. PBR textures, no baked lighting. No red, no gold, no blood, not wet.
```

## Промпт — концепт (картинка)
```
Creature concept for a dark folk-fantasy game: an elite enemy, a house-sized breathing knot of biomechanical cables in a dry ditch at twilight, 5.5 m across, 3.5 m tall, thick coils of dark scaled iron hoops over dark brown fibrous sinew like tendons pulled from the earth and bound in iron, cable roots spreading into the cracked ground, rusty scale dust and grey-green glassy sand around it, one small pale cyan-green glowing point #7FE8C0 in a slit at its centre, faint lavender-blue sparks running along the hoops. Dry, no water, no blood. Muted desaturated palette, painterly photorealism, human figure 1.78 m for scale, 3/4 view.
```
**Дроп (картинка):**
```
Aftermath of a slain giant cable knot: a crater of grey ash and collapsed dead iron coils, in the centre a flat dark river pebble with a carved diamond frame and three dots on a line, the grooves glowing lavender-blue, edges of the stone still ember-dark cherry; one metre away a scaled iron ring 12 cm wide with a punched double-circle mark; each item marked by a vertical beam of warm wax-gold light #FFC24A. Slate-grey smoke, twilight, dry. Painterly photorealism, dark folk-fantasy.
```
