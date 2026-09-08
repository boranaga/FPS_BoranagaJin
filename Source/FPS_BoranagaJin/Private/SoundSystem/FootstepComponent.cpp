#include "SoundSystem/FootstepComponent.h"

#include "SoundSystem/GameAudioSubsystem.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
//#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Perception/AISense_Hearing.h"
#include "PhysicalMaterials/PhysicalMaterial.h"


UFootstepComponent::UFootstepComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFootstepComponent::BeginPlay()
{
	Super::BeginPlay();

	PawnOwner = Cast<APawn>(GetOwner());

	if (!IsValid(PawnOwner))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("UFootstepComponent: Owner must be ACharacter. Owner: %s"),
			*GetNameSafe(GetOwner())
		);
	}
}

void UFootstepComponent::SetFootstepMesh(USkeletalMeshComponent* InMesh)
{
	FootstepMesh = InMesh;
}

void UFootstepComponent::PlayFootstep(FName FootSocketName)
{
	if (!IsValid(PawnOwner))
	{
		return;
	}

	FHitResult HitResult;

	if (!TraceGround(FootSocketName, HitResult))
	{
		return;
	}

	const EPhysicalSurface SurfaceType =
		GetSurfaceType(HitResult);

	PlayFootstepSound(
		SurfaceType,
		HitResult.ImpactPoint
	);

	if (bReportNoise)
	{
		ReportFootstepNoise(
			HitResult.ImpactPoint
		);
	}
}

bool UFootstepComponent::TraceGround(FName FootSocketName, FHitResult& OutHitResult) const
{
	if (!IsValid(PawnOwner)) { return false; }
	if (!IsValid(FootstepMesh)) { return false; }

	FVector FootLocation;
	if (!FootSocketName.IsNone() && FootstepMesh->DoesSocketExist(FootSocketName))
	{
		FootLocation = FootstepMesh->GetSocketLocation(FootSocketName);
	}
	else
	{
		FootLocation = PawnOwner->GetActorLocation();
	}

	const FVector TraceStart = FootLocation + FVector::UpVector * TraceStartOffset;
	const FVector TraceEnd = FootLocation - FVector::UpVector * TraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PawnOwner);

	/*
	 * 이것이 반드시 true여야 PhysicalMaterial을 받을 수 있다.
	 */
	QueryParams.bReturnPhysicalMaterial = true;

	return GetWorld()->LineTraceSingleByChannel(
		OutHitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);
}

EPhysicalSurface UFootstepComponent::GetSurfaceType(
	const FHitResult& HitResult) const
{
	if (!HitResult.PhysMaterial.IsValid())
	{
		return SurfaceType_Default;
	}

	return UPhysicalMaterial::DetermineSurfaceType(
		HitResult.PhysMaterial.Get()
	);
}

void UFootstepComponent::PlayFootstepSound(
	EPhysicalSurface SurfaceType,
	const FVector& Location)
{
	if (!IsValid(PawnOwner))
	{
		return;
	}

	UGameInstance* GameInstance =
		PawnOwner->GetGameInstance();

	if (!IsValid(GameInstance))
	{
		return;
	}

	UGameAudioSubsystem* AudioSubsystem =
		GameInstance->GetSubsystem<UGameAudioSubsystem>();

	if (!IsValid(AudioSubsystem))
	{
		return;
	}

	AudioSubsystem->PlayFootstepSound(
		FootstepProfile,
		SurfaceType,
		Location
	);
}

void UFootstepComponent::ReportFootstepNoise(
	const FVector& Location)
{
	if (!IsValid(PawnOwner))
	{
		return;
	}

	UAISense_Hearing::ReportNoiseEvent(
		this,
		Location,
		NoiseLoudness,
		PawnOwner,
		NoiseMaxRange,
		NoiseTag
	);
}






