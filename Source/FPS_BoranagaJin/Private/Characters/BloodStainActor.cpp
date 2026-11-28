#include "Characters/BloodStainActor.h"

#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
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

void ABloodStainActor::ConfigureDecalFade()
{
	// Restore the visual fade for this lease. Decal lifetime must not destroy
	// the component; the actor's own timer is responsible for pool return.
	DecalComponent->SetFadeIn(ConfiguredFadeInStartDelay, ConfiguredFadeInDuration);
	DecalComponent->SetFadeOut(ConfiguredFadeStartDelay, ConfiguredFadeDuration, false);
	DecalComponent->SetLifeSpan(0.f);
}

bool ABloodStainActor::InitializeBloodStain(AActor* InTrailOwner, int32 InSequenceIndex, const FVector& SurfaceNormal)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(InTrailOwner) || InTrailOwner->GetWorld() != World ||
		InSequenceIndex < 0 || !IsValid(DecalComponent) || SurfaceNormal.ContainsNaN() ||
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
	BloodSpawnTime = World->GetTimeSeconds();
	bBloodStainInitialized = true;

	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	ConfigureDecalFade();
	SetActorHiddenInGame(false);
	DecalComponent->SetVisibility(true);

	// InitialLifeSpan == 0 retains the previous infinite-lifetime convention.
	// If a BP fade finishes earlier, retire the marker when it becomes invisible.
	float LeaseDuration = ConfiguredLifeSpan;
	const float FadeEndTime = DecalComponent->FadeStartDelay + DecalComponent->FadeDuration;
	if (FMath::IsFinite(FadeEndTime) && FadeEndTime > 0.f)
	{
		LeaseDuration = LeaseDuration > 0.f ? FMath::Min(LeaseDuration, FadeEndTime) : FadeEndTime;
	}
	if (LeaseDuration > 0.f)
	{
		World->GetTimerManager().SetTimer(BloodLifeTimer, this, &ABloodStainActor::ReleaseBloodStain, LeaseDuration, false);
	}
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
