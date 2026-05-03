

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Weapon.h"
#include "WeaponTargeting.generated.h"

class UWeaponTargetingState;
class UTargetingSkillWidget;
class UTargetMarkerWidget;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRocketLauncherSkillActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRocketLauncherSkillOvered);

UCLASS()
class FPS_BORANAGAJIN_API AWeaponTargeting : public AWeapon
{
	GENERATED_BODY()
public:
	AWeaponTargeting();
protected:
	virtual void BeginPlay() override;
public:
	virtual void Tick(float DeltaTime) override;

	virtual void InitWeapon(ACharacterPlayer* NewCharacter) override;

	virtual void InitializeUI() override;

	virtual void LoadWeaponData_Upgrade() override;

	virtual void SetInputActionBinding() override;

	///UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool AttachWeaponToPlayer(ACharacterPlayer* TargetCharacter) override;

	virtual void FireSingleProjectile(FWeaponFireParams* FireData = nullptr, int32 NumPenetrable = 0, float AdditionalDamage = 0.f, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f, float AdditionalProjectileRadius = 0.f, bool bIsHoming = false, AActor* HomingTarget = nullptr) override;

	UPROPERTY(VisibleAnywhere)
	UWeaponTargetingState* TargetingState = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* TargetSearchLoopSound = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* TargetLockedSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "TargetMarkerWidget")
	TSubclassOf<UTargetMarkerWidget> TargetMarkerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "UTargetingSkillWidget")
	TSubclassOf<UTargetingSkillWidget> TargetingSkillWidgetClass;
	
	UPROPERTY()
	UTargetingSkillWidget* TargetingSkillWidget;


public:
	void ActivateTargetingSkillWidget(bool bflag);

	UFUNCTION(BlueprintCallable)
	UTargetingSkillWidget* GetTargetingSkillWidget() const { return TargetingSkillWidget; }

	UPROPERTY(BlueprintAssignable, Category = "Skill")
	FOnRocketLauncherSkillActivated OnRocketLauncherSkillActivated;
	UPROPERTY(BlueprintAssignable, Category = "Skills")
	FOnRocketLauncherSkillOvered OnRocketLauncherSkillOvered;

protected:
	FTransform RightHandSocketTransform_Targeting; //TODO: Targeting Weapon으로 옮겨야 함
	FTransform RightHandSocketTransform_Targeting_Crouch;
public:
	FTransform GetRightHandSocketTransform_Targeting() const { return RightHandSocketTransform_Targeting; }
	FTransform GetRightHandSocketTransform_Targeting_Crouch() const { return RightHandSocketTransform_Targeting_Crouch; }

#pragma region FireMode/Targeting
protected:
	UPROPERTY()
	TArray<UTargetMarkerWidget*> TargetMarkerWidgets;
	UPROPERTY()
	TSet<AActor*> Targets;
	UPROPERTY()
	TMap<AActor*, UTargetMarkerWidget*> MapTargetActorToWidget;

	FTimerHandle TargetDetectionTimer;
	int32 MaxTargetNum = 10;

	float MaxTargetDetectionRadius = 3000.f;
	float MaxTargetDetectionAngle = 80.f;

	float CurrentTargetDetectionRadius = 0.f;
	float CurrentTargetDetectionAngle = 0.f;

	float MaxTargetDetectionTime = 8.f;
	float TimeToReachMaxTargetDetectionRange = 2.5f;
	float ElapsedTimeAfterTargetingStarted = 0.f;
protected:
	void StartTargetDetection();
	void UpdateTargetDetection(float DeltaTime);
	void StopTargetDetection(FWeaponFireParams* FireData = nullptr);

	bool SearchOverlappedActor(FVector CenterLocation, float SearchRadius, TArray<AActor*>& OverlappedActors);
	TTuple<FVector2D, bool> GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const;
	bool IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width = 0.0f, float ScreenRatio_Height = 0.0f) const;
	float GetUnsignedAngleBetweenVectors(const FVector& VectorA, const FVector& VectorB, const FVector& Axis);
	bool CheckIfTargetIsBlockedByObstacle(AActor* target);

	UTargetMarkerWidget* CreateTargetMarkerWidget(AActor* TargetActor);
public:
	void UpdateTargetMarkers();
	void ResetTargetMarkers();
protected:
	UPROPERTY()
	TArray<AActor*> ConfirmedTargets;
	int32 CurrentTargetIndex = 0;
	float MissileLaunchDelay = 0.2;
	FTimerHandle MissileLaunchTimer;
protected:
	void StartMissileLaunch(TArray<AActor*> TargetActors, FWeaponFireParams* FireData = nullptr);
	void UpdateMissileLaunch(FWeaponFireParams* FireData = nullptr);
	void StopMissileLaunch();
#pragma endregion

#pragma region Skill/Targeting
protected:
	//bool bIsSkillWeapon = false;
	bool bAllowNormalFireForSkillWeapon = false;
public:
	//bool IsSkillWeapon() const { return bIsSkillWeapon; }
protected:
	bool bCanUseTargetingSkill = true;
	float TargetingSkillCoolDown = 3.f;
	float MaxTargetingTime = 10.f;
	FTimerHandle TargetingSkillTimer;
	void HandleTargetDetectionSkill();
	void UpdateTargetDetectionSkill(float DeltaTime);
	void HandleTargetingSkillFire(bool bIsLeftInput = true, bool bSingleProjectile = true, int32 NumPenetrable = 0);
	void CancelTargetingSkill();
	void EnableTargetingSkill(bool bflag);
	void UpdateTargetingSkillUI();
	float TargetGlobalTimeScale = 1.f;
	float TargetingGlobalTimeDilationSpeed_In = 1.f;
	float TargetingGlobalTimeDilationSpeed_Out = 1.f;
	bool bIsGlobalTimeScaleChanging = false;

	bool TryTakeControl();
	void ReleaseControl();
public:
	float TargetingGlobalTimeScale = 1.f;
	void SetGlobalTimeDilation(float targettimescale = 1.f);
	void UpdateGlobalTimeDiation(float DeltaTime);
#pragma endregion
};
