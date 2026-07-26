#include "Characters/InteractionComponent.h"
#include "Characters/Player/CharacterPlayer.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Interface/InteractableInterface.h"
#include "TimerManager.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacterPlayer>(GetOwner());

	if (!OwnerCharacter)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"UInteractionComponent must be attached to an ACharacter. Owner: %s"
			),
			*GetNameSafe(GetOwner())
		);

		return;
	}

	OwnerPlayerController = Cast<APlayerController>(
		OwnerCharacter->GetController()
	);

	/*
	 * BeginPlay 시점에 Controller가 아직 할당되지 않은 상황을 대비해
	 * UpdateInteractionTarget()에서도 다시 가져옵니다.
	 */
	GetWorld()->GetTimerManager().SetTimer(
		InteractionTraceTimerHandle,
		this,
		&UInteractionComponent::UpdateInteractionTarget,
		TraceInterval,
		true,
		0.f
	);
}

void UInteractionComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason
)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(
			InteractionTraceTimerHandle
		);
	}

	ClearInteractionTarget();

	Super::EndPlay(EndPlayReason);
}

void UInteractionComponent::TryInteract()
{
	//UE_LOG(LogTemp, Error, TEXT("void UInteractionComponent::TryInteract()"));

	if (!IsValid(CurrentInteractionTarget))
	{
		//UE_LOG(LogTemp, Error, TEXT("void UInteractionComponent::TryInteract() 1"));
		return;
	}

	if (!CanInteractWithActor(CurrentInteractionTarget))
	{
		ClearInteractionTarget();
		//UE_LOG(LogTemp, Error, TEXT("void UInteractionComponent::TryInteract() 2"));
		return;
	}

	IInteractableInterface::Execute_Interact(CurrentInteractionTarget, GetOwner());
}

void UInteractionComponent::UpdateInteractionTarget()
{
	if (!IsValid(OwnerCharacter))
	{
		return;
	}

	if (!IsValid(OwnerPlayerController))
	{
		OwnerPlayerController = Cast<APlayerController>(
			OwnerCharacter->GetController()
		);
	}

	FHitResult HitResult;

	if (!PerformInteractionTrace(HitResult))
	{
		//UE_LOG(LogTemp, Error, TEXT("void UInteractionComponent::UpdateInteractionTarget() 1"));
		ClearInteractionTarget();
		return;
	}

	AActor* HitActor = HitResult.GetActor();

	if (!IsActorInteractable(HitActor))
	{
		//UE_LOG(LogTemp, Error, TEXT("void UInteractionComponent::UpdateInteractionTarget() 2"));
		ClearInteractionTarget();
		return;
	}

	if (!CanInteractWithActor(HitActor))
	{
		//UE_LOG(LogTemp, Error, TEXT("void UInteractionComponent::UpdateInteractionTarget() 3"));
		ClearInteractionTarget();
		return;
	}

	SetInteractionTarget(HitActor);
}

bool UInteractionComponent::PerformInteractionTrace(FHitResult& OutHitResult) const
{
	if (!GetWorld() || !IsValid(OwnerCharacter))
	{
		return false;
	}

	const FVector TraceStart = GetTraceStart();
	const FVector TraceEnd =
		TraceStart + GetTraceDirection() * InteractionDistance;

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(InteractionTrace),
		bTraceComplex,
		GetOwner()
	);

	QueryParams.AddIgnoredActor(GetOwner());

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		OutHitResult,
		TraceStart,
		TraceEnd,
		InteractionTraceChannel,
		QueryParams
	);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebugTrace)
	{
		const FColor DebugColor = bHit
			? FColor::Green
			: FColor::Red;

		DrawDebugLine(
			GetWorld(),
			TraceStart,
			TraceEnd,
			DebugColor,
			false,
			TraceInterval,
			0,
			1.5f
		);

		if (bHit)
		{
			DrawDebugSphere(
				GetWorld(),
				OutHitResult.ImpactPoint,
				8.f,
				12,
				DebugColor,
				false,
				TraceInterval
			);
		}
	}
#endif

	return bHit;
}

void UInteractionComponent::SetInteractionTarget(
	AActor* NewTarget
)
{
	if (CurrentInteractionTarget == NewTarget)
	{
		return;
	}

	if (IsValid(CurrentInteractionTarget))
	{
		IInteractableInterface::Execute_EndFocus(
			CurrentInteractionTarget,
			GetOwner()
		);
	}

	CurrentInteractionTarget = NewTarget;

	if (IsValid(CurrentInteractionTarget))
	{
		IInteractableInterface::Execute_BeginFocus(
			CurrentInteractionTarget,
			GetOwner()
		);
	}

	OnInteractionTargetChanged.Broadcast(
		CurrentInteractionTarget
	);
}

void UInteractionComponent::ClearInteractionTarget()
{
	if (!IsValid(CurrentInteractionTarget))
	{
		CurrentInteractionTarget = nullptr;
		return;
	}

	IInteractableInterface::Execute_EndFocus(
		CurrentInteractionTarget,
		GetOwner()
	);

	CurrentInteractionTarget = nullptr;

	OnInteractionTargetChanged.Broadcast(nullptr);
}

bool UInteractionComponent::IsActorInteractable(
	AActor* TargetActor
) const
{
	return IsValid(TargetActor)
		&& TargetActor->Implements<UInteractableInterface>();
}

bool UInteractionComponent::CanInteractWithActor(
	AActor* TargetActor
) const
{
	if (!IsActorInteractable(TargetActor))
	{
		return false;
	}

	return IInteractableInterface::Execute_CanInteract(
		TargetActor,
		GetOwner()
	);
}

FVector UInteractionComponent::GetTraceStart() const
{
	if (IsValid(OwnerPlayerController))
	{
		FVector ViewLocation;
		FRotator ViewRotation;

		OwnerPlayerController->GetPlayerViewPoint(
			ViewLocation,
			ViewRotation
		);

		return ViewLocation;
	}

	return OwnerCharacter
		? OwnerCharacter->GetPawnViewLocation()
		: GetOwner()->GetActorLocation();
}

FVector UInteractionComponent::GetTraceDirection() const
{
	if (IsValid(OwnerPlayerController))
	{
		FVector ViewLocation;
		FRotator ViewRotation;

		OwnerPlayerController->GetPlayerViewPoint(
			ViewLocation,
			ViewRotation
		);

		return ViewRotation.Vector();
	}

	return OwnerCharacter
		? OwnerCharacter->GetBaseAimRotation().Vector()
		: GetOwner()->GetActorForwardVector();
}

AActor* UInteractionComponent::GetCurrentInteractionTarget() const
{
	return CurrentInteractionTarget;
}

bool UInteractionComponent::HasInteractionTarget() const
{
	return IsValid(CurrentInteractionTarget);
}

FText UInteractionComponent::GetCurrentInteractionText() const
{
	if (!IsValid(CurrentInteractionTarget))
	{
		return FText::GetEmpty();
	}

	if (!CurrentInteractionTarget->Implements<UInteractableInterface>())
	{
		return FText::GetEmpty();
	}

	return IInteractableInterface::Execute_GetInteractionText(
		CurrentInteractionTarget,
		GetOwner()
	);
}