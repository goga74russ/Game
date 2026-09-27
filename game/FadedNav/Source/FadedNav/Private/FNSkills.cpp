#include "FNSkills.h"

namespace
{
	using T = EFNSkillTag;
	const FFNSkillDef Defs[] = {
		{ TEXT("Громовой удар"), TEXT("Удар"), T::Strike, 20.f, 4.f, TEXT("Удар вперёд и разряд по дуге 120° на 4 м: 180% урона ближнего оружия. У Искры — кругом на 3 м."), TEXT("площадь") },
		{ TEXT("Громоотвод"), TEXT("Отвод"), T::Spell, 30.f, 10.f, TEXT("Стержень в точку прицела (до 8 м). Через 1,5 с молния R 3 м: урон и оглушение 0,8 с."), TEXT("обездвиживание") },
		{ TEXT("Сполох"), TEXT("Сполох"), T::Spell, 25.f, 7.f, TEXT("Рывок на 6 м сквозь врагов, неуязвимость 0,25 с, разряд по пути 60%."), TEXT("рывок") },
		{ TEXT("Цепная искра"), TEXT("Цепь"), T::Shot, 20.f, 3.f, TEXT("Выстрел перескакивает на 3 цели (−25% за прыжок). Тратит 1 патрон; у Искры +5 Яри вместо патрона."), TEXT("цепь") },
		{ TEXT("Оберег грозы"), TEXT("Оберег"), T::Spell, 35.f, 18.f, TEXT("Щит 25% здоровья на 5 с; когда пробит — разряд кругом R 4 м."), TEXT("щит") },
	};
}

const FFNSkillDef& FNSkills::Def(EFNSkillId Id)
{
	return Defs[FMath::Clamp(static_cast<int32>(Id), 0, static_cast<int32>(EFNSkillId::Count) - 1)];
}
