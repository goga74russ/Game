#include "FNHealthComponent.h"

UFNHealthComponent::UFNHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFNHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
}

float UFNHealthComponent::ApplyDamage(float Amount, AActor* Instigator)
{
	if (bInvulnerable && !IsDead() && Amount > 0.f && OnAvoided)
	{
		OnAvoided();
	}
	if (IsDead() || bInvulnerable || Amount <= 0.f)
	{
		return 0.f;
	}

	const float Applied = FMath::Min(Amount * IncomingMultiplier, Health);
	Health -= Applied;
	if (IsDead())
	{
		OnDeath.Broadcast(Instigator);
	}
	return Applied;
}
