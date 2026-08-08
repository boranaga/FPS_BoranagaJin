#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SaveCheckpoint.generated.h"

class ACharacterPlayer;
class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UParticleSystem;
class UNiagaraSystem;
class USoundBase;

UCLASS()
class FPS_BORANAGAJIN_API ACheckpoint : public AActor
{
	GENERATED_BODY()
public:
	ACheckpoint();
protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditImport() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	UFUNCTION()
	void HandleCheckpointOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

protected:
	void ActivateCheckpoint(ACharacterPlayer* Player);
	void PlayActivationEffects();
	void ApplyActivatedVisualState();
	void ValidateCheckpointID();
public:
	FName GetCheckpointID() const { return CheckpointID; }
	FTransform GetRespawnTransform() const;
	bool IsActivated() const { return bActivated; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,Category = "Checkpoint")
	TObjectPtr<UBoxComponent> TriggerBox;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	TObjectPtr<USceneComponent> RespawnPoint;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	TObjectPtr<UStaticMeshComponent> CheckpointMesh;

protected:
	//TODO: 지금은 ID를 Editor에서 자동생성한 것을 사용하는 방식은데,
	//고유 ID를 커스텀 지정해서 사용하는 것이 좋을 듯
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Checkpoint|Save")
	FName CheckpointID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint|Save")
	bool bAutoSaveOnActivation = true;

	/**
	 * 이미 활성화된 체크포인트를 다시 밟았을 때
	 * 저장을 다시 수행할지 결정합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint|Save")
	bool bAllowRepeatedSave = false;

	/**
	 * Trigger를 한 번만 활성화할지 결정합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	bool bActivateOnlyOnce = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Checkpoint")
	bool bActivated = false;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint|Effects")
	TObjectPtr<UNiagaraSystem> ActivationEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint|Effects")
	TObjectPtr<USoundBase> ActivationSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint|Effects")
	FVector EffectLocationOffset = FVector::ZeroVector;

	/**
	 * 활성화 후 Mesh에 적용할 Material입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint|Effects")
	TObjectPtr<UMaterialInterface> ActivatedMaterial;
};