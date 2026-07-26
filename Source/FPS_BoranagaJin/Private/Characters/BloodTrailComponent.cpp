#include "Characters/BloodTrailComponent.h"
#include "Characters/BloodStainActor.h"
#include "Characters/HealthComponent.h"

UBloodTrailComponent::UBloodTrailComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UBloodTrailComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) { return; }
	HealthComponent = OwnerActor->FindComponentByClass<UHealthComponent>();
	LastBloodDropLocation = OwnerActor->GetActorLocation();
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

void UBloodTrailComponent::SpawnBloodStain()
{
	UE_LOG(LogTemp, Error, TEXT("void UBloodTrailComponent::SpawnBloodStain()"));

	if (!BloodStainClass || !GetWorld())
	{
		return;
	}

	AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return;
	}

	FHitResult GroundHit;

	if (!FindGroundBelow(GroundHit))
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = OwnerActor;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	//TODO: Object Pooling으로 관리하기
	ABloodStainActor* BloodStain =
		GetWorld()->SpawnActor<ABloodStainActor>(
			BloodStainClass,
			GroundHit.ImpactPoint +
			GroundHit.ImpactNormal * 2.f,
			FRotator::ZeroRotator,
			SpawnParameters
		);

	if (!BloodStain)
	{
		return;
	}

	BloodStain->InitializeBloodStain(OwnerActor, NextSequenceIndex, GroundHit.ImpactNormal);

	++NextSequenceIndex;

	LastBloodDropTime = GetWorld()->GetTimeSeconds();
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