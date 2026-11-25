#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BrokenMeshCleanupComponent.generated.h"

class UStaticMeshComponent;

/**
 * Attach to a ADestructibleObject blueprint.
 * Watches the "BrokenMesh" component; once the fragments have fallen and come to rest on
 * the floor, destroys the owning actor after DelayAfterLanding seconds.
 * Uses only the public engine API, so ADestructibleObject itself is untouched.
 */
UCLASS(ClassGroup = (Destruction), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UBrokenMeshCleanupComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBrokenMeshCleanupComponent();

	// Seconds between landing and the owner being destroyed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BrokenMesh", meta = (ClampMin = "0.0"))
	float DelayAfterLanding = 6.f;

	// Fragments count as landed once linear speed (cm/s) stays below this...
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BrokenMesh", meta = (ClampMin = "0.0"))
	float SettleLinearSpeed = 20.f;

	// ...and angular speed (deg/s) stays below this...
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BrokenMesh", meta = (ClampMin = "0.0"))
	float SettleAngularSpeed = 30.f;

	// ...for this many seconds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BrokenMesh", meta = (ClampMin = "0.0"))
	float SettleDuration = 0.5f;

	// Safety net: treat as landed after this long even if the fragments never settle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BrokenMesh", meta = (ClampMin = "0.0"))
	float MaxFallWaitTime = 15.f;

	// The fallen fragments must not push, block or trip the player / enemies / camera.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BrokenMesh")
	bool bIgnorePawnsAndCamera = true;

	// Optional: for this long after the break the fragments ignore static geometry, so they can escape
	// a ceiling they start inside of. 0 = off (fragments near the floor would fall through it).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BrokenMesh", meta = (ClampMin = "0.0"))
	float FreeFallDuration = 0.f;

	// Downward speed given to the fragments at the moment of the break (replaces the break impulse).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BrokenMesh")
	float InitialDownSpeed = 100.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Poll();
	void Cleanup();
	void BeginFreeFall();
	void EndFreeFall();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BrokenMeshComp;

	FTimerHandle PollTimerHandle;
	FTimerHandle CleanupTimerHandle;

	float SinceBreak = 0.f;
	float SettledTime = 0.f;
	bool bLanded = false;
	bool bBreakSeen = false;
	TEnumAsByte<ECollisionResponse> SavedStaticResponse = ECR_Block;
	FTimerHandle FreeFallTimerHandle;

	static constexpr float PollInterval = 0.03f;
	static constexpr float IgnoreInitialTime = 0.3f;
};
