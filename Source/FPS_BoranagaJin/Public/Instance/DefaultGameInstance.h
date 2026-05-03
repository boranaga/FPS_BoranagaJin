

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Items/Weapons/WeaponName.h"
#include "DefaultGameInstance.generated.h"

class USettingSaveGame;

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UCustomGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UCustomGameInstance();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MouseSensitivity = 0.f;

	UPROPERTY()
	TMap<EWeaponName, bool> OwnedWeapons;

protected:
	FString WeaponSaveSlotName;
	void SaveWeaponData();
	void LoadWeaponData();

	FString SettingsSaveSlotName;
	void SaveSettings();
	void LoadSettings();

public:
	virtual void Init() override;
	virtual void Shutdown() override;


	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyAndSaveSettings();
};
