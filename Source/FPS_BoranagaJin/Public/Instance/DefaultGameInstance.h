

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
//#include "Items/Weapons/WeaponName.h"
#include "PlayableMapInfo.h"

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
public:
	const TSoftObjectPtr<UWorld>& GetMainMenuLevel() const
	{
		return MainMenuLevel;
	}
	const TArray<FPlayableMapInfo>& GetPlayableMaps() const { return PlayableMaps; }

	const TSoftObjectPtr<UWorld>& GetFirstGameLevel() const
	{
		return FirstGameLevel;
	}
	const TSoftObjectPtr<UWorld>& GetSecondGameLevel() const
	{
		return SecondGameLevel;
	}

private:
	UPROPERTY(EditDefaultsOnly, Category = "Game Flow|Levels")
	TSoftObjectPtr<UWorld> MainMenuLevel;

	UPROPERTY(EditDefaultsOnly, Category = "Level")
	TArray<FPlayableMapInfo> PlayableMaps;

	UPROPERTY(EditDefaultsOnly, Category = "Game Flow|Levels")
	TSoftObjectPtr<UWorld> FirstGameLevel;
	UPROPERTY(EditDefaultsOnly, Category = "Game Flow|Levels")
	TSoftObjectPtr<UWorld> SecondGameLevel;


//---------------------------------------------

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MouseSensitivity = 0.f;

	//UPROPERTY()
	//TMap<EWeaponName, bool> OwnedWeapons;

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
