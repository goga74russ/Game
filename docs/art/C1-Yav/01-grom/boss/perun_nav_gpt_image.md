# Настоящий Перун (Навь): промты GPT Image (v0.2, черновик)

> ⚠️ **Черновик, не канон.** Заменяет промты v0.1. Направление — вариант Б «Живой громоотвод» (creative-miyazaki, принят продюсером), директор ещё не утверждал.
> Облик `perun_nav_look_v0.1.md` устаревает по пунктам: рост, одежда, пояс, ртуть, кожа. Лор (F) не меняется.
> Каналы и палитра — `docs/art/style_v0.1.md` §2–4, `docs/art/biomes_v0.1.md` §1, `../README.md` §0.
> HEX ртути, кожи, корки и ткани — **[предл.]** арт-директора, ставятся окончательно после серого теста.

## Что изменилось против v0.1

Результат v0.1 читался как погорелец или нежить: человеческий рост, лохмотья, коричневая кожа, ртуть как серебряные болты. В v0.2:
- **Суть:** не человек, с которым что-то случилось, а **бог, превращённый в громоотвод**. «Это не враг, это инструмент.»
- **Рост ~4 м**, человеческие пропорции, сухой, длиннорукий, плечи примерно в 3 ширины головы.
- **Силуэт:** 3–4 асимметричных стеклянных шпиля из фульгуритовой корки на плечах и спине, один сломан. Одна рука всегда поднята к небу.
- **Ртуть** идёт из земли: от корней дуба по земле, вверх по ногам и спине в шпили. Тёмная, зеркальная, без свечения. Серебро запрещено.
- **Кожа** пепельно-серая, минеральная. **Ткань** остеклована: чёрная, жёсткая, без лохмотьев. Пояса, колец, спиралей на руках и обрубков у шеи нет.

## Палитра (для всех промтов)

| Слой | HEX |
|---|---|
| Вспышка лица и молнии в шпили | #8FA8FF, ядро #DDE4FF |
| Дальние молнии неба Нави | #9A8CFF |
| Кожа | пепельная #7A7672, тени #4F4C49; фигуры Лихтенберга #34343C [предл.] |
| Предплечья | чёрное стекло #15161C, блики #3A3F4A [предл.] |
| Фульгуритовая корка | спёкшийся песок #5E5850, стекло в изломах #1E2420 [предл.] |
| Шпили | дымчатое стекло #8C9A8E, тело #3E4640 [предл.] |
| Остеклованная ткань | #0E0F12, холодные блики #3A3F4A [предл.] |
| Ртуть | тёмное зеркало #23272B, отражения #5A6470, зеленца в глубине #4E5E56 [предл.] |
| Небо, тучи, дуб | #2E3440, #4A5160, #2A241E |
| Спёкшаяся трава | сухая #A8996A → стекло #8C9A8E |
| Точка парирования | #7FE8C0, в центре громового знака на груди; светится только во время замаха (решение директора) |

Запрет во всех промтах: золото, серебро, корона, ореол, красный, восковое золото, шлем, доспех, оружие, гниль и раны, вода и дождь, резьба по дереву на теле.

## Порядок

1. **Промт A** — концепт в бою, чтобы директор оценил образ. Формат 1024×1536 (вертикаль).
2. **Промт B** — A-поза спереди для image-to-3D. Генерировать **после** выбора A, приложив выбранный A как референс. 1024×1536.
3. **Промт C** — лист разворота, акцент на спину и шпили. 1536×1024.
4. **Промт D** — лист деталей для текстур. 1536×1024.
5. **Промт E** — после победы. 1536×1024.

Каждый промт самодостаточен, вставлять целиком, отдельным запросом.

**Почему в B рука опущена.** Поднятая рука — сигнатурная поза, но это анимация, а не форма модели. Для image-to-3D и рига нужна симметричная A-поза: иначе генератор срастит руку с шпилями, а плечо при скиннинге порвётся. Поднятую руку ставим в Unreal, idle-анимацией.

**Почему в B все шпили целые.** Сломанный шпиль делается в движке: обломок — отдельный меш, который прячем. Так же устроены фаза 2 (шпили отламываются) и состояние «после победы» (шпилей нет).

---

## Промт A. Концепт в боевой позе (для директора)

```
Concept art, single key illustration for a dark fantasy action RPG boss, painterly photorealism: real physical materials rendered with visible painterly brushwork in the background and razor-sharp detail in the foreground. Vertical composition. Camera elevated about 30 degrees above the ground and looking down at the figure in a three-quarter view, like a slightly lowered isometric game camera, so the shoulders, the back crust and the glass spires are clearly readable from above. Full figure visible from the tips of the spires to the feet, with ground around him.

THE IDEA: an ancient Slavic thunder god who has been turned into a living lightning rod. He is not a man to whom something terrible happened; he is an instrument. Lightning strikes the glass spires on his back, he does not flinch, and he calmly redirects the flash to the spot where his enemy stood a second ago. Mood: cold, inevitable, sacred and mechanical at the same time.

FIGURE AND SCALE: about 4 meters tall but with normal human proportions, not a hulking giant and not a brute: lean, dry, sinewy, long-limbed, fingertips reaching almost to the knees, narrow waist, long neck, a slightly small head. The shoulders are broad, roughly three head-widths across, and clearly wider than the head from above. For scale, an ordinary human warrior about 1.8 meters tall stands in the mid-ground a few steps away from him; the top of the human's head reaches only the god's waist.

POSE: one arm raised straight up toward the sky, hand open, fingers extended together as if pointing at the storm, not making a fist and not striking; this raised arm is his signature. The other arm hangs low and slightly forward, open palm turned toward the ground several meters in front of him, where a bolt of lightning is landing on an empty patch of ground; the small human figure is leaping away from that spot, just escaping. His body is perfectly still and upright, weight even, no strain: the strike costs him nothing. The head is turned toward the impact point.

FACE: he has no face. The skull is bald, smooth and mineral, ears and jawline faintly readable, but from the brow to the chin the entire front of the face is a frozen lightning flash: a ragged, torn-edged, asymmetric burst of cold lavender light (#8FA8FF) with a pale blue-white core (#DDE4FF), irregular, never round, never a disc. Thin branching cracks of light escape from its edges onto the temples and down the neck. No eyes, no nose, no mouth. The flash lights nearby surfaces with hard short shadows, but there is no halo, no crown of light, no rays, no glowing aura around the head.

SKIN: ash-grey and mineral, like weathered stone or fired clay (#7A7672, shadows #4F4C49), completely dry and matte, clean, intact, statuesque. Across the whole body run crisp, clean, finely branching Lichtenberg figures, fern-like lightning patterns printed into the skin in dark slate (#34343C) with the faintest lavender tint. In the center of the chest a six-spoked thunder rosette is burned deep into the skin, crisp-edged, slightly darker than the Lichtenberg lines; at the exact center of the rosette is a small round point, dark and unlit at this moment. No wounds, no rot, no gore, no charred meat: he is not dead, not undead, not a burn victim.

SHOULDERS, BACK AND SPIRES (the main silhouette element): the shoulders, the upper back and the base of the neck are covered by a thick crust of fused fulgurite, sand melted by lightning into rough glassy rock: knobbly sandy grey-umber outer surface (#5E5850) with dark glassy fractures (#1E2420). From this crust grow four asymmetric glass lightning-rod spires, like giant fulgurite tubes: hollow, knobbly, slightly twisted, smoky grey-green glass (#8C9A8E) with a darker core (#3E4640). The tallest rises from behind the left shoulder blade about 1.5 meters above his head; a second, shorter one leans outward from the right shoulder; a third, thin one rises from the middle of the spine; the fourth, on the right shoulder blade, is snapped off halfway, its jagged hollow end exposed. The spires lean in different directions, not symmetric, not a crown, not a halo. A lavender lightning bolt (#8FA8FF, core #DDE4FF) from the sky is hitting the tip of the tallest spire right now; light runs down inside the hollow glass.

FOREARMS AND HANDS: from the elbows to the fingertips each arm is one continuous mass of black fulgurite glass (#15161C, cold highlights #3A3F4A), thicker than the upper arm, like a melted glass pipe with frozen drips, ring-shaped ridges and trapped bubbles. The transition from ash skin to glass at the elbow is jagged and fused. The hands are always open, never fists: long glass fingers with angular knuckles. In the center of each palm is a smooth melted spot; on the lowered hand it is glowing lavender because it is redirecting the strike, on the raised hand it is dark.

CLOTHING: a knee-length shirt-robe of heavy linen that has been vitrified into black glass (#0E0F12): stiff, glossy, its folds frozen in place like obsidian, with clean sharp edges and an intact hem. It does not move in the wind. No tatters, no rags, no torn strips, no burned holes. Sleeveless, the fulgurite crust breaks through the cloth at the shoulders. The neckline is open in a deep V down to the sternum so the burned thunder rosette is visible. No belt, no rings, no straps, no jewelry, no armor.

MERCURY: liquid mercury comes out of the ground, not out of his body. From the roots of a giant blackened oak in the background, several thick streams of mercury run across the ground toward him, climb up his bare legs and calves, wind up his thighs under the glass robe, rise along his spine and pour upward into the bases of the spires, always flowing, always against gravity. The mercury is DARK: a black liquid mirror like a night lake (#23272B), reflecting the storm sky (#5A6470) and the lightning, with a faint greenish undertone in the deepest reflections (#4E5E56). It has no glow of its own. It is not bright silver and not chrome.

FEET AND GROUND: bare ash-grey feet planted firmly. The dry grass under and around his feet is baked into dull glass (#8C9A8E) in a ring, with fused glassy footprints behind him. The rest of the ground is dry pale ochre grass (#A8996A, desaturated) and cracked dark earth.

ENVIRONMENT: in the background a colossal blackened oak (#2A241E) used as a lightning rod, bent iron spire at its crown, the mercury streams running from its roots toward the god, the bark dry and dark. A dry thunderstorm: no rain, no water, no puddles, no wet surfaces anywhere except the mercury. Slate storm sky (#2E3440) with lead-grey clouds (#4A5160), distant violet lightning (#9A8CFF) in the upper left. Dust and static in the air. Eternal dusk: the background is lighter than the figure, so the god reads as the darkest large mass in the frame, with the face flash and the struck spire as the two brightest points.

STYLE: painterly photorealism, dark surreal mythic atmosphere, sacred biomechanics where stone, glass and living body are fused into one mechanism, Slavic myth expressed only through the thunder rosette, the oak and the restraint of the design. Overall saturation low, below 60 percent, except the lavender lightning. Strong, instantly readable silhouette: raised arm, broad shoulders, asymmetric spires.

DO NOT INCLUDE: gold, silver, bright chrome, crown, halo, rays around the head, red color, warm golden light, helmet, armor, weapon, hammer, axe, belt, rings, chains, ropes, cloak, cape, rags, tatters, torn cloth, beard, hair, visible eyes or mouth, wounds, blood, rot, zombie look, burned flesh, carved wood or carvings on the body, rain, water, puddles, neon, sci-fi tech, glowing circuits, floating sparks everywhere, text, logo, watermark, extra limbs.
```

## Промт B. Техническая A-поза спереди (для image-to-3D)

```
IMPORTANT: this is a technical character reference for 3D model generation, not an illustration. Plain flat light-grey background (#BFBFBF), soft even studio light from the front, no rim light, no dramatic lighting, no cast shadows, no floor, no ground, no environment, no fog, no particles, no lightning bolts, no depth of field.

VIEW AND POSE: single full-body figure, strict front orthographic view, centered, entire figure visible from the tips of the spires to the soles of the feet, with empty margin on all sides. Neutral symmetric A-pose: standing straight, feet shoulder-width apart and flat, both arms lowered and angled away from the body at about 40 degrees, hands open with fingers slightly spread, palms facing forward-down. Nothing overlaps the torso, legs or spires; there is a clear gap between the arms and the body.

CHARACTER: an ancient Slavic thunder god turned into a living lightning rod, about 4 meters tall with normal human proportions: lean, dry, sinewy, long-limbed, fingertips reaching almost to the knees, narrow waist, long neck, slightly small head, broad shoulders about three head-widths across. Not a giant brute, no bodybuilder muscles.

HEAD AND FACE: bald, smooth, mineral skull, faintly readable ears and jawline. No face: from brow to chin the front of the face is a flat, slightly recessed area filled with a frozen lightning flash, a ragged torn-edged asymmetric shape of lavender (#8FA8FF) with a pale core (#DDE4FF), never round. Thin branching light-cracks run from its edges onto the temples and neck. No eyes, nose or mouth. The flash is a flat emissive surface inside the face and does not glow outward: no halo, no aura, no rays.

SKIN: ash-grey mineral skin like weathered stone (#7A7672, shadows #4F4C49), dry and matte, clean and intact, no wounds. Crisp finely branching Lichtenberg figures in dark slate (#34343C) all over the body. A six-spoked thunder rosette burned into the center of the chest, with a small round dark point at its exact center, not glowing.

SHOULDERS, BACK AND SPIRES: shoulders, upper back and base of the neck covered by a thick crust of fused fulgurite, knobbly sandy grey-umber rock (#5E5850) with dark glassy fractures (#1E2420). From the crust rise four complete, unbroken asymmetric glass spires like giant fulgurite tubes, knobbly and slightly twisted, rendered as OPAQUE smoky grey-green glass (#8C9A8E, darker body #3E4640), not transparent. From the front: the tallest rises behind the left shoulder about 1.5 m above the head, a shorter one leans outward from the right shoulder, a thin one rises behind the head from the middle of the spine, a fourth medium one rises from the right shoulder blade. Their bases are fused solidly into the crust. They do not form a crown or a halo.

FOREARMS AND HANDS: from elbows to fingertips each arm is one continuous mass of black fulgurite glass (#15161C, highlights #3A3F4A), thicker than the upper arm, like a melted glass pipe with frozen drips, ring ridges and small bubbles; jagged fused transition at the elbow. Open hands with long glass fingers and angular knuckles; a smooth melted dark spot in the center of each palm, not glowing.

CLOTHING: a knee-length sleeveless shirt-robe of linen vitrified into black glass (#0E0F12), stiff and glossy, simple frozen folds that stay close to the body, clean intact hem, no tatters. Deep V neckline open to the sternum so the chest rosette is visible. The fulgurite crust breaks through the cloth at the shoulders. No belt, no rings, no straps, no jewelry.

MERCURY: dark liquid mercury like a black mirror (#23272B with cold reflections #5A6470), no glow, not silver, not chrome. Four smooth streams of it run up the bare legs from the feet, wind around the calves, disappear under the robe and reappear on the back running up into the bases of the spires. At the feet each stream ends neatly at the sole in a short, rounded, smooth stub, like a rivulet that was cut off cleanly; nothing extends beyond the outline of the feet.

FEET: bare ash-grey feet, both fully visible, flat, toes clearly separated. No glass grass, no ground under them.

Material rendering: photoreal, clear readable forms, sharp silhouette, high surface detail, muted colors; the face flash is the only bright element.

DO NOT INCLUDE: gold, silver, chrome, crown, halo, red, helmet, armor, weapon, belt, rings, chains, cloak, rags, tatters, hair, beard, eyes, wounds, rot, zombie look, carved wood on the body, water, lightning bolts, sparks, environment, ground, text, labels, watermark, extra limbs, cropped spires, cropped head or feet.
```

## Промт C. Лист разворота (акцент на спину и шпили)

```
Professional character turnaround sheet for a 3D modeler, one wide image. The same character shown four times side by side at identical scale, standing on one shared baseline, evenly spaced, not overlapping: 1) front view, 2) left side view in strict profile, 3) back view, 4) back three-quarter view from behind the right shoulder. All views orthographic, neutral A-pose (arms lowered and angled about 40 degrees away from the body, open hands, feet shoulder-width apart). Plain flat light-grey background (#BFBFBF), even studio lighting, no cast shadows, no floor, no environment, no lightning, no text, no labels, no numbers. Leave enough empty space above each figure so the tops of the spires are never cropped.

CHARACTER (keep perfectly identical in every view): an ancient Slavic thunder god turned into a living lightning rod, about 4 meters tall with normal human proportions, lean, dry, sinewy, long-limbed, fingertips almost at the knees, narrow waist, long neck, slightly small bald mineral head, broad shoulders about three head-widths across. No face: from brow to chin a flat recessed area holding a frozen ragged asymmetric lightning flash, lavender #8FA8FF with pale core #DDE4FF, never round, thin light-cracks spreading onto the temples and neck, no eyes, no mouth, no halo. Ash-grey matte mineral skin (#7A7672, shadows #4F4C49) covered in crisp dark slate Lichtenberg figures (#34343C); a six-spoked thunder rosette burned into the center of the chest, with a small round dark point at its exact center, not glowing. Forearms from elbow to fingertips are black fulgurite glass (#15161C), thicker than the upper arm, like a melted glass pipe with frozen drips and bubbles; open hands, a smooth melted spot in each palm. Knee-length sleeveless shirt-robe of linen vitrified into stiff glossy black glass (#0E0F12), frozen folds, clean hem, deep V neckline to the sternum, no belt, no tatters. Bare ash-grey feet.

THE BACK IS THE MOST IMPORTANT PART OF THIS SHEET; render it with the most detail:
- A thick crust of fused fulgurite covers both shoulders, the whole upper back down to the lower edge of the shoulder blades, and the back of the neck up to the base of the skull: knobbly, sandy grey-umber rock (#5E5850) with dark glassy fractures (#1E2420). The crust breaks through the black glass robe; the robe's back is open from the neck to the lower back, its edges fused into the crust.
- Four asymmetric glass spires grow out of the crust like giant fulgurite tubes: hollow, knobbly, slightly twisted, smoky grey-green glass (#8C9A8E, darker body #3E4640). Spire 1, the tallest, rises from the left shoulder blade, leaning slightly back and outward, its tip about 1.5 m above the head. Spire 2, shorter and thicker, rises from the top of the right shoulder and leans outward to the right. Spire 3, thin and straight, rises from the middle of the spine between the shoulder blades, its tip just above the head. Spire 4, medium, rises from the right shoulder blade and is snapped off halfway, ending in a jagged hollow broken tube. The four spires point in four different directions and never form a symmetrical crown or halo.
- Dark liquid mercury (black mirror #23272B, cold reflections #5A6470, no glow, not silver) runs in four smooth streams from the soles up the backs of the legs, under the robe, then up along the spine in a braided channel, and splits into the bases of the four spires, flowing upward against gravity. At the feet each stream ends cleanly in a short rounded stub.
- The side view shows how far the spires project backward, the upright posture without hunching, the thickness of the glass forearms compared to the upper arms, and the depth of the shoulder crust.

Photoreal materials, muted desaturated palette, only the face flash is bright. DO NOT INCLUDE: gold, silver, chrome, crown, halo, red, helmet, armor, weapon, belt, rings, chains, cloak, rags, hair, beard, eyes, wounds, rot, carved wood on the body, water, lightning bolts, sparks, environment, text, labels, watermark.
```

## Промт D. Лист деталей для текстур

```
Character material detail sheet for texture artists, one wide image divided into six close-up panels in a 3 by 2 grid, separated by thin even gaps, plain light-grey background (#BFBFBF), soft even studio lighting, no cast shadows, no environment, no text, no labels, no numbers. All panels belong to the same character: an ancient Slavic thunder god turned into a living lightning rod, with ash-grey mineral skin, black glass forearms, a fulgurite crust with glass spires on his back, a robe of vitrified black linen and dark liquid mercury flowing up his body.

Panel 1, the face-flash: close-up of the head in front three-quarter view. Bald smooth mineral skull, ash-grey (#7A7672), ears and jawline faintly readable. From brow to chin the face is replaced by a frozen lightning flash: a ragged, torn-edged, asymmetric shape of cold lavender light (#8FA8FF) with a pale blue-white core (#DDE4FF), irregular and never round, set into a shallow recess. Fine branching light-cracks run out of its edges onto the temples and down the neck and fade into dark slate Lichtenberg lines (#34343C). No eyes, nose or mouth, no halo, no glow outside the face surface.

Panel 2, glass forearm and open palm: right forearm and hand, palm facing the viewer. Black fulgurite glass (#15161C, cold highlights #3A3F4A), thicker than a normal forearm, like a melted glass pipe: frozen drips, ring-shaped ridges, small trapped bubbles, fine internal cracks. Long glass fingers with angular knuckles, open hand. In the center of the palm a smooth melted round spot glowing lavender (#8FA8FF) from inside. At the top edge of the panel the jagged fused transition from ash-grey skin with Lichtenberg lines into the glass.

Panel 3, fulgurite crust and spire base: close-up of the top of the shoulder. Thick crust of fused fulgurite: knobbly, sandy grey-umber (#5E5850) with visible sand grains fused together, dark glassy fractures (#1E2420). From it rises the base of a hollow glass spire like a giant fulgurite tube, smoky grey-green glass (#8C9A8E, darker body #3E4640), knobbly outer surface and smooth glassy inner wall; next to it the jagged hollow end of a snapped-off spire. A thin stream of dark mercury enters the spire base from below.

Panel 4, vitrified cloth: a section of the robe over the chest with the open V neckline. Linen turned into black glass (#0E0F12): stiff glossy frozen folds like obsidian, the linen weave still faintly visible under the glass surface, clean sharp hem edge, no tatters, no holes. Through the neckline, ash-grey mineral skin with crisp Lichtenberg figures and a six-spoked thunder rosette burned deep into the sternum. At the exact center of the rosette a small round point glows with a soft cold turquoise-green light (#7FE8C0), like a single ember of foxfire, sharp-edged and point-like, its light staying inside the rosette and not spreading as a halo. The six spokes themselves stay dark.

Panel 5, foot on baked grass: bare ash-grey mineral foot standing on dry pale ochre grass (#A8996A); in a ring around the foot the grass has fused into dull glass (#8C9A8E), blades frozen and shiny, with a glassy footprint behind it. A smooth stream of dark mercury climbs from the ground over the ankle. Completely dry: no water, no dew.

Panel 6, mercury mirror: a flat material swatch of flowing liquid mercury running upward over the ash-grey skin of a calf. The mercury is a dark liquid mirror (#23272B), reflecting a stormy sky (#5A6470) and a thin lavender lightning reflection (#8FA8FF), faint greenish undertone in the deepest areas (#4E5E56). Thick, heavy, smooth, rounded edges, no glow of its own. It is not bright silver and not chrome.

Photoreal material detail, muted colors. DO NOT INCLUDE: gold, silver, chrome, red, rags, wounds, blood, rot, carved wood, water drops, text, labels, watermark.
```

## Промт E. После победы (без шпилей, сидит на корне)

```
Concept art, quiet aftermath scene for a dark fantasy action RPG, painterly photorealism with soft painterly background and detailed foreground. Horizontal composition. Camera slightly elevated, looking down at about 25 degrees, three-quarter view, medium-wide shot.

SUBJECT: an ancient Slavic thunder god who was used as a living lightning rod and has just been released. He is about 4 meters tall with lean, dry, long-limbed human proportions, but now he sits hunched on a huge twisting root of a colossal blackened oak (#2A241E), and next to the root he looks small, almost like an ordinary tired man. His elbows rest on his knees, his black glass forearms (#15161C, like melted glass pipes with frozen drips and bubbles) hang between his knees, open glass hands loose and empty, palms dark. Head lowered.

FACE: he has no face. From brow to chin there is only a small, quiet, soft lavender glow (#8FA8FF with a faint pale core #DDE4FF), steady and calm like a small flame in a lantern, ragged at the edges but no longer a violent flash. It barely lights his chest. No eyes, no mouth, no halo, no rays.

BODY: bald ash-grey mineral skin (#7A7672, shadows #4F4C49), dry and matte, covered in crisp dark slate Lichtenberg figures (#34343C), a six-spoked thunder rosette burned into the center of the chest. His shoulders and upper back are still covered with the rough crust of fused fulgurite (#5E5850, dark glassy fractures #1E2420), but all the glass spires are gone: only four rough round sockets and short stumps remain where they grew, their edges clean, not bleeding. The broken glass spires lie shattered in the grass around the oak, smoky grey-green glass (#8C9A8E), dull and dead. He wears a knee-length sleeveless robe of linen vitrified into stiff glossy black glass (#0E0F12), frozen folds, clean hem, no tatters, no belt. Bare ash-grey feet rest on grass that has fused into dull glass around them.

MERCURY: the mercury no longer climbs his body. The dark mirror-like streams (#23272B) that once ran from the oak roots are draining away into cracks in the ground, thin and still, losing their shine. No mercury on his legs or back.

ENVIRONMENT AND LIGHT: the storm is passing. The slate sky (#2E3440) and lead-grey clouds (#4A5160) are breaking up, only a faint distant violet flicker (#9A8CFF) at the horizon. Cool, dim, calm dusk light; dust slowly settling. Completely dry: no rain, no water, no puddles, no wet surfaces. The oak root and trunk dominate the frame and make him look small. Saturation low, below 60 percent. Mood: emptiness, release, exhaustion, a god who was a tool and is now nothing in particular.

DO NOT INCLUDE: glass spires attached to his body, lightning bolts, bright flash, gold, silver, chrome, crown, halo, red color, warm golden light, helmet, armor, weapon, belt, rings, chains, rags, tatters, hair, beard, eyes, wounds, blood, rot, zombie look, carved wood on the body, rain, water, tears, puddles, other characters, text, logo, watermark.
```

---

## Советы по генерации

- **Порядок важен.** Сначала 4–6 вариантов A, директор выбирает один. B, C и D генерировать уже с выбранным A во вложении и строкой в начале: `Use the attached image as the exact character reference: keep the same design, proportions, spires and materials.`
- **Если опять «погорелец» или нежить**, добавить в начало: `He looks like a statue of mineral stone and glass, clean and intact, not a burned man, not a corpse.`
- **Если ртуть стала серебром или хромом**: `The mercury is almost black, like a dark lake reflecting the storm; it is the darkest liquid in the image, never bright silver.`
- **Если шпили складываются в корону или ореол над головой**: `The spires grow from the shoulder blades and the back, not from the head; they lean in different directions and leave the head silhouette clear.`
- **Если в B руки срастаются со шпилями** — увеличить угол A-позы до 45–50° и повторить.
- **Для 3D-генератора** шпили лучше непрозрачные. Прозрачное стекло Meshy и Tripo превращают в дыры или кашу. Прозрачность и свечение делаем в Unreal: отдельный материал по маске лица, ладоней и трещин. Сломанный шпиль, отламывание в фазе 2 и пустые гнёзда в E — отдельные меши.
- **Точка парирования в A (по желанию).** Чтобы показать директору замах с горящей точкой, в промте A заменить блок POSE на: `POSE: wind-up before the strike: one arm raised straight up toward the sky with an open hand, the other arm drawn back, open palm glowing lavender, the whole body perfectly still. At the exact center of the burned thunder rosette on his chest a small round point glows with soft cold turquoise-green light (#7FE8C0), point-like, no halo.` — и убрать из SKIN фразу про `dark and unlit at this moment`. Риск: на вертикали с изометрической камерой грудь видна плохо, точку прикроет поднятая рука; проверить в сером прототипе.
- **Изометрия.** Если хочется проверить вид сверху, к C можно дописать пятый вид: `5) top-down view from directly above`. Генератор часто делает его неточно, поэтому лучше отдельным запросом.

## Решить директору

1. Утвердить направление «Живой громоотвод» и рост ~4 м (в v0.1 было 3,2–3,4 м).
2. ~~Точка парирования~~ — **решено директором:** #7FE8C0 в центре громового знака на груди, светится только во время замаха. В A не горит (поза — сам удар, не замах; вариант с замахом — в советах), в B/C только знак, в D горит (панель 4, для маски emissive).
3. Сцена у Стола в Прави (v0.1 §10): нужна ли версия без корки и шпилей?
