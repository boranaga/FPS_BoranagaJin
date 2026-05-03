


#include "Instance/DefaultGameInstance.h"

//#include "Instance/CheckpointSubsystem.h"
#include "Kismet/GameplayStatics.h"
//#include "SaveGame/SettingSaveGame.h"
//#include "SaveGame/CustumSaveGame.h"

UCustomGameInstance::UCustomGameInstance()
{
	SettingsSaveSlotName = TEXT("SettingsSaveSlot");
	MouseSensitivity = 0.7f; // default value for game instance;
}

void UCustomGameInstance::Init()
{
	Super::Init();
	LoadSettings();
}

void UCustomGameInstance::Shutdown()
{
	SaveSettings();

	Super::Shutdown();
}

// --- 옵션 UI에서 호출할 공용 함수 ---

void UCustomGameInstance::ApplyAndSaveSettings()
{
	SaveSettings();
}


void UCustomGameInstance::SaveSettings()
{
	//USettingSaveGame* SaveGameInstance = Cast<USettingSaveGame>(UGameplayStatics::CreateSaveGameObject(USettingSaveGame::StaticClass()));
	//if (SaveGameInstance)
	//{
	//	// 현재 gameinstance에 저장된 값을 savegame 객체에 복사
	//	SaveGameInstance->SavedMouseSensitivity = this->MouseSensitivity;

	//	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SettingsSaveSlotName, 0);
	//}
}

void UCustomGameInstance::LoadSettings()
{
	//USettingSaveGame* LoadGameInstance = nullptr;

	//if (UGameplayStatics::DoesSaveGameExist(SettingsSaveSlotName, 0))
	//{
	//	LoadGameInstance = Cast<USettingSaveGame>(UGameplayStatics::LoadGameFromSlot(SettingsSaveSlotName, 0));
	//}

	//if (LoadGameInstance)
	//{
	//	MouseSensitivity = LoadGameInstance->SavedMouseSensitivity;
	//}
	//else
	//{
	//	SaveSettings();
	//}
}
