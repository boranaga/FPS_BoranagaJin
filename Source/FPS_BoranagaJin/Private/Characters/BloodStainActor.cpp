#include "Characters/BloodStainActor.h"

#include "Components/DecalComponent.h"
#include "Kismet/KismetMathLibrary.h"

ABloodStainActor::ABloodStainActor()
{
	PrimaryActorTick.bCanEverTick = false;

	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("BloodDecalComponent"));

	SetRootComponent(DecalComponent);

	/*
	 * DecalComponent의 X축이 투영 방향이다.
	 * 혈흔 크기는 프로젝트에 맞게 조정한다.
	 */
	DecalComponent->DecalSize = FVector(10.f, 24.f, 24.f);

	InitialLifeSpan = 30.f;
}

void ABloodStainActor::BeginPlay()
{
	Super::BeginPlay();

	BloodSpawnTime = GetWorld()->GetTimeSeconds();
}

void ABloodStainActor::InitializeBloodStain(AActor* InTrailOwner, int32 InSequenceIndex, const FVector& SurfaceNormal)
{
	TrailOwner = InTrailOwner;
	SequenceIndex = InSequenceIndex;
	BloodSpawnTime = GetWorld()->GetTimeSeconds();

	const FVector RandomForward =
		FVector::VectorPlaneProject(
			FMath::VRand(),
			SurfaceNormal
		).GetSafeNormal();

	FVector ForwardDirection = RandomForward;

	if (ForwardDirection.IsNearlyZero())
	{
		ForwardDirection = FVector::ForwardVector;
	}

	/*
	 * Decal의 투영 방향을 바닥의 반대 방향으로 맞춘다.
	 */
	const FRotator DecalRotation =
		UKismetMathLibrary::MakeRotFromXZ(
			-SurfaceNormal,
			ForwardDirection
		);

	SetActorRotation(DecalRotation);

	const float RandomScale = FMath::FRandRange(0.8f, 1.2f);
	SetActorScale3D(FVector(RandomScale));
}

AActor* ABloodStainActor::GetTrailOwner() const
{
	return TrailOwner.Get();
}

int32 ABloodStainActor::GetSequenceIndex() const
{
	return SequenceIndex;
}

float ABloodStainActor::GetBloodSpawnTime() const
{
	return BloodSpawnTime;
}