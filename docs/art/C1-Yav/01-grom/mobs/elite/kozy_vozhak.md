# Козий вожак (элита 2)

> Часть библии персонажей главы Перуна (v0.1, черновик art-director). Общие правила, перепись, бюджеты и вопросы — `docs/art/C1-Yav/01-grom/README.md`. Пометки: **[предл.]** — предложение, **[пробел]** — нет в документах, **[Д]** — ориентир.

### A2. Козий вожак (элита 2, остаётся в демо (решение директора 2026-09-27); нужен козий риг)

- **Что это:** «старая, с обломанным рогом и **железом, вросшим в хребет**, стоит отдельно» (T §6). Таран с рывком по прямой (L §2.4). Даёт камень 3 и поддержку.
- **Силуэт:** коза, стоящая отдельно от стада. Обломанный рог даёт **асимметрию** головы — главный признак. Железо — гребень на хребте: **обрывок громовой жилы**, как на столбах Сухоречья; коза тёрлась о жилу, и та вросла [предл. летописца]. Размер — [пробел]; [предл.] в 1,5 раза крупнее обычной козы, чтобы читался как элита.
- **Материалы:** жёсткая седая шерсть; рог; железо с окалиной.
- **Палитра [предл.]:** шерсть #8E8678 → седина #B8B0A0, рог #5A4E40, железо #2E2C2C.
- **Узнаваемая деталь:** обломанный рог + железный гребень.
- **Телеграф:** **роет землю копытом, над ним звенит ведро на журавле** (L §2.4, T §6). Звон ведра — часть окружения, а не модели: журавль нужен рядом с ареной вожака.
- **Точка парирования** — [пробел] (GDD §5: у элит есть светящаяся точка). [предл.] Гнилушная точка в месте, где железо входит в хребет, загорается во время рытья.
- **Звук [предл.]:** хрип, удар копыта, скрежет железа на хребте при рывке.

### Детализация (2026-09-27) [предл.]
- **Размеры:** высота в холке 1,15 м (обычная коза ~0,75 м), длина корпуса 1,6 м; целый рог — саблевидный, 55 см, назад-вбок; обломок второго — 14 см, скол рваный, светлее #8A7A64. Борода 25 см.
- **Жила в хребте:** от холки до крупа, 90 см; 7–9 железных обручей #2E2C2C с окалиной #4A3E36, в зазорах тёмная жила #4A3430; шерсть вокруг вытерта, кожа — плотный серый рубец #6A625A (без ран и крови). Над жилой торчит оборванный конец жилы 20 см у холки — силуэт «гребня».
- **Шерсть:** длинная, свалявшаяся на боках колтунами, по брюху — выгоревшая до #B8B0A0, на морде седая.
- **Глаза:** козьи, с горизонтальным зрачком, тёмные, **не светятся** (светится только точка на хребте).
- **Износ:** порванное ухо, копыта стёрты и расслоены, на боку — бурое пятно глины.
- **Силуэт в бою (0,5 с):** опущенная голова с одним рогом вперёд + железный гребень — читается как «таран».

### Что роняет и как выглядит дроп
Выдача по `skills_demo_v0.1` §1 и решению 2026-09-27 п. 9: **поддержка** («Заземление» по skills_demo) и **камень 5 «Оберег грозы»**.
1. **Смерть:** вожак падает на бок, жила на хребте искрит голубым #8FA8FF 2 с и темнеет; журавль рядом замирает, ведро звякает последний раз.
2. **Камень «Оберег грозы»** — галька (`docs/art/common/items/skill_stone.md`) с **кругом** (чары) и узором **полукруг-чаша** (щит), зигзаг по краю. [предл.] Выпадает **из-под обломка рога**: лежит у головы вожака, рядом — отломанный кончик рога 14 см (декор, не подбирается). Столб #FFC24A, кольцо #8FA8FF (60%).
3. **Поддержка** — носитель-кольцо [предл.] (см. `zhilny_uzel.md`): **обруч, снятый с жилы на хребте**, Ø 12 см, внутри выбит узор поддержки. Для «Заземления» [предл.] — вертикальная черта, упирающаяся в три горизонтальные (знак «в землю»). Лежит на хребте павшей козы, столб #FFC24A, кольцо охры #9C7A3A.
4. **Кадр:** туша седой козы, два золотых столба — у головы и над хребтом, сзади — журавль колодца на фоне неба.

## Промпт для Meshy

Общие условия и негатив — в `README.md` (раздел «Промпты: общие условия»).

**A2 Козий вожак**
```
Old large mountain goat, 1.15 m at the withers, 1.6 m body length, long stiff matted grey coat faded pale on the belly, grey beard, one long curved horn, the other broken off to a ragged stump, torn ear, worn split hooves, a 90 cm ridge of scaled iron hoops over dark sinew grown into its spine from withers to rump like a fused cable, grey scar skin around it, a torn cable end sticking up at the withers, small pale cyan-green glow where the iron enters the back, eyes dark and not glowing. Standing side and front view, neutral grey background. PBR textures, no baked lighting. No red, no blood, not wet.
```

## Промпт — концепт (картинка)
```
Creature concept for a dark folk-fantasy game: an elite enemy, an old huge mountain goat 1.15 m at the withers standing apart from the herd on a dry highland pasture at twilight, matted grey weathered coat, one long curved horn and one broken stump, a ridge of dark scaled iron hoops over brown sinew fused into its spine like a grown-in cable with a torn end sticking up, a small pale cyan-green glowing point #7FE8C0 where the iron enters the back, head lowered, pawing the ground with a hoof, a wooden well sweep with a hanging bucket behind it. Dry, no blood. Muted desaturated palette, painterly photorealism, human figure 1.78 m for scale, 3/4 view.
```
**Дроп (картинка):**
```
A slain old grey goat lying on its side on dry grass, iron ridge on its spine gone dark; near its head a flat dark pebble with a carved circle frame and a bowl-shaped half-circle, grooves glowing lavender-blue, beside a broken horn tip; on its back a scaled iron ring with a punched grounding mark; each item marked by a vertical beam of warm wax-gold light #FFC24A; a well sweep against a pale stormy sky. Twilight, dry, painterly photorealism, dark folk-fantasy.
```
