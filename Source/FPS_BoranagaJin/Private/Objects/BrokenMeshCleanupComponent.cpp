#include "Objects/BrokenMeshCleanupComponent.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

UBrokenMeshCleanupComponent::UBrokenMeshCleanupComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBrokenMeshCleanupComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		TArray<UStaticMeshComponent*> MeshComps;
		Owner->GetComponents<UStaticMeshComponent>(MeshComps);
		for (UStaticMeshComponent* Comp : MeshComps)
		{
			if (Comp && Comp->GetName().StartsWith(TEXT("BrokenMesh")))
			{
				BrokenMeshComp = Comp;
				break;
			}
		}
	}

	if (!BrokenMeshComp)
	{
		return;
	}

	// Actors placed in a level with Static mobility make every component Static, and a Static
	// component cannot simulate physics (the fragments would hang in place).
	if (BrokenMeshComp->Mobility != EComponentMobility::Movable)
	{
		BrokenMeshComp->SetMobility(EComponentMobility::Movable);
	}

	if (bIgnorePawnsAndCamera)
	{
		BrokenMeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);              // enemies, default pawns
		BrokenMeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		BrokenMeshComp->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore); // "Player" object type (see DefaultEngine.ini)
		BrokenMeshComp->SetCollisionResponseToChannel(ECC_GameTraceChannel7, ECR_Ignore); // "ClimbWall" trace
	}

	GetWorld()->GetTimerManager().SetTimer(
		PollTimerHandle, this, &UBrokenMeshCleanupComponent::Poll, PollInterval, true);
}

void UBrokenMeshCleanupComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
		World->GetTimerManager().ClearTimer(CleanupTimerHandle);
		World->GetTimerManager().ClearTimer(FreeFallTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UBrokenMeshCleanupComponent::Poll()
{
	if (bLanded || !BrokenMeshComp)
	{
		return;
	}

	// The break has not happened yet (BrokenMesh is hidden and not simulating).
	if (!BrokenMeshComp->IsVisible() || !BrokenMeshComp->IsSimulatingPhysics())
	{
		SinceBreak = 0.f;
		SettledTime = 0.f;
		return;
	}

	if (!bBreakSeen)
	{
		bBreakSeen = true;
		BeginFreeFall();
	}

	SinceBreak += PollInterval;

	bool bSettled = false;
	if (SinceBreak >= IgnoreInitialTime + FreeFallDuration)
	{
		const float LinearSpeed = BrokenMeshComp->GetPhysicsLinearVelocity().Size();
		const float AngularSpeed = BrokenMeshComp->GetPhysicsAngularVelocityInDegrees().Size();
		bSettled = !BrokenMeshComp->RigidBodyIsAwake()
			|| (LinearSpeed <= SettleLinearSpeed && AngularSpeed <= SettleAngularSpeed);
	}

	SettledTime = bSettled ? SettledTime + PollInterval : 0.f;

	if (SettledTime >= SettleDuration || SinceBreak >= MaxFallWaitTime)
	{
		bLanded = true;
		UWorld* World = GetWorld();
		World->GetTimerManager().ClearTimer(PollTimerHandle);
		World->GetTimerManager().SetTimer(
			CleanupTimerHandle, this, &UBrokenMeshCleanupComponent::Cleanup, FMath::Max(DelayAfterLanding, 0.01f), false);
	}
}

void UBrokenMeshCleanupComponent::BeginFreeFall()
{
	// Physics-only: still collides with the floor in the simulation, but is invisible to scene queries
	// (player movement sweeps / step-up / mantle traces), so the wreck can never snag the player.
	if (bIgnorePawnsAndCamera)
	{
		BrokenMeshComp->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	}

	if (AActor* Owner = GetOwner())
	{
		TArray<UStaticMeshComponent*> MeshComps;
		Owner->GetComponents<UStaticMeshComponent>(MeshComps);
		for (UStaticMeshComponent* Comp : MeshComps)
		{
			if (Comp && Comp->GetName().StartsWith(TEXT("IntactMesh")))
			{
				// Undo whatever the break impulse did in the last few ms.
				BrokenMeshComp->SetWorldTransform(Comp->GetComponentTransform(), false, nullptr, ETeleportType::TeleportPhysics);
				break;
			}
		}
	}

	if (FreeFallDuration > 0.f)
	{
		SavedStaticResponse = BrokenMeshComp->GetCollisionResponseToChannel(ECC_WorldStatic);
		BrokenMeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
		GetWorld()->GetTimerManager().SetTimer(
			FreeFallTimerHandle, this, &UBrokenMeshCleanupComponent::EndFreeFall, FreeFallDuration, false);
	}
	BrokenMeshComp->SetPhysicsLinearVelocity(FVector(0.f, 0.f, -InitialDownSpeed));
	BrokenMeshComp->SetPhysicsAngularVelocityInDegrees(FVector(FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(-90.f, 90.f)));
	BrokenMeshComp->WakeAllRigidBodies();
}

void UBrokenMeshCleanupComponent::EndFreeFall()
{
	if (BrokenMeshComp)
	{
		BrokenMeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, SavedStaticResponse);
	}
}

void UBrokenMeshCleanupComponent::Cleanup()
{
	if (AActor* Owner = GetOwner())
	{
		Owner->Destroy();
	}
}
