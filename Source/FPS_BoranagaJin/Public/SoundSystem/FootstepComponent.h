#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FootstepComponent.generated.h"

//class ACharacter; //TODO: APawn으로 대체
class APawn;
class ACharacterPlayer;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UFootstepComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFootstepComponent();
protected:
	virtual void BeginPlay() override;
public:
	void SetFootstepMesh(USkeletalMeshComponent* InMesh);
public:
	void PlayFootstep(FName FootSocketName);
private:
	bool TraceGround(FName FootSocketName, FHitResult& OutHitResult) const;

	EPhysicalSurface GetSurfaceType(const FHitResult& HitResult) const;

	void PlayFootstepSound(EPhysicalSurface SurfaceType, const FVector& Location);

	void ReportFootstepNoise(const FVector& Location);
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep", meta = (AllowPrivateAccess = "true"))
	FName FootstepProfile = TEXT("Player");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Trace", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float TraceStartOffset = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Trace", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float TraceDistance = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|AI Hearing", meta = (AllowPrivateAccess = "true"))
	bool bReportNoise = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|AI Hearing", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float NoiseLoudness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|AI Hearing", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float NoiseMaxRange = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|AI Hearing", meta = (AllowPrivateAccess = "true"))
	FName NoiseTag = TEXT("Footstep");
private:
	//UPROPERTY()
	//TObjectPtr<ACharacter> CharacterOwner;
	//UPROPERTY()
	//TObjectPtr<ACharacterPlayer> CharacterOwner;
	UPROPERTY()
	TObjectPtr<APawn> PawnOwner;
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> FootstepMesh;
};