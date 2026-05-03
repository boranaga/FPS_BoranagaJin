

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Characters/Player/PlayerMovementComponent.h"
#include "Items/WeaponState/WeaponStateType.h"
#include "Items/Weapons/WeaponInterface.h"
#include "Items/Weapons/ArmRecoilParams.h"
#include "Data/PlayerAnimationData.h"
#include "PlayerCharacterAnimInstance.generated.h"

class ACharacterPlayer;
class AWeapon;

// Ring buffer for float with O(1) moving average

struct FPS_BORANAGAJIN_API FRingBufferFloat
{
public:
	int32   BufferSize = 0;
	TArray<float> Buffer;
	int32   Head = 0;     // write index
	int32   Count = 0;    // number of valid samples (<= BufferSize)
	double  Sum = 0.0;    // use double to reduce accumulation error

	FRingBufferFloat() = default;

	explicit FRingBufferFloat(int32 InSize)
	{
		Init(InSize);
	}

	void Init(int32 InSize)
	{
		check(InSize > 0);
		BufferSize = InSize;
		Buffer.SetNumZeroed(BufferSize); // or Buffer.Init(0.f, BufferSize);
		Head = 0;
		Count = 0;
		Sum = 0.0;
	}

	FORCEINLINE void Reset()
	{
		if (BufferSize > 0)
		{
			FMemory::Memzero(Buffer.GetData(), BufferSize * sizeof(float));
		}
		Head = 0;
		Count = 0;
		Sum = 0.0;
	}

	FORCEINLINE void Push(float NewValue, bool bMoveHead)
	{
		//// Remove the value being overwritten when full
		//if (Count == BufferSize)
		//{
		//	Sum -= Buffer[Head];
		//}
		//else
		//{
		//	++Count;
		//}

		//Buffer[Head] = NewValue;
		//Sum += NewValue;

		//// Wrap
		//Head = (Head + 1);
		//if (Head >= BufferSize) Head = 0;


		//----------------
		Buffer[Head] = NewValue;

		if (bMoveHead)
		{
			Head = (Head + 1) % BufferSize;
		}
	}

	FORCEINLINE float GetAverage() const
	{
		return (Count > 0) ? static_cast<float>(Sum / static_cast<double>(Count)) : 0.0f;
	}

	FORCEINLINE float GetApproximation() const
	{
		float a = 0;
		float b = 0;

		float sum_xy = 0;
		float sum_x = 0;
		float sum_y = 0;
		float sum_xx = 0;

		for (int32 i = 0; i < BufferSize; i++)
		{
			sum_xy += (i * Buffer[(Head + i) % BufferSize]);
			sum_x += i;
			sum_y += Buffer[(Head + i) % BufferSize];
			sum_xx += i * i;
		}

		a = ((BufferSize)*sum_xy - sum_x * sum_y) / ((BufferSize)*sum_xx - sum_x * sum_x);
		b = sum_y / (BufferSize)-(sum_x / (BufferSize)) * a;


		return a * (BufferSize - 1) + b;
	}



	FORCEINLINE bool IsFull() const { return Count == BufferSize; }
	FORCEINLINE int32 Num()   const { return Count; }
};

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UPlayerCharacterAnimInstance : public UAnimInstance, public IWeaponInterface
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

protected:
	// <Player Movement>
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<ACharacterPlayer> PlayerCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EMovementState MovementState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsRunning = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsCrouching = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsDashing = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D MovementInputVector;

	//-------------------------------------------------
	// <Weapon system>

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Weapon)
	FDataTableRowHandle AnimationDataTableHandle;
	FPlayerAnimationData* AnimationData;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	EWeaponStateType CurrentWeaponStateType;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bIsZoomIn = false;

	//TODO: ������ ��. �������
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FVector ScreenCenterWorldLocation;

	//TODO: ������ ��. �������
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FVector TargetRightHandWorldLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FTransform AimSocketRelativeTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FTransform AimPointRelativeTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FTransform LeftHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FTransform TargetLeftHandSocketTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FVector AimPointLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FRotator AimPointRotation;

	//TODO: Logic �ٲ�� ��
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	AWeapon* CurrentWeapon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	AWeapon* CurrentSkillWeapon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bHasWeapon = false;

	//--------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FVector AimPointRelativeLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FRotator AimPointRelativeRotation;

	//---------------------------------------

	//UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	//FTransform DefalutCameraRelativeTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FVector HandTargetRelativeLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FRotator HandTargetRelativeRotation;

public:
	void LoadAnimationData();

	void UpdateWeapon();

	void SetAimSocket();

	void SetAimPoint();

	//---------------------------

	void SetTargetRightHandTransform();

	void LogTransform(const FTransform& Transform, const FString& TransformName = TEXT("Transform"));

	//---------------------------
	FTransform GetLeftHandTransform();
	FTransform GetTargetLeftHandTransfrom();

#pragma region RightHandSocket
protected:
	UPROPERTY(BlueprintReadOnly, Category = "RightHandSocket")
	FTransform RightHandSocketTransform;
	UPROPERTY(BlueprintReadOnly, Category = "RightHandSocket")
	FTransform RightHandSocketTransform_Crouch;
	UPROPERTY(BlueprintReadOnly, Category = "RightHandSocket")
	FTransform RightHandSocketTransform_Targeting;
	UPROPERTY(BlueprintReadOnly, Category = "RightHandSocket")
	FTransform RightHandSocketTransform_Targeting_Crouch;

	UPROPERTY(BlueprintReadOnly, Category = "RightHandSocket")
	FTransform CurrentRightHandSocketTransform;
public:
	void UpdateRightHandSocket(float DeltaTime);
#pragma endregion

#pragma region ArmRecoil
public:
	virtual void AddArmRecoil(FArmRecoilParams* armrecoil = nullptr, float AdditionalRecoilAmountX = 0.f, float AdditionalRecoilAmountY = 0.f, float AdditionalRecoilAmountZ = 0.f) override;
protected:
	bool bIsArmRecoiling = false;

	// <Old>
	UPROPERTY()
	FArmRecoilParams ArmRecoil_Hand;
	UPROPERTY()
	FArmRecoilParams ArmRecoil_UpperArm;
	UPROPERTY()
	FArmRecoilParams ArmRecoil_LowerArm;

	FRotator TotalTargetRecoil_Hand = { 0.f, 0.f, 0.f };
	FRotator TotalTargetRecoil_LowerArm = { 0.f, 0.f, 0.f };
	FRotator TotalTargetRecoil_UpperArm = { 0.f, 0.f, 0.f };

	FRotator CulmulatedRecoil_Hand = { 0.f, 0.f, 0.f };
	FRotator CulmulatedRecoil_LowerArm = { 0.f, 0.f, 0.f };
	FRotator CulmulatedRecoil_UpperArm = { 0.f, 0.f, 0.f };

	FRotator RecoveredRecoil_Hand = { 0.f, 0.f, 0.f };
	FRotator RecoveredRecoil_LowerArm = { 0.f, 0.f, 0.f };
	FRotator RecoveredRecoil_UpperArm = { 0.f, 0.f, 0.f };

	UPROPERTY(BlueprintReadOnly, Category = "ArmRecoil")
	FRotator CurrentRecoil_Hand = { 0.f, 0.f, 0.f };
	UPROPERTY(BlueprintReadOnly, Category = "ArmRecoil")
	FRotator CurrentRecoil_LowerArm = { 0.f, 0.f, 0.f };
	UPROPERTY(BlueprintReadOnly, Category = "ArmRecoil")
	FRotator CurrentRecoil_UpperArm = { 0.f, 0.f, 0.f };

	//--------------------------
	// <New>
	UPROPERTY()
	FArmRecoilParams ArmRecoil;

	FRotator TotalTargetRecoil_Rot;
	FVector TotalTargetRecoil_Vec;

	UPROPERTY(BlueprintReadOnly, Category = "ArmRecoil")
	FRotator CurrentRecoil_Rot;
	UPROPERTY(BlueprintReadOnly, Category = "ArmRecoil")
	FVector CurrentRecoil_Vec;

	UPROPERTY(BlueprintReadOnly, Category = "ArmRecoil")
	FRotator ConvertedCurrentRecoil_Rot;
	UPROPERTY(BlueprintReadOnly, Category = "ArmRecoil")
	FVector ConvertedCurrentRecoil_Vec;

public:
	void ApplyArmRecoil(float DeltaTime);
	void RecoverArmRecoil(float DeltaTime);
	void UpdateArmRecoil(float DeltaTime);
	void ConvertRecoilValueFrame();
#pragma endregion

#pragma region SkillWeaponRecoil
protected:
	UPROPERTY(BlueprintReadOnly, Category = "SkillWeaponSocket")
	FTransform SkillWeaponSocketTransform_Active;
	UPROPERTY(BlueprintReadOnly, Category = "SkillWeaponSocket")
	FTransform SkillWeaponSocketTransform_Inactive;

	UPROPERTY(BlueprintReadOnly, Category = "SkillWeaponSocket")
	FTransform CurrentSkillWeaponSocketTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillWeaponSocket")
	float ActiveRotationInterpSpeed_SkillWeapon = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillWeaponSocket")
	float ActiveLocationInterpSpeed_SkillWeapon = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillWeaponSocket")
	float InctiveRotationInterpSpeed_SkillWeapon = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillWeaponSocket")
	float InctiveLocationInterpSpeed_SkillWeapon = 5.f;
public:
	void UpdateSkillWeaponSocket(float DeltaTime);
public:
	virtual void AddSkillWeaponRecoil(FArmRecoilParams* armrecoil = nullptr, float AdditionalRecoilAmountX = 0.f, float AdditionalRecoilAmountY = 0.f, float AdditionalRecoilAmountZ = 0.f) override;
	void ApplySkillWeaponRecoil(float DeltaTime);
	void RecoverSkillWeaponRecoil(float DeltaTime);
	void UpdateSkillWeaponRecoil(float DeltaTime);
	void ConvertSkillWeaponRecoilValueFrame();
protected:
	bool bIsSkillWeaponRecoiling = false;

	UPROPERTY()
	FArmRecoilParams SkillWeaponRecoil;

	FRotator TotalTargetSkillWeaponRecoil_Rot;
	FVector TotalTargetSkillWeaponRecoil_Vec;

	UPROPERTY(BlueprintReadOnly, Category = "ArmRecoil")
	FRotator CurrentSkillWeaponRecoil_Rot;
	UPROPERTY(BlueprintReadOnly, Category = "ArmRecoil")
	FVector CurrentSkillWeaponRecoil_Vec;

	UPROPERTY(BlueprintReadOnly, Category = "ArmRecoil")
	FRotator ConvertedSkillWeaponCurrentRecoil_Rot;
	UPROPERTY(BlueprintReadOnly, Category = "ArmRecoil")
	FVector ConvertedSkillWeaponCurrentRecoil_Vec;

#pragma endregion

#pragma region Crouch
protected:
	FRotator RotationWhenCrouching_Hand_R;
	FRotator RotationWhenCrouching_LowerArm_R;
	FRotator RotationWhenCrouching_UpperArm_R;
#pragma endregion

#pragma region Spring Damper
protected:
	FVector CurrentComponentPos;
	FVector CurrentComponentVel;

	FVector PreviousComponentPos; //World Transform

	FTransform RightHandSocketSpringDamperTransform;

protected:
	FVector DamperScale;

	FVector Stiffness;
	FVector Damping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringDamper")
	FVector DampingRatio = { 1.f, 1.f, 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringDamper")
	FVector HalfLife = { 1.f, 1.f, 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringDamper")
	bool bUseDampingRatio = true;

	float fast_negexp(float x);
	float halflife_to_damping(float halflife, float eps = 1e-5f) { return (4.0f * 0.69314718056f) / (halflife + eps); }
	float damping_to_halflife(float damping, float eps = 1e-5f) { return (4.0f * 0.69314718056f) / (damping + eps); }
	float frequency_to_stiffness(float frequency) { return FMath::Square(2.0f * UE_PI * frequency); }
	float stiffness_to_frequency(float stiffness) { return FMath::Sqrt(stiffness) / (2.0f * UE_PI); }
	float critical_halflife(float frequency) { return damping_to_halflife(FMath::Sqrt(frequency_to_stiffness(frequency) * 4.0f)); }
	float critical_frequency(float halflife) { return stiffness_to_frequency(FMath::Square(halflife_to_damping(halflife)) / 4.0f); }
	float damping_ratio_to_stiffness(float ratio, float damping) { return FMath::Square(damping / (ratio * 2.0f)); }
	float damping_ratio_to_damping(float ratio, float stiffness) { return ratio * 2.0f * FMath::Sqrt(stiffness); }
	void SpringDamper(FVector CurrPos, FVector CurrVel, FVector GoalPos, FVector GoalVel, FVector& OutPos, FVector& OutVel, FVector stiffness, FVector damping, float DeltaTime, float eps = 1e-6f);
	void SpringDamper_f(float CurrPos, float CurrVel, float GoalPos, float GoalVel, double& OutPos, double& OutVel, float damping_ratio, float halflife, float DeltaTime, float eps = 1e-8f);
	void UpdateSpringDamper(float DeltaTime);
	void UpdateSpringDamper_Upgrade(float DeltaTime);
	void UpdateSpringDamper_MLS(float DeltaTime);
#pragma endregion

#pragma region Tilt
protected:
	void UpdateRightHandTilt(float DeltaTime);
#pragma endregion

#pragma region MLS
private:
	bool bUseMLS = true;
	int32 MLSBufferSize = 10;
	FRingBufferFloat SpringDamperPosBuffer;
#pragma endregion


};