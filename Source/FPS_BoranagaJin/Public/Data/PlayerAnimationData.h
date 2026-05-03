

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PlayerAnimationData.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FPlayerAnimationData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringDamper")
	FVector DamperScale = { 0.01f, 0.01f, 0.03f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringDamper")
	FVector Stiffness = { 100.f, 100.f, 100.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringDamper")
	FVector Damping = { 40.f, 40.f, 30.f };


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringDamper")
	FVector DampingRatio = { 1.f, 1.f, 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringDamper")
	FVector HalfLife = { 1.f, 1.f, 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringDamper")
	bool bUseDampingRatio = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringDamper")
	bool bUseMLS = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringDamper")
	int32 MLSBufferSize = 10;
	//--------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FRotator RotationWhenCrouching_Hand_R = FRotator();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FRotator RotationWhenCrouching_LowerArm_R = FRotator();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FRotator RotationWhenCrouching_UpperArm_R = FRotator();

	//--------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillWeaponSocket")
	float ActiveRotationInterpSpeed_SkillWeapon = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillWeaponSocket")
	float ActiveLocationInterpSpeed_SkillWeapon = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillWeaponSocket")
	float InctiveRotationInterpSpeed_SkillWeapon = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillWeaponSocket")
	float InctiveLocationInterpSpeed_SkillWeapon = 5.f;
};