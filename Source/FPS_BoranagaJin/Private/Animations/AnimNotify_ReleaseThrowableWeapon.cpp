#include "Animations/AnimNotify_ReleaseThrowableWeapon.h"
#include "Characters/Player/CharacterPlayer.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_ReleaseThrowableWeapon::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	ACharacterPlayer* CharacterPlayer = Cast<ACharacterPlayer>(MeshComp->GetOwner());
	if (!IsValid(CharacterPlayer)) { return; }

	CharacterPlayer->NotifyThrowableWeaponRelease();
}

FString UAnimNotify_ReleaseThrowableWeapon::GetNotifyName_Implementation() const
{
	return TEXT("Release Throwable Weapon");
}