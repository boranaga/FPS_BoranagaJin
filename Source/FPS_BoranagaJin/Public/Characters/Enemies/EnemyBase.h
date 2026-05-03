

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/TimelineComponent.h"

#include "Characters/DamageInterface.h"
#include "Characters/DamageParams.h"
#include "EnemyActions.h"
//#include "Utilities/EnemyPatrolRoute.h"

#include "EnemyBase.generated.h"

class AEnemyBaseAIController;
class AEnemyWeapon;

class UNiagaraComponent;
class UACEnemyDamageSystem;
class UWidgetComponent;
class UMotionWarpingComponent;

class UBehaviorTree;

UCLASS()
class FPS_BORANAGAJIN_API AEnemyBase : public ACharacter, public IDamageInterface, public IEnemyActions
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AEnemyBaseAIController> AIController;

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Actor Components", meta = (AllowPrivateAccess = "true"))
	//TObjectPtr<UACEnemyDamageSystem> DamageSystemComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Widgets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	FVector2D HealthBarWidgetSize;

	FTimerHandle HideHealthBarHandle;
	FOnMontageEnded OnHitMontageEnded;

	bool bIsDeathEventBound = false;

protected:
	// [protected variables]
	FName EnemyType; // for initializing differently btw enemy types from the DT

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBehaviorTree> BehaviorTree;

	/*UPROPERTY()
	AEnemyWeapon* EnemyWeapon;*/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Niagara")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionWarping", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Patrol Route", meta = (AllowPrivateAccess = "true"))
	//TObjectPtr<AEnemyPatrolRoute> PatrolRoute;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UTimelineComponent> HitColorTimeline;
	FOnTimelineFloat OnHitColorTimelineFloat;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UTimelineComponent> DissolveTimeline;
	FOnTimelineFloat OnDissolveColorTimelineFloat;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCurveFloat> HitColorCurve;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCurveFloat> DissolveColorCurve;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> DeathSound;

	float AttackDamageAmount = 0.f;
	float AttackRate = 0.f;
	float MeleeAttackRange = 0.f;
	float MeleeAttackSphereRadius = 0.f;

	float MaxWalkSpeed = 0.f;
	float MinWalkSpeedVariation = 0.f;
	float MaxWalkSpeedVariation = 0.f;
	float MinAttackRateVariation = 0.f;
	float MaxAttackRateVariation = 0.f;

	bool bIsLevelSequenceSpawned = false;

	bool bCanInflictDamage = true;

	// [protected functions]
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnDamagedTriggered();

	virtual void OnDeathTriggered();

	virtual void OnHitEnded(UAnimMontage* AnimMontage, bool bInterrupted);

	virtual void UpdateHealthBarValue();

	virtual UAnimMontage* GetRandomAnimationMontage(TArray<UAnimMontage*> AnimMontages);

	UFUNCTION()
	void UpdateHitColor(float Alpha);

	UFUNCTION()
	void UpdateDissolveColor(float Alpha);

public:
	AEnemyBase();

	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (RowType = "/Script/SuraS.EnemyAttributesData"))
	FDataTableRowHandle EnemyAttributesDT;

	// ai controller getter
	FORCEINLINE AEnemyBaseAIController* GetAIController() const { return AIController; }

	// player controller getter
	FORCEINLINE APlayerController* GetPlayerController() const { return PlayerController; }

	// widget size getter
	FORCEINLINE FVector2D GetHealthBarWidgetSize() const { return HealthBarWidgetSize; }

	// damage system comp getter
	//FORCEINLINE UACEnemyDamageSystem* GetDamageSystemComp() const { return DamageSystemComp; }

	// niagara comp
	UNiagaraComponent* GetNiagaraComponent() const { return NiagaraComponent; }

	// motion warp comp
	UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

	// other getters
	FORCEINLINE float GetAttackDamageAmount() const { return AttackDamageAmount; }
	FORCEINLINE float GetMeleeAttackRange() const { return MeleeAttackRange; }
	FORCEINLINE float GetMeleeAttackSphereRadius() const { return MeleeAttackSphereRadius; }
	FORCEINLINE float GetMaxWalkSpeed() const { return MaxWalkSpeed; }
	FORCEINLINE float GetMinWalkSpeedVariation() const { return MinWalkSpeedVariation; }
	FORCEINLINE float GetMaxWalkSpeedVariation() const { return MaxWalkSpeedVariation; }
	FORCEINLINE float GetAttackRate() const { return AttackRate; }
	FORCEINLINE float GetMinAttackRateVariation() const { return MinAttackRateVariation; }
	FORCEINLINE float GetMaxAttackRateVariation() const { return MaxAttackRateVariation; }

	// for ai controller turn off/on
	UFUNCTION(BlueprintCallable)
	void TurnOffAIController(bool bIsLevelSequenceSpawned);

	UFUNCTION(BlueprintCallable)
	void TurnOnAIController();

	FORCEINLINE bool CanInflictDamage() const { return bCanInflictDamage; }
	void SetCanInflictDamage(bool val) { bCanInflictDamage = val; };

	// behavior tree getter
	FORCEINLINE UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

	FORCEINLINE FName GetEnemyType() const { return EnemyType; }

	//FORCEINLINE AEnemyPatrolRoute* GetPatrolRoute() const { return PatrolRoute; }

	void SetUpAIController(AEnemyBaseAIController* const NewAIController); // const ptr: the ptr address can't be changed

	virtual bool TakeDamage(const FDamageParams& DamageData, AActor* DamageCauser) override;

	virtual void Attack(ACharacterPlayer* Player) override;

	//virtual void SetMovementSpeed(EEnemySpeed Speed) override;

	//virtual void JumpWall(const FVector& Destination) override;

	virtual void Climb(const FVector& Destination);

	virtual void OnClimbEnded(UAnimMontage* AnimMontage, bool bInterrupted);

	virtual void LungeToTarget(float LungeForce);

	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

	UAnimMontage* ChooseRandomAttackMontage();

	UAnimMontage* GetClimbMontage() const { return ClimbAnimation; };

	UAnimMontage* GetCoopAttackMontage() const { return ThrowAnimation; };

	UAnimMontage* GetFallingMontage() const { return FallingAnimation; };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	TArray<UAnimMontage*> HitAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	TArray<UAnimMontage*> DeathAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	TArray<UAnimMontage*> AttackAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	UAnimMontage* ClimbAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	UAnimMontage* ThrowAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	UAnimMontage* FallingAnimation = nullptr;

	//poolsystem
	bool isInitialized = false;
	void InitializeEnemy();

	//killLog Binding
	void BindKillLogOnDeath();

	//classShift
	virtual void SetCrippled() { IsCrippled = false; }
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	bool IsCrippled = false;
};

