#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_Footstep.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API UAnimNotify_Footstep : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep", meta = (AllowPrivateAccess = "true"))
	FName FootSocketName;
};