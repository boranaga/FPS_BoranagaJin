#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveSystem/SaveGameCustom.h"
#include "SaveGameSubsystem.generated.h"

class UFPSGameSave;
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

	/**
	 * 파일을 동기식으로 읽습니다.
	 *
	 * 로드 버튼처럼 곧바로 레벨 이름을 알아야 하는 경우에 사용합니다.
	 * 실제 상태 적용은 레벨 이동 후 ApplyLoadedGame()에서 수행합니다.
	 */
	bool LoadGame();

	/**
	 * 로드된 세이브의 레벨로 이동합니다.
	 */
	bool OpenSavedLevel();

	/**
	 * 현재 월드와 플레이어에 LoadedSaveGame 데이터를 적용합니다.
	 */
	bool ApplyLoadedGame();

	bool DoesSaveExist() const;

	bool DeleteSave();

	/**
	 * 새 게임 시작 시 기존 메모리 데이터를 초기화합니다.
	 *
	 * bDeleteSaveFile이 true이면 디스크의 세이브도 삭제합니다.
	 */
	void StartNewGame(bool bDeleteSaveFile);

	void SetCurrentCheckpoint(FName NewCheckpointID, const FTransform& RespawnTransform);

	void SetCompletedTutorialIDs(const TSet<FName>& TutorialIDs);

	void AddCompletedTutorialID(FName TutorialID);

public:
	bool IsSaving() const
	{
		return bIsSaving;
	}

	bool HasLoadedSave() const
	{
		return IsValid(LoadedSaveGame);
	}

	bool HasPendingLoad() const
	{
		return bPendingApplyLoadedGame;
	}

	bool HasValidCheckpoint() const
	{
		return CurrentCheckpointData.IsValid();
	}

	UFPSGameSave* GetLoadedSaveGame() const
	{
		return LoadedSaveGame;
	}

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

	ACharacterPlayer* FindPlayerCharacter() const;

	void CaptureCurrentGameState(UFPSGameSave& SaveObject);
	void CapturePlayerData(UFPSGameSave& SaveObject);
	void CaptureWorldActorData(UFPSGameSave& SaveObject);

	void RestorePlayerData(
		const UFPSGameSave& SaveObject);

	void RestoreWorldActorData(
		const UFPSGameSave& SaveObject);

	void HandleAsyncSaveCompleted(
		const FString& SlotName,
		const int32 UserIndex,
		bool bSuccess);

	FName GetCurrentLevelName() const;

	bool ValidateLoadedSave(
		const UFPSGameSave* SaveObject) const;

private:
	UPROPERTY()
	FCheckpointSaveData CurrentCheckpointData;

	UPROPERTY(Transient)
	TObjectPtr<UFPSGameSave> LoadedSaveGame;

	UPROPERTY(Transient)
	TObjectPtr<UFPSGameSave> SaveGameBeingWritten;

	UPROPERTY()
	FString SaveSlotName = TEXT("MainSave");

	UPROPERTY()
	int32 SaveUserIndex = 0;

	UPROPERTY()
	int32 CurrentSaveVersion = 1;

	UPROPERTY()
	bool bIsSaving = false;

	UPROPERTY()
	bool bPendingApplyLoadedGame = false;

	//UPROPERTY()
	//FName CurrentCheckpointID = NAME_None;

	UPROPERTY()
	TSet<FName> CompletedTutorialIDs;
};