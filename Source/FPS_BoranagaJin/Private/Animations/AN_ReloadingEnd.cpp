


#include "Animations/AN_ReloadingEnd.h"
#include "Items/Weapons/WeaponInterface.h"
#include "Items/Weapons/WeaponSystemComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Characters/Player/CharacterPlayer.h"

namespace
{
	void TryCallReloadingEnd(UObject* Object)
	{
		if (!Object) return;
		IWeaponInterface* WeaponInterface = Cast<IWeaponInterface>(Object);
		if (WeaponInterface) { WeaponInterface->ReloadingEnd(); }
	}
}

void UAN_ReloadingEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	TryCallReloadingEnd(Owner);

	ACharacterPlayer* Player = Cast<ACharacterPlayer>(Owner);
	if (!Player) return;

	UWeaponSystemComponent* WeaponSystem = Player->GetWeaponSystemComponent();
	if (!WeaponSystem) return;

	AActor* CurrentWeapon = WeaponSystem->GetCurrentWeapon();
	TryCallReloadingEnd(CurrentWeapon);
}