#include "Items/Projectile/ProjectileShell.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ObjectPoolSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "PhysicsEngine/BodyInstance.h"

AProjectileShell::AProjectileShell()
{
	// Chaos and a one-shot timer handle the shell; no Actor Tick is needed.
	PrimaryActorTick.bCanEverTick = false;
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->SetCollisionObjectType(ECC_GameTraceChannel4);
	CapsuleComponent->SetCollisionProfileName(FName(TEXT("ProjectileShell")));
	CapsuleComponent->InitCapsuleSize(4.f, 10.f);
	CapsuleComponent->SetSimulatePhysics(false);

	ProjectileShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileShell"));
	ProjectileShellMesh->SetupAttachment(RootComponent);
	ProjectileShellMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileShellMesh->SetCastShadow(false);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	InitialLifeSpan = 0.f;
}

void AProjectileShell::BeginPlay()
{
	Super::BeginPlay();
	// Read the actual BP/profile collision setting before temporarily disabling it.
	if (const FBodyInstance* Body = CapsuleComponent->GetBodyInstance())
	{
		ActiveCollisionMode = Body->GetCollisionEnabled(false);
	}
	InitShell();
}

void AProjectileShell::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bIsActiveInPool = false;
	ResetShellRuntime();
	OwningPool = nullptr;
	Super::EndPlay(EndPlayReason);
}

void AProjectileShell::SetOwningPool(UObjectPoolSubsystem* NewPool)
{
	OwningPool = NewPool;
}

void AProjectileShell::OnActivateFromPool()
{
	bIsActiveInPool = true;
	// The shell stays inert until EjectShell installs the new transform and impulse.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void AProjectileShell::OnDeactivateToPool()
{
	bIsActiveInPool = false;
	ResetShellRuntime();
}

void AProjectileShell::InitShell()
{
	ResetShellRuntime();
}

void AProjectileShell::ResetShellRuntime()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(InvisibleTimer);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	if (IsValid(ProjectileShellMesh)) ProjectileShellMesh->SetVisibility(false);
	if (IsValid(CapsuleComponent))
	{
		if (CapsuleComponent->IsSimulatingPhysics())
		{
			CapsuleComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			CapsuleComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		CapsuleComponent->SetSimulatePhysics(false);
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	SetActorEnableCollision(false);
	SetOwner(nullptr);
	SetInstigator(nullptr);
}

void AProjectileShell::HideProjectileShell()
{
	if (IsValid(OwningPool))
	{
		if (bIsActiveInPool) OwningPool->ReturnToPool(this);
		return;
	}
	ResetShellRuntime();
	Destroy();
}

void AProjectileShell::FellOutOfWorld(const UDamageType& dmgType)
{
	HideProjectileShell();
}

void AProjectileShell::OutsideWorldBounds()
{
	HideProjectileShell();
}

void AProjectileShell::EjectShell(const FVector EjectLocation, const FRotator EjectRotation, const FVector EjectImpulse)
{
	if (IsValid(OwningPool) && !bIsActiveInPool) return;
	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(CapsuleComponent) || !IsValid(ProjectileShellMesh) ||
		!FMath::IsFinite(VisibleDuration) || VisibleDuration <= 0.f ||
		EjectLocation.ContainsNaN() || EjectRotation.ContainsNaN() || EjectImpulse.ContainsNaN())
	{
		HideProjectileShell();
		return;
	}
	ResetShellRuntime();
	SetActorLocationAndRotation(EjectLocation, EjectRotation, false, nullptr, ETeleportType::TeleportPhysics);
	CapsuleComponent->SetCollisionEnabled(ActiveCollisionMode);
	SetActorEnableCollision(true);
	CapsuleComponent->SetSimulatePhysics(true);
	if (!CapsuleComponent->IsSimulatingPhysics())
	{
		HideProjectileShell();
		return;
	}
	CapsuleComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CapsuleComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	ProjectileShellMesh->SetVisibility(true);
	SetActorHiddenInGame(false);
	CapsuleComponent->AddImpulse(EjectImpulse, NAME_None, false);
	World->GetTimerManager().SetTimer(InvisibleTimer, this, &AProjectileShell::HideProjectileShell, VisibleDuration, false);
}
