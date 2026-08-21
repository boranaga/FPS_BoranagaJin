#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveSystem/SaveGameCustom.h"
#include "SaveSystem/SaveSlotInfo.h"
#include "SaveGameSubsystem.generated.h"

class UFPSGameSave;
class USaveSlotIndexSaveGame;
class ACharacterPlayer;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSaveGameCompleted, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadGameCompleted, bool);
DECLARE_MULTICAST_DELEGATE(FOnSaveGameApplied);

UCLASS()
class FPS_BORANAGAJIN_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
public:
	bool SaveGameAsync();
	bool SaveGameSync();

	//bool LoadGame();
	//bool LoadGame_Upgrade();
	bool LoadGameFromSlot(const FString& SlotName);

	bool OpenSavedLevel();
	bool ApplyLoadedGame();

	//bool DoesSaveExist() const;
	//bool DeleteSave();
	bool DoesSaveExist(const FString& SlotName) const;
	bool DeleteSave(const FString& SlotName);


	const TArray<FSaveSlotInfo>& GetSaveSlots() const;
	const FString& GetCurrentSaveSlotName() const { return CurrSaveSlotName; }


	//void StartNewGame(bool bDeleteSaveFile);
	bool StartNewGame();

	void SetCurrentCheckpoint(FName NewCheckpointID, const FTransform& RespawnTransform);
	void SetCompletedTutorialIDs(const TSet<FName>& TutorialIDs);
	void AddCompletedTutorialID(FName TutorialID);
public:
	bool HasCurrentSaveSlot() const { return !CurrSaveSlotName.IsEmpty(); }
	bool IsSaving() const { return bIsSaving; }
	bool HasLoadedSave() const { return IsValid(LoadedSaveGame); }
	bool HasPendingLoad() const { return bPendingApplyLoadedGame; }
	bool HasValidCheckpoint() const { return CurrentCheckpointData.IsValid(); }
	UFPSGameSave* GetLoadedSaveGame() const { return LoadedSaveGame; }

	FName GetSavedLevelName() const;

	//FName GetCurrentCheckpointID() const;
	const FCheckpointSaveData& GetCurrentCheckpointData() const
	{
		return CurrentCheckpointData;
	}
	const TSet<FName>& GetCompletedTutorialIDs() const;
public:
	FOnSaveGameCompleted OnSaveGameCompleted;
	FOnLoadGameCompleted OnLoadGameCompleted;
	FOnSaveGameApplied OnSaveGameApplied;
private:
	UFPSGameSave* CreateSaveGameObject() const;

	FString GenerateSaveSlotName() const;

	void LoadSlotIndex();
	bool SaveSlotIndex();
	void UpdateCurrentSlotInfo(const UFPSGameSave& SaveObject);
	void RemoveInvalidSlotEntries();

	ACharacterPlayer* FindPlayerCharacter() const;

	void CaptureCurrentGameState(UFPSGameSave& SaveObject);
	void CapturePlayerData(UFPSGameSave& SaveObject);
	void CaptureWorldActorData(UFPSGameSave& SaveObject);
	void CaptureWorldActorData_Upgrade(UFPSGameSave& SaveObject);

	void RestorePlayerData(const UFPSGameSave& SaveObject);
	void RestoreRuntimeSpawnedWorldActorData(const UFPSGameSave& SaveObject);
	void RestoreWorldActorData(const UFPSGameSave& SaveObject);

	void HandleAsyncSaveCompleted(const FString& SlotName, const int32 UserIndex, bool bSuccess);

	FName GetCurrentLevelName() const;

	bool ValidateLoadedSave(const UFPSGameSave* SaveObject) const;

private:
	UPROPERTY()
	FCheckpointSaveData CurrentCheckpointData;

	UPROPERTY()
	FString CurrSaveSlotName = TEXT("MainSave");

	UPROPERTY()
	FString SlotIndexSaveName = TEXT("SaveSlotIndex");

	UPROPERTY()
	TObjectPtr<USaveSlotIndexSaveGame> SlotIndex;

	UPROPERTY(Transient)
	TObjectPtr<UFPSGameSave> LoadedSaveGame;

	UPROPERTY(Transient)
	TObjectPtr<UFPSGameSave> SaveGameBeingWritten;

	//UPROPERTY()
	//FString SaveSlotName_Main = TEXT("MainSave");
	//UPROPERTY()
	//FString SaveSlotName_1 = TEXT("SaveSlot_1");
	//UPROPERTY()
	//FString SaveSlotName_2 = TEXT("SaveSlot_2");
	//UPROPERTY()
	//FString SaveSlotName_3 = TEXT("SaveSlot_3");

	UPROPERTY()
	int32 SaveUserIndex = 0;

	UPROPERTY()
	int32 CurrentSaveVersion = 1;

	UPROPERTY()
	bool bIsSaving = false;

	UPROPERTY()
	bool bPendingApplyLoadedGame = false;

	UPROPERTY()
	TSet<FName> CompletedTutorialIDs;
};