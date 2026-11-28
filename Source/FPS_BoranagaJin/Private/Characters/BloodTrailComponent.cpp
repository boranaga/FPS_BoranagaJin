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
	bRuntimeInitialized = true;
	if (PendingBloodTrailSaveData.IsSet())
	{
		const FBloodTrailComponentSaveData Pending = PendingBloodTrailSaveData.GetValue();
		PendingBloodTrailSaveData.Reset();
		ApplyBloodTrailSaveData(Pending);
	}
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

void UBloodTrailComponent::WriteBloodTrailSaveData(FBloodTrailComponentSaveData& OutData) const
{
	OutData = FBloodTrailComponentSaveData();
	if (PendingBloodTrailSaveData.IsSet())
	{
		OutData = PendingBloodTrailSaveData.GetValue();
		return;
	}
	const UWorld* World = GetWorld();
	if (!World || !bRuntimeInitialized) return;
	const float Now = World->GetTimeSeconds();
	OutData.bHasData = true;
	OutData.bIsBleeding = IsBleeding();
	OutData.RemainingBleedingTime = OutData.bIsBleeding
		? (BleedingEndTime > 0.f ? FMath::Max(0.f, BleedingEndTime - Now) : -1.f)
		: 0.f;
	OutData.LastBloodDropAge = LastBloodDropTime == -BIG_NUMBER
		? -1.f : FMath::Max(0.f, Now - LastBloodDropTime);
	OutData.LastBloodDropLocation = LastBloodDropLocation;
	OutData.NextSequenceIndex = NextSequenceIndex;
}

void UBloodTrailComponent::LoadBloodTrailSaveData(const FBloodTrailComponentSaveData& SaveData)
{
	if (!bRuntimeInitialized || !GetWorld())
	{
		PendingBloodTrailSaveData = SaveData;
		return;
	}
	PendingBloodTrailSaveData.Reset();
	ApplyBloodTrailSaveData(SaveData);
}

void UBloodTrailComponent::ApplyBloodTrailSaveData(const FBloodTrailComponentSaveData& SaveData)
{
	const UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();
	if (!World || !IsValid(OwnerActor)) return;
	const float Now = World->GetTimeSeconds();

	// Replace the previous state, including in-place loads and legacy saves.
	StopBleeding();
	LastBloodDropTime = -BIG_NUMBER;
	LastBloodDropLocation = OwnerActor->GetActorLocation();
	NextSequenceIndex = 0;
	if (!SaveData.bHasData) return;

	if (!SaveData.LastBloodDropLocation.ContainsNaN())
		LastBloodDropLocation = SaveData.LastBloodDropLocation;
	if (FMath::IsFinite(SaveData.LastBloodDropAge) && SaveData.LastBloodDropAge >= 0.f)
		LastBloodDropTime = Now - SaveData.LastBloodDropAge;
	NextSequenceIndex = FMath::Max(0, SaveData.NextSequenceIndex);

	if (!SaveData.bIsBleeding || !FMath::IsFinite(SaveData.RemainingBleedingTime)) return;
	if (SaveData.RemainingBleedingTime == -1.f)
		StartBleeding(true);
	else if (SaveData.RemainingBleedingTime > 0.f)
		StartBleeding(false, SaveData.RemainingBleedingTime);
}

void UBloodTrailComponent::StartBleeding(bool bLoop, float duration)
{
	if (bLoop)
	{
		bIsBleeding = true;
		BleedingEndTime = 0.f; // Clear a previous timed bleeding deadline.
		return;
	}
	if (!GetWorld() || !FMath::IsFinite(duration) || duration <= 0.f)
	{
		StopBleeding();
		return;
	}
	bIsBleeding = true;
	BleedingEndTime = GetWorld()->GetTimeSeconds() + duration;
}

void UBloodTrailComponent::UpdateBleedingState()
{
	if (bIsBleeding && !IsBleeding()) StopBleeding();
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
	if (!World || !IsValid(OwnerActor) || !BloodStainClass || NextSequenceIndex == MAX_int32) return;

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
	bIsBleeding = false;
	BleedingEndTime = 0.f;
}

bool UBloodTrailComponent::IsBleeding() const
{
	if (!bIsBleeding) return false;
	const UWorld* World = GetWorld();
	return BleedingEndTime <= 0.f || (World && World->GetTimeSeconds() < BleedingEndTime);
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

void UBloodTrailComponent::AdvanceSequenceIndexPast(int32 RestoredSequenceIndex)
{
	if (RestoredSequenceIndex >= 0 && RestoredSequenceIndex < MAX_int32)
	{
		NextSequenceIndex = FMath::Max(NextSequenceIndex, RestoredSequenceIndex + 1);
		if (PendingBloodTrailSaveData.IsSet())
		{
			FBloodTrailComponentSaveData& Pending = PendingBloodTrailSaveData.GetValue();
			if (!Pending.bHasData)
			{
				Pending.bHasData = true;
				if (const AActor* OwnerActor = GetOwner())
					Pending.LastBloodDropLocation = OwnerActor->GetActorLocation();
			}
			Pending.NextSequenceIndex = FMath::Max(Pending.NextSequenceIndex, RestoredSequenceIndex + 1);
		}
	}
}
