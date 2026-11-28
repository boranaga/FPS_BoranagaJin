#include "Characters/BloodTrailComponent.h"
#include "Characters/BloodStainActor.h"
#include "Characters/HealthComponent.h"
#include "Characters/CharacterComponentSettingsSource.h"
#include "Characters/CharacterComponentSettings.h"
#include "ObjectPoolSubsystem.h"
#include "Engine/World.h"

UBloodTrailComponent::UBloodTrailComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = BloodTrailTickInterval;
}

void UBloodTrailComponent::BeginPlay()
{
	Super::BeginPlay();

	ReloadSettingsFromOwnerDataTable();
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) { return; }
	HealthComponent = OwnerActor->FindComponentByClass<UHealthComponent>();
	LastBloodDropLocation = OwnerActor->GetActorLocation();
	EnsureBloodStainPool();
}

void UBloodTrailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateBleedingState();
	if (!bIsBleeding || !CanDropBlood()) { return; }
	SpawnBloodStain();
}

//void UBloodTrailComponent::NotifyDamageReceived(float AppliedDamage)
//{
//	if (!GetWorld() || AppliedDamage <= 0.f) { return; }
//
//	const bool bHeavyDamage = AppliedDamage >= HeavyDamageThreshold;
//	const bool bLowHealth = HealthComponent && HealthComponent->GetHealthPercent() <= BleedingHealthThresholdPercent;
//
//	if (!bHeavyDamage && !bLowHealth)
//	{
//		return;
//	}
//
//	bIsBleeding = true;
//
//	if (bHeavyDamage)
//	{
//		BleedingEndTime = GetWorld()->GetTimeSeconds() + BleedingDurationAfterHeavyDamage;
//	}
//}

void UBloodTrailComponent::StartBleeding(bool bLoop, float duration)
{
	UE_LOG(LogTemp, Error, TEXT("void UBloodTrailComponent::StartBleeding(bool bLoop, float duration)"));

	if (bLoop) { bIsBleeding = true; }
	else
	{
		bIsBleeding = true;
		BleedingEndTime = GetWorld()->GetTimeSeconds() + duration;
	}
}

void UBloodTrailComponent::UpdateBleedingState()
{
	if (bIsBleeding)
	{
		const bool bHeavyDamageBleedingActive = GetWorld()->GetTimeSeconds() < BleedingEndTime;
		if (!bHeavyDamageBleedingActive && GetWorld() && BleedingEndTime > 0)
		{
			StopBleeding();
		}
	}
}

bool UBloodTrailComponent::CanDropBlood() const
{
	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor || !GetWorld() || !BloodStainClass)
	{
		return false;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	if (CurrentTime - LastBloodDropTime < MinimumBloodDropInterval)
	{
		return false;
	}

	const float MovedDistance = FVector::Dist2D(OwnerActor->GetActorLocation(), LastBloodDropLocation);

	return MovedDistance >= MinimumBloodDropDistance;
}

bool UBloodTrailComponent::FindGroundBelow(FHitResult& OutHitResult) const
{
	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor || !GetWorld())
	{
		return false;
	}

	const FVector TraceStart = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, GroundTraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BloodGroundTrace), false, OwnerActor);

	return GetWorld()->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
}

void UBloodTrailComponent::EnsureBloodStainPool() const
{
	UWorld* World = GetWorld();
	if (!World || !BloodStainClass || BloodStainPoolPrewarmCount <= 0) return;
	if (UObjectPoolSubsystem* Pool = World->GetSubsystem<UObjectPoolSubsystem>())
	{
		// Multiple characters using one BP class share a minimum capacity.
		Pool->EnsurePoolCapacity(BloodStainClass, BloodStainPoolPrewarmCount);
	}
}

void UBloodTrailComponent::SpawnBloodStain()
{
	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (!World || !IsValid(OwnerActor) || !BloodStainClass) return;

	UObjectPoolSubsystem* Pool = World->GetSubsystem<UObjectPoolSubsystem>();
	if (!IsValid(Pool)) return;

	FHitResult GroundHit;
	if (!FindGroundBelow(GroundHit)) return;

	const FVector Normal = GroundHit.ImpactNormal.GetSafeNormal();
	if (Normal.IsNearlyZero() || Normal.ContainsNaN() || GroundHit.ImpactPoint.ContainsNaN()) return;
	const FVector SpawnLocation = GroundHit.ImpactPoint + Normal * 2.f;

	ABloodStainActor* BloodStain = Pool->SpawnFromPool(
		BloodStainClass, SpawnLocation, FRotator::ZeroRotator);
	if (!IsValid(BloodStain)) return;

	if (!BloodStain->InitializeBloodStain(OwnerActor, NextSequenceIndex, Normal))
	{
		// A failed initialization must not leave a checked-out actor behind.
		Pool->ReturnToPool(BloodStain);
		return;
	}

	// Only successful stains advance trail ordering, time and distance.
	++NextSequenceIndex;
	LastBloodDropTime = World->GetTimeSeconds();
	LastBloodDropLocation = OwnerActor->GetActorLocation();
}

void UBloodTrailComponent::StopBleeding()
{
	UE_LOG(LogTemp, Error, TEXT("void UBloodTrailComponent::StopBleeding()"));

	bIsBleeding = false;
	BleedingEndTime = 0.f;
}

bool UBloodTrailComponent::IsBleeding() const
{
	return bIsBleeding;
}
bool UBloodTrailComponent::ReloadSettingsFromOwnerDataTable()
{
	const FCharacterComponentSettingsRow* Row =
		CharacterComponentSettings::Resolve(GetOwner(), TEXT("BloodTrailComponent"));
	if (!Row) return false;

	// Empty class preserves the component's class assigned from a Blueprint.
	if (Row->BloodTrail.BloodStainClass)
	{
		BloodStainClass = Row->BloodTrail.BloodStainClass;
	}

	BleedingHealthThresholdPercent = FMath::Clamp(
		Row->BloodTrail.BleedingHealthThresholdPercent, 0.f, 1.f);
	HeavyDamageThreshold = FMath::Max(0.f, Row->BloodTrail.HeavyDamageThreshold);
	MinimumBloodDropInterval = FMath::Max(0.01f, Row->BloodTrail.MinimumBloodDropInterval);
	MinimumBloodDropDistance = FMath::Max(0.f, Row->BloodTrail.MinimumBloodDropDistance);
	GroundTraceDistance = FMath::Max(1.f, Row->BloodTrail.GroundTraceDistance);
	BleedingDurationAfterHeavyDamage = FMath::Max(
		0.f, Row->BloodTrail.BleedingDurationAfterHeavyDamage);
	BloodTrailTickInterval = FMath::Max(0.01f, Row->BloodTrail.BloodTrailTickInterval);

	// Tick interval can be changed at runtime without reconstructing the component.
	SetComponentTickInterval(BloodTrailTickInterval);
	// A runtime DT reload can select a different BloodStain Blueprint class.
	// Existing marks keep their original class and expire independently.
	if (HasBegunPlay()) EnsureBloodStainPool();
	return true;
}
