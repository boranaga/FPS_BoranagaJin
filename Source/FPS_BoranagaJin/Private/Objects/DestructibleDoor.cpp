#include "Objects/DestructibleDoor.h"

#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"

ADestructibleDoor::ADestructibleDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
	DoorPivot->SetupAttachment(SceneRoot);

	// Physical doors default to keeping all destruction states aligned.
	bSynchronizeMeshTransforms = true;

	// A physical door must be movable, including its simulated debris.
	DoorPivot->SetMobility(EComponentMobility::Movable);
	IntactMesh->SetMobility(EComponentMobility::Movable);
	BrokenMesh->SetMobility(EComponentMobility::Movable);
	FracturedMesh->SetMobility(EComponentMobility::Movable);

	// An additional interaction hit area, independent of the moving door mesh.
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(SceneRoot);
	InteractionVolume->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	InteractionVolume->SetBoxExtent(FVector(35.f, 110.f, 110.f));
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionVolume->SetGenerateOverlapEvents(false);
	InteractionVolume->SetCanEverAffectNavigation(false);

	/*
	 * 기존 ADestructibleObject에서는 IntactMesh와 BrokenMesh가
	 * SceneRoot에 붙어 있습니다.
	 *
	 * 여기서 DoorPivot으로 다시 지정하여 DoorPivot을 움직이거나
	 * 회전하면 문 Mesh도 함께 움직이게 합니다.
	 */
	IntactMesh->SetupAttachment(DoorPivot);
	BrokenMesh->SetupAttachment(DoorPivot);
	FracturedMesh->SetupAttachment(DoorPivot);
}

void ADestructibleDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Rebuild old Blueprints that still keep FracturedMesh under SceneRoot.
	// All three meshes must inherit DoorPivot movement, even without sync.
	if (IsValid(FracturedMesh) && IsValid(DoorPivot) &&
		FracturedMesh->GetAttachParent() != DoorPivot &&
		!FracturedMesh->IsSimulatingPhysics())
	{
		FracturedMesh->AttachToComponent(
			DoorPivot, FAttachmentTransformRules::KeepWorldTransform);
	}
	SynchronizeDestructionMeshTransforms();
	UpdateInteractionVolumeCollision();
}

void ADestructibleDoor::SetInteractionVolumeEnabled(bool bEnabled)
{
	bEnableInteractionVolume = bEnabled;
	UpdateInteractionVolumeCollision();
}

void ADestructibleDoor::UpdateInteractionVolumeCollision()
{
	if (!IsValid(InteractionVolume)) { return; }

	// The box participates only in interaction raycasts. It never blocks
	// player movement, projectiles on other channels, or overlap detection.
	InteractionVolume->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionVolume->SetGenerateOverlapEvents(false);

	const bool bShouldTrace = bEnableInteractionVolume && !bDestroyed &&
		(bInteractionVolumeActiveWhenOpen || !bIsOpen);
	InteractionVolume->SetCollisionEnabled(
		bShouldTrace ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void ADestructibleDoor::BeginPlay()
{
	Super::BeginPlay();

	// Native components and pre-existing Blueprint instances should both
	// use the same moving pivot. Avoid attaching an active Chaos simulation.
	if (IsValid(FracturedMesh) && IsValid(DoorPivot) &&
		FracturedMesh->GetAttachParent() != DoorPivot &&
		!FracturedMesh->IsSimulatingPhysics())
	{
		FracturedMesh->AttachToComponent(
			DoorPivot, FAttachmentTransformRules::KeepWorldTransform);
	}
	SynchronizeDestructionMeshTransforms();

	if (IsValid(IntactMesh))
	{
		bInitialIntactNavigationRelevant = IntactMesh->CanEverAffectNavigation();
	}

	if (!DoorPivot)
	{
		return;
	}

	ClosedRelativeLocation = DoorPivot->GetRelativeLocation();
	ClosedRelativeRotation = DoorPivot->GetRelativeRotation();

	OpenRelativeLocation = ClosedRelativeLocation + SlideOffset;
	OpenRelativeRotation = ClosedRelativeRotation + GetRotationOffset();

	bIsOpen = bStartsOpen;
	bIsMoving = false;

	if (bStartsOpen)
	{
		if (DoorOpenType == EDoorOpenType::Slide)
		{
			DoorPivot->SetRelativeLocation(OpenRelativeLocation);
		}
		else
		{
			DoorPivot->SetRelativeRotation(OpenRelativeRotation);
		}
	}
	else
	{
		DoorPivot->SetRelativeLocation(ClosedRelativeLocation);
		DoorPivot->SetRelativeRotation(ClosedRelativeRotation);
	}

	SetActorTickEnabled(false);
	SynchronizeDestructionMeshTransforms();
	UpdateInteractionVolumeCollision();
}

void ADestructibleDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsMoving || bDestroyed || !DoorPivot)
	{
		SetActorTickEnabled(false);
		return;
	}

	switch (DoorOpenType)
	{
	case EDoorOpenType::Slide:
		UpdateSlidingDoor(DeltaTime);
		break;

	case EDoorOpenType::Rotate:
		UpdateRotatingDoor(DeltaTime);
		break;

	default:
		break;
	}
}

void ADestructibleDoor::OpenDoor()
{
	SetDoorOpen(true);
}

void ADestructibleDoor::CloseDoor()
{
	SetDoorOpen(false);
}

void ADestructibleDoor::ToggleDoor()
{
	SetDoorOpen(!bIsOpen);
}

void ADestructibleDoor::SetDoorOpen(bool bOpen)
{
	if (bDestroyed || !DoorPivot)
	{
		return;
	}

	/*
	 * bIsOpen은 현재 완전히 열린 상태라기보다
	 * 문이 이동 중일 때는 목표 상태를 의미합니다.
	 */
	if (bIsOpen == bOpen && !bIsMoving)
	{
		return;
	}

	bIsOpen = bOpen;
	bIsMoving = true;

	UpdateInteractionVolumeCollision();
	SetActorTickEnabled(true);
}

bool ADestructibleDoor::IsDoorOpen() const
{
	return bIsOpen;
}

bool ADestructibleDoor::IsDoorMoving() const
{
	return bIsMoving;
}

bool ADestructibleDoor::CanEnemyBreachDoor() const
{
	if (!bAllowEnemyBreach || !CanBeDamagedByAI() || bIsOpen || bIsMoving ||
		!IsValid(IntactMesh) || !IntactMesh->IsVisible() ||
		IntactMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision ||
		IntactMesh->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block)
	{
		return false;
	}

	const float Remaining = GetRemainingDurability();
	const float Maximum = GetMaximumDurability();
	if (Maximum <= KINDA_SMALL_NUMBER) return false;
	const float Threshold = EnemyBreachMaximumDurability >= 0.f
		? EnemyBreachMaximumDurability
		: Maximum * EnemyBreachMaximumDurabilityFraction;
	return Remaining <= Threshold;
}

void ADestructibleDoor::OnAfterLoad()
{
	Super::OnAfterLoad();
	if (bClearBrokenDoorPath && bDestroyed)
	{
		if (IsValid(IntactMesh)) IntactMesh->SetCanEverAffectNavigation(false);
		if (IsValid(BrokenMesh))
		{
			BrokenMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			BrokenMesh->SetCanEverAffectNavigation(false);
		}
		if (IsValid(FracturedMesh))
		{
			FracturedMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			FracturedMesh->SetCanEverAffectNavigation(false);
		}
	}
	UpdateInteractionVolumeCollision();
}

void ADestructibleDoor::RestoreDestructibleObject()
{
	Super::RestoreDestructibleObject();
	if (bClearBrokenDoorPath && IsValid(IntactMesh))
		IntactMesh->SetCanEverAffectNavigation(bInitialIntactNavigationRelevant);
	if (IsValid(BrokenMesh)) BrokenMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	if (IsValid(FracturedMesh)) FracturedMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	UpdateInteractionVolumeCollision();
}

FVector ADestructibleDoor::GetEnemyBreachSurfacePoint(const FVector& FromLocation) const
{
	return IsValid(IntactMesh)
		? IntactMesh->Bounds.GetBox().GetClosestPointTo(FromLocation)
		: GetActorLocation();
}

void ADestructibleDoor::BreakObject(
	const FVector& HitLocation,
	const FVector& HitDirection
)
{
	//if (bDestroyed)
	//{
	//	return;
	//}

	//bIsMoving = false;
	//SetActorTickEnabled(false);

	//Super::BreakObject(HitLocation, HitDirection);


	//------------------

	if (bDestroyed)
	{
		return;
	}

	bIsMoving = false;
	SetActorTickEnabled(false);

	if (IntactMesh)
	{
		IntactMesh->SetRenderCustomDepth(false);
	}

	Super::BreakObject(
		HitLocation,
		HitDirection
	);
	UpdateInteractionVolumeCollision();

	if (bClearBrokenDoorPath)
	{
		// DestructibleObject enables physics on debris after the break.
		// Allow the character capsule to pass through the doorway, while
		// fragments may still interact with the environment and each other.
		if (IsValid(BrokenMesh))
		{
			BrokenMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			BrokenMesh->SetCanEverAffectNavigation(false);
		}
		if (IsValid(FracturedMesh))
		{
			FracturedMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			FracturedMesh->SetCanEverAffectNavigation(false);
		}
		// Dynamic Navigation runtime generation can then update the opening.
		if (IsValid(IntactMesh)) IntactMesh->SetCanEverAffectNavigation(false);
	}



}

void ADestructibleDoor::UpdateSlidingDoor(float DeltaTime)
{
	const FVector CurrentLocation = DoorPivot->GetRelativeLocation();
	const FVector TargetLocation = GetTargetLocation();

	const float InterpSpeed =
		bIsOpen ? OpenInterpSpeed : CloseInterpSpeed;

	const FVector NewLocation = FMath::VInterpTo(
		CurrentLocation,
		TargetLocation,
		DeltaTime,
		InterpSpeed
	);

	DoorPivot->SetRelativeLocation(NewLocation);

	if (NewLocation.Equals(TargetLocation, LocationTolerance))
	{
		DoorPivot->SetRelativeLocation(TargetLocation);

		bIsMoving = false;
		SetActorTickEnabled(false);
	}
}

void ADestructibleDoor::UpdateRotatingDoor(float DeltaTime)
{
	const FRotator CurrentRotation = DoorPivot->GetRelativeRotation();
	const FRotator TargetRotation = GetTargetRotation();

	const float InterpSpeed =
		bIsOpen ? OpenInterpSpeed : CloseInterpSpeed;

	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaTime,
		InterpSpeed
	);

	DoorPivot->SetRelativeRotation(NewRotation);

	if (NewRotation.Equals(TargetRotation, RotationTolerance))
	{
		DoorPivot->SetRelativeRotation(TargetRotation);

		bIsMoving = false;
		SetActorTickEnabled(false);
	}
}

FVector ADestructibleDoor::GetTargetLocation() const
{
	return bIsOpen
		? OpenRelativeLocation
		: ClosedRelativeLocation;
}

FRotator ADestructibleDoor::GetTargetRotation() const
{
	return bIsOpen
		? OpenRelativeRotation
		: ClosedRelativeRotation;
}

FRotator ADestructibleDoor::GetRotationOffset() const
{
	switch (RotationAxis)
	{
	case EDoorRotationAxis::X:
		/*
		 * Unreal의 FRotator에서 X축 회전은 Roll입니다.
		 */
		return FRotator(0.f, 0.f, OpenAngle);

	case EDoorRotationAxis::Y:
		/*
		 * Unreal의 FRotator에서 Y축 회전은 Pitch입니다.
		 */
		return FRotator(OpenAngle, 0.f, 0.f);

	case EDoorRotationAxis::Z:
		/*
		 * 일반적인 좌우 여닫이문은 Z축, 즉 Yaw 회전을 사용합니다.
		 */
		return FRotator(0.f, OpenAngle, 0.f);

	default:
		return FRotator::ZeroRotator;
	}
}

void ADestructibleDoor::Interact_Implementation(AActor* Interactor)
{
	UE_LOG(LogTemp, Error, TEXT("void ADestructibleDoor::Interact_Implementation(AActor* Interactor)"));

	if (!CanInteract_Implementation(Interactor))
	{
		return;
	}

	ToggleDoor();
}

bool ADestructibleDoor::CanInteract_Implementation(
	AActor* Interactor
) const
{
	if (bDestroyed)
	{
		return false;
	}

	if (!IsValid(Interactor))
	{
		return false;
	}

	return true;
}

FText ADestructibleDoor::GetInteractionText_Implementation(
	AActor* Interactor
) const
{
	if (bIsOpen)
	{
		return CloseInteractionText;
	}

	return OpenInteractionText;
}

void ADestructibleDoor::BeginFocus_Implementation(
	AActor* Interactor
)
{
	if (!bHighlightWhenFocused || !IntactMesh)
	{
		return;
	}

	IntactMesh->SetRenderCustomDepth(true);
	IntactMesh->SetCustomDepthStencilValue(
		HighlightStencilValue
	);
}

void ADestructibleDoor::EndFocus_Implementation(
	AActor* Interactor
)
{
	if (!IntactMesh)
	{
		return;
	}

	IntactMesh->SetRenderCustomDepth(false);
}
