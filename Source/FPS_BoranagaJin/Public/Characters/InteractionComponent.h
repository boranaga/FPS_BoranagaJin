#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class ACharacterPlayer;
class APlayerController;
class UCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnInteractionTargetChanged,
	AActor*,
	NewTarget
);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason
	) override;

public:
	/**
	 * 상호작용 키 입력 시 호출합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	/**
	 * 현재 상호작용 가능한 대상을 다시 탐색합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UpdateInteractionTarget();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetCurrentInteractionTarget() const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool HasInteractionTarget() const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetCurrentInteractionText() const;

protected:
	bool PerformInteractionTrace(FHitResult& OutHitResult) const;

	void SetInteractionTarget(AActor* NewTarget);
	void ClearInteractionTarget();

	bool IsActorInteractable(AActor* TargetActor) const;
	bool CanInteractWithActor(AActor* TargetActor) const;

	FVector GetTraceStart() const;
	FVector GetTraceDirection() const;

protected:
	/**
	 * 플레이어 카메라 또는 시점에서 전방으로 검사할 거리입니다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Interaction|Trace",
		meta = (ClampMin = "0.0", Units = "cm")
	)
	float InteractionDistance = 250.f;

	/**
	 * 상호작용 대상 확인 주기입니다.
	 *
	 * 0.05초면 초당 20회 검사합니다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Interaction|Trace",
		meta = (ClampMin = "0.01", Units = "s")
	)
	float TraceInterval = 1.f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Interaction|Trace"
	)
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel =
		ECC_Visibility;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Interaction|Trace"
	)
	bool bTraceComplex = false;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Interaction|Debug"
	)
	bool bDrawDebugTrace = false;

	UPROPERTY(
		BlueprintAssignable,
		Category = "Interaction"
	)
	FOnInteractionTargetChanged OnInteractionTargetChanged;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Interaction"
	)
	TObjectPtr<AActor> CurrentInteractionTarget;

private:
	UPROPERTY()
	TObjectPtr<ACharacterPlayer> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<APlayerController> OwnerPlayerController;

	FTimerHandle InteractionTraceTimerHandle;
};