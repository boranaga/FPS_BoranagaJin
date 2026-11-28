#include "Characters/BloodStainActor.h"

#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "ObjectPoolSubsystem.h"
#include "TimerManager.h"

ABloodStainActor::ABloodStainActor()
{
	PrimaryActorTick.bCanEverTick = false;
	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("BloodDecalComponent"));
	SetRootComponent(DecalComponent);
	// The local X axis is the decal projection direction.
	DecalComponent->DecalSize = FVector(10.f, 24.f, 24.f);

	// Retain the existing editable InitialLifeSpan default for BP compatibility.
	// The actual per-lease lifetime is managed by BloodLifeTimer.
	InitialLifeSpan = 30.f;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ABloodStainActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Capture BP instance settings before BeginPlay or pool callbacks can clear them.
	ConfiguredLifeSpan = FMath::IsFinite(InitialLifeSpan) ? FMath::Max(0.f, InitialLifeSpan) : 30.f;
	if (IsValid(DecalComponent))
	{
		ConfiguredFadeStartDelay = DecalComponent->FadeStartDelay;
		ConfiguredFadeDuration = DecalComponent->FadeDuration;
		ConfiguredFadeInStartDelay = DecalComponent->FadeInStartDelay;
		ConfiguredFadeInDuration = DecalComponent->FadeInDuration;
		DecalComponent->bDestroyOwnerAfterFade = false;
	}
}

void ABloodStainActor::BeginPlay()
{
	Super::BeginPlay();

	// AActor and UDecalComponent may start native destruction timers in BeginPlay.
	// Do not clear BloodLifeTimer here: a lease may have been initialized before BeginPlay.
	SetLifeSpan(0.f);
	if (IsValid(DecalComponent)) DecalComponent->SetLifeSpan(0.f);
	if (!bBloodStainInitialized)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		SetActorTickEnabled(false);
		if (IsValid(DecalComponent)) DecalComponent->SetVisibility(false);
	}
}

void ABloodStainActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// World teardown and external destruction are not normal pool returns.
	StopLifetimeTimers();
	bBloodStainInitialized = false;
	bIsActiveInPool = false;
	TrailOwner.Reset();
	SequenceIndex = INDEX_NONE;
	BloodSpawnTime = 0.f;
	OwningPool.Reset();
	Super::EndPlay(EndPlayReason);
}

void ABloodStainActor::SetOwningPool(UObjectPoolSubsystem* NewPool)
{
	OwningPool = NewPool;
}

void ABloodStainActor::OnActivateFromPool()
{
	// The subsystem has already applied the requested world location.
	// Reset only per-lease state; do not move the actor away from that location.
	ResetBloodStainRuntime();
	bIsActiveInPool = true;
}

void ABloodStainActor::OnDeactivateToPool()
{
	bIsActiveInPool = false;
	ResetBloodStainRuntime();
}

void ABloodStainActor::StopLifetimeTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BloodLifeTimer);
		SetLifeSpan(0.f);
		if (IsValid(DecalComponent)) DecalComponent->SetLifeSpan(0.f);
	}
}

void ABloodStainActor::ResetBloodStainRuntime()
{
	bBloodStainInitialized = false;
	StopLifetimeTimers();
	TrailOwner.Reset();
	SequenceIndex = INDEX_NONE;
	BloodSpawnTime = 0.f;
	InstanceID.Invalidate();
	SavedBloodAge = -1.f;
	SavedRemainingLife = -2.f;
	bSavedOwnerIsPlayer = false;
	SavedTrailOwnerID.Invalidate();
	SavedPlacedOwnerName = NAME_None;
	SetOwner(nullptr);
	SetInstigator(nullptr);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	SetActorScale3D(FVector::OneVector);

	if (IsValid(DecalComponent))
	{
		DecalComponent->SetVisibility(false);
		// Clear previous fade state without destroying the component or its actor.
		if (GetWorld())
		{
			DecalComponent->SetFadeOut(0.f, 0.f, false);
			DecalComponent->SetFadeIn(0.f, 0.f);
			DecalComponent->SetLifeSpan(0.f);
		}
	}
}

void ABloodStainActor::ConfigureDecalFade(float Age)
{
	// Resume the remaining fade schedule; a partially completed fade restarts
	// from the visible endpoint over its remaining duration (no material change).
	const auto RemainingFade = [Age](float Delay, float Duration, float& OutDelay, float& OutDuration)
		{
			Delay = FMath::Max(0.f, Delay);
			Duration = FMath::Max(0.f, Duration);
			OutDelay = FMath::Max(0.f, Delay - Age);
			OutDuration = FMath::Max(0.f, Duration - FMath::Max(0.f, Age - Delay));
		};
	float InDelay, InDuration, OutDelay, OutDuration;
	RemainingFade(ConfiguredFadeInStartDelay, ConfiguredFadeInDuration, InDelay, InDuration);
	RemainingFade(ConfiguredFadeStartDelay, ConfiguredFadeDuration, OutDelay, OutDuration);
	DecalComponent->SetFadeIn(InDelay, InDuration);
	DecalComponent->SetFadeOut(OutDelay, OutDuration, false);
	DecalComponent->SetLifeSpan(0.f);
}

void ABloodStainActor::ShowBloodStain(float Age, float RemainingLife)
{
	StopLifetimeTimers();
	BloodSpawnTime = GetWorld()->GetTimeSeconds() - Age;
	bBloodStainInitialized = true;
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	ConfigureDecalFade(Age);
	SetActorHiddenInGame(false);
	DecalComponent->SetVisibility(true);
	if (RemainingLife > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(BloodLifeTimer, this,
			&ABloodStainActor::ReleaseBloodStain, RemainingLife, false);
	}
}

bool ABloodStainActor::InitializeBloodStain(AActor* InTrailOwner, int32 InSequenceIndex, const FVector& SurfaceNormal)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(InTrailOwner) || InTrailOwner->GetWorld() != World ||
		InSequenceIndex < 0 || InSequenceIndex == MAX_int32 || !IsValid(DecalComponent) || SurfaceNormal.ContainsNaN() ||
		SurfaceNormal.IsNearlyZero() || (OwningPool.IsValid() && !bIsActiveInPool) ||
		bBloodStainInitialized)
	{
		return false;
	}

	StopLifetimeTimers();
	const FVector Normal = SurfaceNormal.GetSafeNormal();
	FVector Tangent = FVector::VectorPlaneProject(FMath::VRand(), Normal).GetSafeNormal();
	if (Tangent.IsNearlyZero())
	{
		const FVector Axis = FMath::Abs(Normal.Z) < 0.99f ? FVector::UpVector : FVector::ForwardVector;
		Tangent = FVector::CrossProduct(Normal, Axis).GetSafeNormal();
	}

	// Preserve the original surface projection, random orientation and scale.
	SetActorRotation(UKismetMathLibrary::MakeRotFromXZ(-Normal, Tangent));
	SetActorScale3D(FVector(FMath::FRandRange(0.8f, 1.2f)));
	SetOwner(InTrailOwner);
	TrailOwner = InTrailOwner;
	SequenceIndex = InSequenceIndex;
	InstanceID = FGuid::NewGuid();

	// InitialLifeSpan == 0 retains the previous infinite-lifetime convention.
	float LeaseDuration = ConfiguredLifeSpan;
	const float FadeEndTime = ConfiguredFadeStartDelay + ConfiguredFadeDuration;
	if (FMath::IsFinite(FadeEndTime) && FadeEndTime > 0.f)
	{
		LeaseDuration = LeaseDuration > 0.f ? FMath::Min(LeaseDuration, FadeEndTime) : FadeEndTime;
	}
	ShowBloodStain(0.f, LeaseDuration > 0.f ? LeaseDuration : -1.f);
	return true;
}

void ABloodStainActor::LifeSpanExpired()
{
	// Protect pooled actors even if external code uses the inherited SetLifeSpan API.
	if (OwningPool.IsValid()) ReleaseBloodStain();
	else Super::LifeSpanExpired();
}

void ABloodStainActor::ReleaseBloodStain()
{
	if (UObjectPoolSubsystem* Pool = OwningPool.Get())
	{
		if (bIsActiveInPool) Pool->ReturnToPool(this);
		return;
	}

	// Compatibility for a directly spawned, non-pooled stain.
	ResetBloodStainRuntime();
	Destroy();
}

AActor* ABloodStainActor::GetTrailOwner() const
{
	return bBloodStainInitialized ? TrailOwner.Get() : nullptr;
}

int32 ABloodStainActor::GetSequenceIndex() const
{
	return bBloodStainInitialized ? SequenceIndex : INDEX_NONE;
}

float ABloodStainActor::GetBloodSpawnTime() const
{
	return bBloodStainInitialized ? BloodSpawnTime : 0.f;
}

FGuid ABloodStainActor::GetInstanceID() const
{
	return bBloodStainInitialized ? InstanceID : FGuid();
}

float ABloodStainActor::GetRemainingBloodLife() const
{
	UWorld* World = GetWorld();
	if (!World || !bBloodStainInitialized) return 0.f;
	const float LeaseRemaining = World->GetTimerManager().GetTimerRemaining(BloodLifeTimer);
	// Respect an earlier expiry requested through the inherited SetLifeSpan API.
	const float NativeRemaining = GetLifeSpan();
	if (NativeRemaining > 0.f)
	{
		return LeaseRemaining >= 0.f ? FMath::Min(LeaseRemaining, NativeRemaining) : NativeRemaining;
	}
	return LeaseRemaining >= 0.f ? LeaseRemaining : -1.f;
}

bool ABloodStainActor::ShouldSaveBloodStain() const
{
	if (!bBloodStainInitialized || !InstanceID.IsValid() || !IsValid(DecalComponent)) return false;
	const float Remaining = GetRemainingBloodLife();
	return FMath::IsFinite(Remaining) && (Remaining == -1.f || Remaining > 0.f);
}

void ABloodStainActor::OnBeforeSave()
{
	if (!ShouldSaveBloodStain()) return;
	SavedBloodAge = FMath::Max(0.f, static_cast<float>(GetWorld()->GetTimeSeconds() - BloodSpawnTime));
	SavedRemainingLife = GetRemainingBloodLife();
	bSavedOwnerIsPlayer = false;
	SavedTrailOwnerID.Invalidate();
	SavedPlacedOwnerName = NAME_None;
	if (AActor* OwnerActor = TrailOwner.Get())
	{
		// CharacterPlayer uses ISaveablePlayerInterface rather than a world GUID.
		bSavedOwnerIsPlayer = OwnerActor == UGameplayStatics::GetPlayerPawn(this, 0);
		if (bSavedOwnerIsPlayer) return;
		if (const ISaveableActorInterface* SaveableOwner = Cast<ISaveableActorInterface>(OwnerActor))
		{
			SavedTrailOwnerID = SaveableOwner->GetInstanceID();
			if (SavedTrailOwnerID.IsValid()) return;
		}
		// Never match a dynamically generated Actor by its potentially reused name.
		if (OwnerActor->GetLevel() == GetWorld()->PersistentLevel && OwnerActor->HasAnyFlags(RF_WasLoaded))
		{
			SavedPlacedOwnerName = OwnerActor->GetFName();
		}
	}
}

void ABloodStainActor::PrepareRestoredBloodStain(const FGuid& SavedID, AActor* ResolvedOwner)
{
	if (!bIsActiveInPool || bBloodStainInitialized) return;
	InstanceID = SavedID;
	if (IsValid(ResolvedOwner) && ResolvedOwner->GetWorld() == GetWorld())
	{
		TrailOwner = ResolvedOwner;
		SetOwner(ResolvedOwner);
	}
}

void ABloodStainActor::OnAfterLoad()
{
	if (!bIsActiveInPool || bBloodStainInitialized) return;
	if (!GetWorld() || !IsValid(DecalComponent) || !InstanceID.IsValid() ||
		SequenceIndex < 0 || SequenceIndex == MAX_int32 ||
		!FMath::IsFinite(SavedBloodAge) || SavedBloodAge < 0.f ||
		!FMath::IsFinite(SavedRemainingLife) ||
		(SavedRemainingLife != -1.f && SavedRemainingLife <= 0.f))
	{
		ReleaseBloodStain();
		return;
	}
	// Transform is already restored by the subsystem. Do not randomize it again.
	// BloodSpawnTime is rebased to this world's clock (it may be negative).
	ShowBloodStain(SavedBloodAge, SavedRemainingLife);
}
