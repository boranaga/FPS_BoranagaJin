#include "Objects/FireAreaComponent.h"
#include "Interface/DamageInterface.h"
#include "Objects/DestructibleObject.h"

#include "TimerManager.h"

UFireAreaComponent::UFireAreaComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InitSphereRadius(FireAreaRadius);

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCollisionObjectType(ECC_WorldDynamic);

	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	SetGenerateOverlapEvents(true);
}

void UFireAreaComponent::BeginPlay()
{
	Super::BeginPlay();

	SetSphereRadius(FireAreaRadius);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UFireAreaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateFireArea();

	Super::EndPlay(EndPlayReason);
}

void UFireAreaComponent::ActivateFireArea(AActor* InFireCauser)
{
	FireCauser = InFireCauser;

	SetSphereRadius(FireAreaRadius);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetGenerateOverlapEvents(true);

	GetWorld()->GetTimerManager().SetTimer(
		FireDamageTimerHandle,
		this,
		&UFireAreaComponent::ApplyFireAreaDamage,
		DamageInterval,
		true
	);
}

void UFireAreaComponent::DeactivateFireArea()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireDamageTimerHandle);
	}

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FireCauser = nullptr;
}

void UFireAreaComponent::ApplyFireAreaDamage()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);

	AActor* OwnerActor = GetOwner();

	for (AActor* TargetActor : OverlappingActors)
	{
		if (!TargetActor) continue;

		if (!bDamageOwner && TargetActor == OwnerActor)
		{
			continue;
		}

		if (TargetActor->Implements<UDamageableInterface>())
		{
			IDamageableInterface* Damageable = Cast<IDamageableInterface>(TargetActor);

			if (Damageable)
			{
				const FVector HitDirection = (TargetActor->GetActorLocation() - GetComponentLocation()).GetSafeNormal();

				FDamageParams Damage;
				Damage.DamageAmount = FireAreaDamage;
				Damage.ImpactPoint = TargetActor->GetActorLocation();
				Damage.HitDirection = HitDirection;
				Damage.DamageCauser = FireCauser ? FireCauser : OwnerActor;

				Damageable->ReceiveDamage(Damage);
			}
		}

		if (bIgniteOtherDestructibleObjects)
		{
			//TODO: 불이 번지는 속도 조절해야함

			if (ADestructibleObject* DestructibleObject = Cast<ADestructibleObject>(TargetActor))
			{
				DestructibleObject->Ignite(FireCauser ? FireCauser : OwnerActor);
			}
		}
	}
}