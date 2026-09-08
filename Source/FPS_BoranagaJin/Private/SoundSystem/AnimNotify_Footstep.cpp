#include "SoundSystem/AnimNotify_Footstep.h"

#include "SoundSystem/FootstepComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"


void UAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();

	if (!IsValid(Owner))
	{
		return;
	}

	UFootstepComponent* FootstepComponent = Owner->FindComponentByClass<UFootstepComponent>();

	if (!IsValid(FootstepComponent))
	{
		return;
	}

	FootstepComponent->PlayFootstep(FootSocketName);
}