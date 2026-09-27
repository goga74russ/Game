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

	float Incoming = Amount * IncomingMultiplier;
	if (Shield > 0.f)
	{
		const float Absorbed = FMath::Min(Shield, Incoming);
		Shield -= Absorbed;
		Incoming -= Absorbed;
		if (Shield <= 0.f && OnShieldBroken) { OnShieldBroken(); }
		if (Incoming <= 0.f) { return 0.f; }
	}
	const float Applied = FMath::Min(Incoming, Health);
	Health -= Applied;
	if (IsDead())
	{
		OnDeath.Broadcast(Instigator);
	}
	return Applied;
}
