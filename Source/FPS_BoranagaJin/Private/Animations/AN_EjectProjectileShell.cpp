


#include "Animations/AN_EjectProjectileShell.h"
#include "Items/Weapons/WeaponInterface.h"
#include "Items/Weapons/WeaponSystemComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Characters/Player/CharacterPlayer.h"

namespace
{
	void TryCallEjectProjectileShell(UObject* Object)
	{
		if (!Object) return;
		IWeaponInterface* WeaponInterface = Cast<IWeaponInterface>(Object);
		if (WeaponInterface) { WeaponInterface->EjectProjectileShell(); }
	}
}

void UAN_EjectProjectileShell::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	TryCallEjectProjectileShell(Owner);

	ACharacterPlayer* Player = Cast<ACharacterPlayer>(Owner);
	if (!Player) return;

	UWeaponSystemComponent* WeaponSystem = Player->GetWeaponSystemComponent();
	if (!WeaponSystem) return;

	AActor* CurrentWeapon = WeaponSystem->GetCurrentWeapon();
	TryCallEjectProjectileShell(CurrentWeapon);
}