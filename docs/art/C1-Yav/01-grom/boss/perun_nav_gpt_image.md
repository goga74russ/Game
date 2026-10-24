# Настоящий Перун (Навь): промты GPT Image под 3D (v0.1, черновик)

> Основа: `perun_nav_look_v0.1.md`. Цель — референсы для image-to-3D (Tripo, Meshy и т. п.), а не красивый концепт.
> Открытые вопросы директору (рост, атрибут, точка парирования) приняты по умолчанию: 3,3 м, без атрибута, точка не рисуется.

## Порядок
1. **Промт 1** — один вид спереди в A-позе. Это главный вход для генератора 3D.
2. **Промт 2** — лист разворота (спереди, сбоку, сзади, три четверти) для мультивидовой генерации и для моделлера.
3. **Промт 3** — крупные детали (лицо, руки, торс, пояс, ступни) для текстур.
4. **Промт 4** — жилы и обрубки отдельно: в 3D-модель жилы до дуба не входят, дуб — отдельный ассет.

Каждый промт вставлять целиком, отдельным запросом.

## Промт 1. Вид спереди, A-поза (для image-to-3D)

```
Character reference image for 3D model generation. Single full-body figure, strict front orthographic view, centered, entire figure visible from top of head to soles of feet with empty margin around it. Neutral A-pose: standing straight, feet shoulder-width apart and flat on the ground, arms angled down and away from the body at about 40 degrees, fingers slightly spread and relaxed, nothing overlapping the torso or legs. Plain flat light-grey background (#BFBFBF), no floor, no environment, no cast shadow, soft even studio lighting from the front, no rim light, no fog, no particles, no motion blur, no depth of field.

Subject: an ancient Slavic thunder god in a broken, forgotten state, a tall gaunt man about 3.3 meters tall. Lean, sinewy, dry body: long arms, wide bony shoulders, narrow waist, visible tendons and ropy veins, no bodybuilder muscles. Head slightly small for the body. Skin grey-tan, dry, matte, finely cracked like drought-baked earth, not wet, not shiny, no wounds, no gore, not undead.

Face: there is no face. The skull shape, ears, jawline and neck are normal and solid, but from the brows down to the chin the front of the face is a flat, slightly recessed oval area filled with a frozen lightning flash: a ragged bright patch of lavender blue (#8FA8FF) with a pale blue-white core (#DDE4FF), its edges breaking into thin branching Lichtenberg lightning patterns that spread onto the cheeks, temples and neck. No eyes, no nose, no mouth, no hair on the face. Short dark grey hair on the scalp, slightly singed and standing up as if charged with static.

Body markings: dark branching Lichtenberg lightning-burn scars running from the neck down the chest and along the upper arms, like fern-shaped burns printed into the skin, matte dark brown-black. A single six-spoked thunder rosette burned into the center of the chest, faint, the same dark burn color.

Hands and forearms: from fingertips to elbows the skin has turned into black glassy fulgurite: glossy black volcanic glass texture (#15161C) with small bubbles, ridges and fine cracks, with a faint lavender (#8FA8FF) glow deep inside the cracks. Long fingers with slightly angular knuckles. In the center of each palm a smooth melted round spot. The transition at the elbow from skin to glass is jagged and irregular.

Clothing: a long, heavy linen shirt reaching to mid-shin, originally unbleached, now almost black with soot and char (#2E2620 to #4A4038), thick coarse weave visible. The hem is burned into ragged hanging wedges with charred edges. Sleeves burned away to just above the elbows, charred edges. A wide plain leather belt at the waist, dark brown, with four dull iron rings attached along it. No trousers visible below the shirt except bare shins. Feet bare, burned dark, toes slightly charred, no glass on the feet.

Back attachments: thick tubes of liquid mercury (silver #B8C2C6 with a greenish sheen #8FA89A, smooth reflective liquid metal) emerge from his spine, shoulder blades and the back of his skull; in this front view only short stubs are visible behind the shoulders and neck, cleanly cut off about 30 cm from the body, ending in rounded drops. Two thinner mercury tendrils wrap around each forearm in a loose spiral and sink into the black glass.

Colors overall muted and desaturated, the face flash is the only bright and emissive element. Painterly photoreal material rendering, clear readable shapes, sharp silhouette, high detail on surfaces.

Strictly no: weapon, axe, hammer, helmet, crown, cloak, cape, armor, jewelry, gold, silver ornaments, beard, moustache, carved wood on the body, red color, halo, aura, sparks floating around, lightning bolts in the air, environment, oak tree, ground, text, labels, watermark, extra limbs, cropped feet or head.
```

## Промт 2. Лист разворота

```
Professional character turnaround sheet for a 3D modeler, one wide image. The same character shown four times side by side at identical scale and height, aligned on one baseline, evenly spaced, not overlapping: 1) front view, 2) left side view (profile), 3) back view, 4) front three-quarter view. All views orthographic, neutral A-pose (arms angled 40 degrees down from the body, feet shoulder-width apart), plain flat light-grey background (#BFBFBF), even studio lighting, no cast shadows, no environment, no text or labels.

Character (keep perfectly consistent in every view): an ancient Slavic thunder god in a forgotten, broken state, a tall gaunt man about 3.3 m tall, lean and sinewy, long arms, wide bony shoulders, narrow waist, head slightly small for the body, dry matte grey-tan skin with fine drought cracks. No face: from brows to chin a flat slightly recessed oval filled with a frozen lavender lightning flash (#8FA8FF) with a pale core (#DDE4FF), edges breaking into branching Lichtenberg patterns over cheeks and neck; skull, ears and jaw intact; short singed dark grey hair standing up with static. Dark branching lightning-burn scars on neck, chest and upper arms; a faint six-spoked thunder rosette burned into the center of the chest. Forearms and hands from fingertips to elbows are black glassy fulgurite (#15161C) with bubbles and cracks glowing faint lavender inside, jagged transition at the elbows, melted round spot in each palm. Long heavy soot-black linen shirt to mid-shin, hem burned into ragged wedges, sleeves burned off above the elbows; wide dark leather belt with four dull iron rings; bare burned feet.

Back view must show clearly: six thick tubes of liquid mercury (silver #B8C2C6 with greenish sheen #8FA89A) emerging from the spine in a vertical row between the shoulder blades and one from the back of the skull, all cut off cleanly about 30 cm from the body and ending in rounded drops; the shirt is torn open on the back around the mercury entry points, showing cracked skin. Side view must show the slight forward hunch of the upper back and the mercury stubs sticking out backwards. Thin mercury tendrils spiral around both forearms in every view.

Muted desaturated palette, only the face flash bright. Strictly no weapon, helmet, cloak, armor, jewelry, gold, beard, moustache, carved wood, red, halo, floating sparks, environment, text, labels.
```

## Промт 3. Лист деталей (для текстур)

```
Character detail sheet for texture artists, six close-up panels on a plain light-grey background (#BFBFBF), even studio lighting, no text, no labels, panels separated by thin gaps.

Panel 1: head, front view, close-up. Normal skull, ears and jaw, short singed dark grey hair standing up with static. Instead of facial features, from brows to chin a flat slightly recessed oval of frozen lavender lightning (#8FA8FF) with a pale blue-white core (#DDE4FF), ragged edges dissolving into fine branching Lichtenberg lightning lines across the cheeks, temples and neck. No eyes, nose or mouth.
Panel 2: head, side view, showing how the flash surface sits inside the face like a shallow mask-shaped recess and how the lightning lines run down the neck.
Panel 3: right hand and forearm, palm up: black glassy fulgurite (#15161C), glossy volcanic glass with bubbles, ridges and fine cracks, faint lavender light deep inside the cracks, a smooth melted round spot in the center of the palm, long fingers with angular knuckles, a jagged irregular transition into cracked grey-tan skin at the elbow, a thin spiral tendril of liquid mercury (#B8C2C6, greenish sheen #8FA89A) wrapping the forearm and sinking into the glass.
Panel 4: chest close-up: dry matte grey-tan skin with fine drought cracks, dark fern-like Lichtenberg burn scars, a faint six-spoked thunder rosette burned into the center of the chest, charred edge of the linen shirt neckline.
Panel 5: waist and hem: coarse heavy linen almost black with soot (#2E2620 to #4A4038), visible weave, wide dark brown leather belt with four dull iron rings, shirt hem burned into ragged hanging wedges with charred edges.
Panel 6: back between the shoulder blades: six thick tubes of reflective liquid mercury emerging from the spine through torn shirt and cracked skin, the skin around each entry point slightly raised and darkened, tubes cut off and ending in rounded drops.

Photoreal material detail, muted colors, no environment, no weapon, no gold, no red, no blood or gore.
```

## Промт 4. Жилы и дуб (отдельный ассет, по желанию)

```
Environment prop reference, front view, plain light-grey background (#BFBFBF), even lighting, no character. The base of a giant dark oak tree used as a lightning rod: massive twisted roots, dark bark (#5A4A36 to #2E2620) with deep cracks, two heavy iron hoops around the trunk, thin streams of liquid mercury (#B8C2C6, greenish sheen #8FA89A) flowing UP along the cracks of the bark against gravity. From the roots, seven thick tubes of liquid mercury stretch outward along the ground toward the viewer and end cut off in rounded drops at the image edge. Grass around the roots baked into dull glass. Dry, no water, no rain, muted colors, no text.
```

## Советы по генерации
- Если GPT Image рисует драматичный фон или свет, добавьте в начало: «IMPORTANT: plain grey background and flat studio light, this is a technical reference».
- Если вспышку на лице он делает объёмным светом вокруг головы, допишите: «the flash is a flat emissive surface inside the face, it does not glow outward».
- Для 3D-генератора выбирайте вариант промта 1, где руки не касаются тела и видны обе ступни. От этого зависит, разделит ли генератор конечности.
- Свечение (вспышка, трещины в стекле) генераторы 3D запекают в цвет текстуры. Настоящее свечение делается потом в Unreal: отдельный emissive-материал по маске лица и трещин.
