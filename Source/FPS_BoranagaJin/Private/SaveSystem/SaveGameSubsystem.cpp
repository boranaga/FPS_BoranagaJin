#include "SaveSystem/SaveGameSubsystem.h"

#include "SaveSystem/SaveGameCustom.h"
#include "SaveSystem/SaveSlotIndexSaveGame.h"
#include "SaveSystem/SaveSlotInfo.h"
#include "Interface/SaveableActorInterface.h"
#include "Interface/SaveablePlayerInterface.h"
#include "SaveSystem/SaveGameArchive.h"
#include "SaveSystem/GameResultLogSaveGame.h"
#include "ObjectPoolSubsystem.h"

#include "Characters/Player/CharacterPlayer.h"
#include "Items/Item.h"
#include "Items/ItemPickUp.h"
#include "Items/InventorySystemComponent.h"
#include "SaveSystem/InventorySaveData.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogSaveGameSubsystem, Log, All);

namespace
{
	void CollectSavedInventoryIDs(const UFPSGameSave& Save, TSet<FGuid>& OutIDs)
	{
		const auto Collect = [&OutIDs](const TArray<FInventorySlotSaveData>& Slots)
			{
				for (const FInventorySlotSaveData& Slot : Slots)
				{
					if (Slot.Item.InstanceID.IsValid()) OutIDs.Add(Slot.Item.InstanceID);
					for (const FItemInstanceSaveData& LegacyItem : Slot.Items)
						if (LegacyItem.InstanceID.IsValid()) OutIDs.Add(LegacyItem.InstanceID);
				}
			};
		Collect(Save.PlayerData.ItemInventorySlots);
		Collect(Save.PlayerData.PrimaryWeaponInventorySlots);
		Collect(Save.PlayerData.SecondaryWeaponInventorySlots);
		Collect(Save.PlayerData.ThrowableWeaponInventorySlots);
	}

	bool ShouldSkipWorldItem(AActor* Actor, const TSet<AItem*>& InventoryItems, const TSet<FGuid>& LegacyLinkedIDs)
	{
		AItem* Item = Cast<AItem>(Actor);
		// Inventory snapshots own held items. Inactive pooled item actors are not world objects.
		// A dropped item is represented by the active AItemPickUp's serialized snapshot.
		return IsValid(Item) && (InventoryItems.Contains(Item) || (!Item->IsActiveInPool() && !LegacyLinkedIDs.Contains(Item->GetInstanceID())));
	}

	void CollectLegacyLinkedIDs(UWorld* World, TSet<FGuid>& OutIDs)
	{
		if (!IsValid(World)) return;
		for (TActorIterator<AItemPickUp> It(World); It; ++It)
		{
			AItemPickUp* Pickup = *It;
			if (IsValid(Pickup) && Pickup->IsActiveInPool() && Pickup->GetLegacyItemInstanceID().IsValid())
				OutIDs.Add(Pickup->GetLegacyItemInstanceID());
		}
	}

	bool ValidateInventorySnapshot(ACharacterPlayer* Player, const UFPSGameSave& Save)
	{
		UInventorySystemComponent* Inventory = IsValid(Player) ? Player->GetInventorySystemComponent() : nullptr;
		if (!IsValid(Inventory)) return false;
		const auto Matches = [Inventory](EInventoryName Type, const TArray<FInventorySlotSaveData>& SavedSlots)
			{
				const TArray<FInventorySlot>* Slots = Inventory->GetInventoryConst(Type);
				if (!Slots || Slots->Num() != SavedSlots.Num()) return false;
				for (int32 Index = 0; Index < Slots->Num(); ++Index)
				{
					const FInventorySlot& Slot = (*Slots)[Index];
					const FItemInstanceSaveData& Saved = SavedSlots[Index].Item;
					if (Slot.IsEmpty()) { if (!SavedSlots[Index].IsEmpty()) return false; continue; }
					if (!Saved.IsValid() || Saved.ItemNum != Slot.GetStackItemNum() ||
						Saved.InstanceID != Slot.GetItem()->GetInstanceID()) return false;
				}
				return true;
			};
		return Matches(EInventoryName::Item, Save.PlayerData.ItemInventorySlots) &&
			Matches(EInventoryName::PrimaryWeapon, Save.PlayerData.PrimaryWeaponInventorySlots) &&
			Matches(EInventoryName::SecondaryWeapon, Save.PlayerData.SecondaryWeaponInventorySlots) &&
			Matches(EInventoryName::ThrowableWeapon, Save.PlayerData.ThrowableWeaponInventorySlots);
	}

	void CollectRuntimeInventoryItems(ACharacterPlayer* Player, TSet<AItem*>& OutItems)
	{
		if (!IsValid(Player)) return;
		UInventorySystemComponent* Inventory = Player->GetInventorySystemComponent();
		if (!IsValid(Inventory)) return;
		for (EInventoryName Type : { EInventoryName::Item, EInventoryName::PrimaryWeapon, EInventoryName::SecondaryWeapon, EInventoryName::ThrowableWeapon })
		{
			if (const TArray<FInventorySlot>* Slots = Inventory->GetInventoryConst(Type))
				for (const FInventorySlot& Slot : *Slots)
					if (AItem* Item = Slot.GetItem()) OutItems.Add(Item);
		}
	}
}

void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadedSaveGame = nullptr;
	SaveGameBeingWritten = nullptr;

	bIsSaving = false;
	bPendingApplyLoadedGame = false;

	CurrSaveSlotName.Empty();

	CurrentCheckpointData.Reset();
	CompletedTutorialIDs.Reset();

	LoadSlotIndex();
	LoadGameResultLog();

	UE_LOG(LogSaveGameSubsystem, Log, TEXT("SaveGameSubsystem initialized."));
}

void USaveGameSubsystem::Deinitialize()
{
	OnSaveGameCompleted.Clear();
	OnLoadGameCompleted.Clear();
	OnSaveGameApplied.Clear();

	LoadedSaveGame = nullptr;
	SaveGameBeingWritten = nullptr;

	Super::Deinitialize();
}

bool USaveGameSubsystem::SaveGameAsync()
{
	if (bIsSaving)
	{
		UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Save request ignored because another save is in progress."));
		return false;
	}

	if (CurrSaveSlotName.IsEmpty())
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Save failed: CurrentSaveSlotName is empty."));
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Save failed: World is invalid."));
		OnSaveGameCompleted.Broadcast(false);
		return false;
	}

	UFPSGameSave* NewSaveGame = CreateSaveGameObject();
	if (!IsValid(NewSaveGame))
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Save failed: Could not create SaveGame object."));
		OnSaveGameCompleted.Broadcast(false);
		return false;
	}

	CaptureCurrentGameState(*NewSaveGame);
	if (!ValidateInventorySnapshot(FindPlayerCharacter(), *NewSaveGame))
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Save aborted: incomplete inventory snapshot."));
		OnSaveGameCompleted.Broadcast(false);
		return false;
	}

	/*
	 * 비동기 저장이 끝날 때까지 GC되지 않도록
	 * UPROPERTY 포인터로 보관합니다.
	 */
	SaveGameBeingWritten = NewSaveGame;
	bIsSaving = true;

	FAsyncSaveGameToSlotDelegate SaveDelegate;
	SaveDelegate.BindUObject(this, &USaveGameSubsystem::HandleAsyncSaveCompleted);

	UGameplayStatics::AsyncSaveGameToSlot(NewSaveGame, CurrSaveSlotName, SaveUserIndex, SaveDelegate);

	UE_LOG(LogSaveGameSubsystem, Log, TEXT("Async save started. Slot: %s"), *CurrSaveSlotName);

	return true;
}

bool USaveGameSubsystem::SaveGameSync()
{
	if (bIsSaving)
	{
		UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Save failed: Async save is already in progress."));
		return false;
	}

	if (CurrSaveSlotName.IsEmpty())
	{
		UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Save failed: No active save slot."));
		return false;
	}

	UFPSGameSave* NewSaveGame = CreateSaveGameObject();
	if (!IsValid(NewSaveGame))
	{
		UE_LOG(LogSaveGameSubsystem, Warning, TEXT("USaveGameSubsystem::SaveGameSync(): No active save slot."));
		return false;
	}

	CaptureCurrentGameState(*NewSaveGame);
	if (!ValidateInventorySnapshot(FindPlayerCharacter(), *NewSaveGame))
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Save aborted: incomplete inventory snapshot."));
		OnSaveGameCompleted.Broadcast(false);
		return false;
	}

	const bool bSuccess = UGameplayStatics::SaveGameToSlot(NewSaveGame, CurrSaveSlotName, SaveUserIndex);

	if (!bSuccess)
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Save failed. Slot: %s"), *CurrSaveSlotName);
		OnSaveGameCompleted.Broadcast(false);
		return false;
	}

	LoadedSaveGame = NewSaveGame;

	UpdateCurrentSlotInfo(*NewSaveGame);
	SaveSlotIndex();

	UE_LOG(LogSaveGameSubsystem, Log, TEXT("Game saved. Slot: %s"), *CurrSaveSlotName);

	OnSaveGameCompleted.Broadcast(true);

	return true;
}

//bool USaveGameSubsystem::LoadGame()
//{
//	if (bIsSaving)
//	{
//		UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Load failed: Save is currently in progress."));
//		OnLoadGameCompleted.Broadcast(false);
//		return false;
//	}
//
//	if (!DoesSaveExist())
//	{
//		UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Load failed: Save slot does not exist. Slot: %s"), *SaveSlotName_Main);
//		OnLoadGameCompleted.Broadcast(false);
//		return false;
//	}
//
//	USaveGame* LoadedObject = UGameplayStatics::LoadGameFromSlot(SaveSlotName_Main, SaveUserIndex);
//	UFPSGameSave* LoadedFPSGame = Cast<UFPSGameSave>(LoadedObject);
//
//	if (!ValidateLoadedSave(LoadedFPSGame))
//	{
//		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Load failed: Save data is invalid or incompatible."));
//
//		LoadedSaveGame = nullptr;
//		bPendingApplyLoadedGame = false;
//
//		OnLoadGameCompleted.Broadcast(false);
//		return false;
//	}
//
//	LoadedSaveGame = LoadedFPSGame;
//	//CurrentCheckpointID = LoadedSaveGame->CheckpointID;
//	CurrentCheckpointData = LoadedSaveGame->CheckpointData;
//	CompletedTutorialIDs = LoadedSaveGame->CompletedTutorialIDs;
//
//	bPendingApplyLoadedGame = true;
//
//	UE_LOG(LogSaveGameSubsystem, Log, TEXT("Save loaded. Level: %s"), *LoadedSaveGame->SavedLevelName.ToString());
//
//	OnLoadGameCompleted.Broadcast(true);
//	return true;
//}

//bool USaveGameSubsystem::LoadGame_Upgrade()
//{
//	if (bIsSaving)
//	{
//		OnLoadGameCompleted.Broadcast(false);
//		return false;
//	}
//
//	if (!DoesSaveExist())
//	{
//		OnLoadGameCompleted.Broadcast(false);
//		return false;
//	}
//
//	USaveGame* LoadedObject = UGameplayStatics::LoadGameFromSlot(SaveSlotName_Main, SaveUserIndex);
//	UFPSGameSave* LoadedFPSGame = Cast<UFPSGameSave>(LoadedObject);
//
//	if (!ValidateLoadedSave(LoadedFPSGame))
//	{
//		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Load failed: Save data is invalid or incompatible."));
//
//		LoadedSaveGame = nullptr;
//		bPendingApplyLoadedGame = false;
//
//		OnLoadGameCompleted.Broadcast(false);
//		return false;
//	}
//
//	LoadedSaveGame = LoadedFPSGame;
//	CurrentCheckpointData = LoadedSaveGame->CheckpointData;
//	CompletedTutorialIDs = LoadedSaveGame->CompletedTutorialIDs;
//
//	bPendingApplyLoadedGame = true;
//
//	UE_LOG(LogSaveGameSubsystem, Log, TEXT("Save loaded. Level: %s"), *LoadedSaveGame->SavedLevelName.ToString());
//
//	OnLoadGameCompleted.Broadcast(true);
//	return true;
//}

bool USaveGameSubsystem::LoadGameFromSlot(const FString& SlotName)
{
	if (bIsSaving)
	{
		OnLoadGameCompleted.Broadcast(false);
		return false;
	}

	if (SlotName.IsEmpty())
	{
		OnLoadGameCompleted.Broadcast(false);
		return false;
	}

	if (!DoesSaveExist(SlotName))
	{
		UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Save slot does not exist: %s"), *SlotName);
		OnLoadGameCompleted.Broadcast(false);
		return false;
	}

	USaveGame* LoadedObject = UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex);
	UFPSGameSave* LoadedFPSGame = Cast<UFPSGameSave>(LoadedObject);

	if (!ValidateLoadedSave(LoadedFPSGame))
	{
		LoadedSaveGame = nullptr;
		bPendingApplyLoadedGame = false;

		OnLoadGameCompleted.Broadcast(false);
		return false;
	}

	StopPlayTimeTracking();

	CurrSaveSlotName = SlotName;

	LoadedSaveGame = LoadedFPSGame;
	CurrentCheckpointData = LoadedSaveGame->CheckpointData;
	CompletedTutorialIDs = LoadedSaveGame->CompletedTutorialIDs;

	SavedAccumulatedPlayTimeSeconds = LoadedSaveGame->AccumulatedPlayTimeSeconds;
	//PlaySessionStartSeconds = 0.0;
	//bPlayTimeTracking = false;

	bPendingApplyLoadedGame = true;

	UE_LOG(LogSaveGameSubsystem, Log, TEXT("Save loaded. Slot: %s, Level: %s"), *CurrSaveSlotName, *LoadedSaveGame->SavedLevelName.ToString());

	OnLoadGameCompleted.Broadcast(true);

	return true;
}

bool USaveGameSubsystem::OpenSavedLevel()
{
	if (!IsValid(LoadedSaveGame))
	{
		UE_LOG(LogSaveGameSubsystem, Warning, TEXT("OpenSavedLevel failed: No loaded save data."));
		return false;
	}

	if (LoadedSaveGame->SavedLevelName.IsNone())
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("OpenSavedLevel failed: Saved level name is empty."));
		return false;
	}

	bPendingApplyLoadedGame = true;

	UGameplayStatics::OpenLevel(GetWorld(), LoadedSaveGame->SavedLevelName);

	return true;
}

bool USaveGameSubsystem::ApplyLoadedGame()
{
	if (!bPendingApplyLoadedGame) { return false; }
	if (!ValidateLoadedSave(LoadedSaveGame))
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("ApplyLoadedGame failed: Save data is invalid."));
		bPendingApplyLoadedGame = false;
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World)) { return false; }

	const FName CurrentLevelName = GetCurrentLevelName();
	if (CurrentLevelName != LoadedSaveGame->SavedLevelName)
	{
		UE_LOG(
			LogSaveGameSubsystem,
			Warning,
			TEXT(
				"ApplyLoadedGame aborted: Current level '%s' "
				"does not match saved level '%s'."
			),
			*CurrentLevelName.ToString(),
			*LoadedSaveGame->SavedLevelName.ToString()
		);
		return false;
	}

	/*
	 * 월드 오브젝트를 먼저 복원한 후 플레이어를 복원합니다.
	 *
	 * 예를 들어 발전기, 문, 체크포인트가 플레이어 상태에 영향을 준다면
	 * 월드 상태가 먼저 준비되는 편이 안전합니다.
	 */
	 //TODO: Map에 설치되어 있는 ItemPickUp의 경우는 어떻게 처리해야하나? 
	 // ItemPickUp과 생성된 Itme과의 매칭을 어떻게 처리해야할지 고려해야함
	RestoreRuntimeSpawnedWorldActorData(*LoadedSaveGame);
	RestoreWorldActorData(*LoadedSaveGame);
	// Legacy dropped pickups stored an actor ID instead of a self-contained snapshot.
	// Their restored actor must be available to SpawnItem, not active in the world.
	{
		TSet<AItem*> LiveInventoryItems;
		CollectRuntimeInventoryItems(FindPlayerCharacter(), LiveInventoryItems);
		TSet<FGuid> SavedInventoryIDs;
		CollectSavedInventoryIDs(*LoadedSaveGame, SavedInventoryIDs);
		if (UObjectPoolSubsystem* Pool = World->GetSubsystem<UObjectPoolSubsystem>())
		{
			for (TActorIterator<AItemPickUp> It(World); It; ++It)
			{
				AItemPickUp* Pickup = *It;
				if (!IsValid(Pickup) || !Pickup->IsActiveInPool() || !Pickup->GetLegacyItemInstanceID().IsValid()) continue;
				const TSubclassOf<AItem> ItemClass = Pickup->GetItemClass();
				if (!ItemClass || SavedInventoryIDs.Contains(Pickup->GetLegacyItemInstanceID())) continue;
				AItem* Item = Cast<AItem>(Pool->FindActorByInstanceIDFromPool(Pickup->GetLegacyItemInstanceID(), ItemClass));
				if (IsValid(Item) && !LiveInventoryItems.Contains(Item))
				{
					FItemInstanceSaveData Snapshot;
					if (Item->WriteItemSaveData(Snapshot))
						Pickup->SetDroppedItemState(Snapshot, Pickup->IsThrowableWeapon() ? Item->GetItemNum() : INDEX_NONE);
					if (Item->IsActiveInPool()) Item->DeactivateItem();
				}
			}
		}
	}
	if (!RestorePlayerData(*LoadedSaveGame))
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("ApplyLoadedGame failed: inventory restore incomplete."));
		bPendingApplyLoadedGame = false;
		return false;
	}

	CurrentCheckpointData = LoadedSaveGame->CheckpointData;
	CompletedTutorialIDs = LoadedSaveGame->CompletedTutorialIDs;

	bPendingApplyLoadedGame = false;

	UE_LOG(LogSaveGameSubsystem, Error, TEXT("Loaded game was successfully applied."));

	StartPlayTimeTracking();

	OnSaveGameApplied.Broadcast();
	return true;
}

//bool USaveGameSubsystem::DoesSaveExist() const
//{
//	return UGameplayStatics::DoesSaveGameExist(SaveSlotName_Main, SaveUserIndex);
//}

//bool USaveGameSubsystem::DeleteSave()
//{
//	if (bIsSaving)
//	{
//		UE_LOG(
//			LogSaveGameSubsystem,
//			Warning,
//			TEXT("DeleteSave failed: Save is currently in progress.")
//		);
//
//		return false;
//	}
//
//	if (!DoesSaveExist())
//	{
//		LoadedSaveGame = nullptr;
//		SaveGameBeingWritten = nullptr;
//
//		bPendingApplyLoadedGame = false;
//		CurrentCheckpointData.Reset();
//		CompletedTutorialIDs.Reset();
//
//		return true;
//	}
//
//	const bool bDeleted =
//		UGameplayStatics::DeleteGameInSlot(
//			SaveSlotName_Main,
//			SaveUserIndex
//		);
//
//	if (bDeleted)
//	{
//		LoadedSaveGame = nullptr;
//		SaveGameBeingWritten = nullptr;
//
//		bPendingApplyLoadedGame = false;
//		CurrentCheckpointData.Reset();
//		CompletedTutorialIDs.Reset();
//
//		UE_LOG(
//			LogSaveGameSubsystem,
//			Log,
//			TEXT("Save deleted. Slot: %s"),
//			*SaveSlotName_Main
//		);
//	}
//	else
//	{
//		UE_LOG(
//			LogSaveGameSubsystem,
//			Error,
//			TEXT("Failed to delete save. Slot: %s"),
//			*SaveSlotName_Main
//		);
//	}
//
//	return bDeleted;
//}

bool USaveGameSubsystem::DoesSaveExist(const FString& SlotName) const
{
	if (SlotName.IsEmpty()) { return false; }
	return UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex);
}

bool USaveGameSubsystem::DeleteSave(const FString& SlotName)
{
	if (bIsSaving) { return false; }
	if (SlotName.IsEmpty()) { return false; }

	if (!UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
	{
		return false;
	}

	const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, SaveUserIndex);
	if (!bDeleted) { return false; }

	if (IsValid(SlotIndex))
	{
		SlotIndex->Slots.RemoveAll([&SlotName](const FSaveSlotInfo& Info)
			{
				return Info.SlotName == SlotName;
			});

		SaveSlotIndex();
	}

	if (CurrSaveSlotName == SlotName)
	{
		CurrSaveSlotName.Empty();
		LoadedSaveGame = nullptr;
		bPendingApplyLoadedGame = false;
	}

	return true;
}

const TArray<FSaveSlotInfo>& USaveGameSubsystem::GetSaveSlots() const
{
	static const TArray<FSaveSlotInfo> EmptyArray;
	if (!IsValid(SlotIndex))
	{
		return EmptyArray;
	}
	return SlotIndex->Slots;
}

//void USaveGameSubsystem::StartNewGame(bool bDeleteSaveFile)
//{
//	if (bDeleteSaveFile)
//	{
//		DeleteSave();
//	}
//
//	LoadedSaveGame = nullptr;
//	SaveGameBeingWritten = nullptr;
//
//	bPendingApplyLoadedGame = false;
//	CurrentCheckpointData.Reset();
//	CompletedTutorialIDs.Reset();
//}

bool USaveGameSubsystem::StartNewGame()
{
	if (bIsSaving) { return false; }

	StopPlayTimeTracking();

	LoadedSaveGame = nullptr;
	SaveGameBeingWritten = nullptr;

	bPendingApplyLoadedGame = false;

	CurrentCheckpointData.Reset();
	CompletedTutorialIDs.Reset();

	ResetPlayTimeTracking();

	CurrSaveSlotName = GenerateSaveSlotName();

	UE_LOG(LogSaveGameSubsystem, Log, TEXT("New game slot created: %s"), *CurrSaveSlotName);

	return true;
}

void USaveGameSubsystem::SetCurrentCheckpoint(FName CheckpointID, const FTransform& RespawnTransform)
{
	if (CheckpointID.IsNone())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"SetCurrentCheckpoint failed: "
				"CheckpointID is None."
			)
		);
		return;
	}
	CurrentCheckpointData.CheckpointID = CheckpointID;
	CurrentCheckpointData.RespawnTransform = RespawnTransform;
}

void USaveGameSubsystem::SetCompletedTutorialIDs(
	const TSet<FName>& TutorialIDs)
{
	CompletedTutorialIDs = TutorialIDs;
}

void USaveGameSubsystem::AddCompletedTutorialID(
	FName TutorialID)
{
	if (!TutorialID.IsNone())
	{
		CompletedTutorialIDs.Add(TutorialID);
	}
}

FName USaveGameSubsystem::GetSavedLevelName() const
{
	return IsValid(LoadedSaveGame)
		? LoadedSaveGame->SavedLevelName
		: NAME_None;
}

//FName USaveGameSubsystem::GetCurrentCheckpointID() const
//{
//	return CurrentCheckpointID;
//}

const TSet<FName>&
USaveGameSubsystem::GetCompletedTutorialIDs() const
{
	return CompletedTutorialIDs;
}

void USaveGameSubsystem::StartPlayTimeTracking()
{
	if (bPlayTimeTracking)
	{
		return;
	}

	if (CurrSaveSlotName.IsEmpty())
	{
		UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Cannot start play time tracking: No active save slot."));
		return;
	}

	PlaySessionStartSeconds = FPlatformTime::Seconds();
	bPlayTimeTracking = true;

	UE_LOG(
		LogSaveGameSubsystem,
		Log,
		TEXT("Play time tracking started. Slot=%s, Accumulated=%.2f"),
		*CurrSaveSlotName,
		SavedAccumulatedPlayTimeSeconds
	);
}

void USaveGameSubsystem::StopPlayTimeTracking()
{
	if (!bPlayTimeTracking) { return; }

	const double CurrentSeconds = FPlatformTime::Seconds();
	const double SessionElapsedSeconds = FMath::Max(0.0, CurrentSeconds - PlaySessionStartSeconds);

	SavedAccumulatedPlayTimeSeconds += SessionElapsedSeconds;

	PlaySessionStartSeconds = 0.0;
	bPlayTimeTracking = false;

	UE_LOG(
		LogSaveGameSubsystem,
		Log,
		TEXT("Play time tracking stopped. Total=%.2f"),
		SavedAccumulatedPlayTimeSeconds
	);
}

void USaveGameSubsystem::ResetPlayTimeTracking()
{
	SavedAccumulatedPlayTimeSeconds = 0.0;
	PlaySessionStartSeconds = 0.0;
	bPlayTimeTracking = false;
}

double USaveGameSubsystem::GetAccumulatedPlayTimeSeconds() const
{
	if (!bPlayTimeTracking)
	{
		return SavedAccumulatedPlayTimeSeconds;
	}

	const double CurrentSeconds = FPlatformTime::Seconds();
	const double SessionElapsedSeconds = FMath::Max(0.0, CurrentSeconds - PlaySessionStartSeconds);

	return SavedAccumulatedPlayTimeSeconds + SessionElapsedSeconds;
}

FString USaveGameSubsystem::GetFormattedPlayTime() const
{
	const int64 TotalSeconds = FMath::Max<int64>(0, static_cast<int64>(GetAccumulatedPlayTimeSeconds()));

	const int64 Hours = TotalSeconds / 3600;
	const int64 Minutes = (TotalSeconds % 3600) / 60;
	const int64 Seconds = TotalSeconds % 60;

	return FString::Printf(
		TEXT("%02lld:%02lld:%02lld"),
		Hours,
		Minutes,
		Seconds
	);
}

UFPSGameSave* USaveGameSubsystem::CreateSaveGameObject() const
{
	USaveGame* SaveObject = UGameplayStatics::CreateSaveGameObject(UFPSGameSave::StaticClass());
	return Cast<UFPSGameSave>(SaveObject);
}

FString USaveGameSubsystem::GenerateSaveSlotName() const
{
	return FString::Printf(TEXT("Save_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
}

void USaveGameSubsystem::LoadSlotIndex()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotIndexSaveName, SaveUserIndex))
	{
		USaveGame* LoadedObject = UGameplayStatics::LoadGameFromSlot(SlotIndexSaveName, SaveUserIndex);
		SlotIndex = Cast<USaveSlotIndexSaveGame>(LoadedObject);
	}

	if (!IsValid(SlotIndex))
	{
		SlotIndex = Cast<USaveSlotIndexSaveGame>(UGameplayStatics::CreateSaveGameObject(USaveSlotIndexSaveGame::StaticClass()));
	}

	RemoveInvalidSlotEntries();
}

bool USaveGameSubsystem::SaveSlotIndex()
{
	if (!IsValid(SlotIndex)) { return false; }
	return UGameplayStatics::SaveGameToSlot(SlotIndex, SlotIndexSaveName, SaveUserIndex);
}

void USaveGameSubsystem::UpdateCurrentSlotInfo(const UFPSGameSave& SaveObject)
{
	if (!IsValid(SlotIndex)) { return; }
	if (CurrSaveSlotName.IsEmpty()) { return; }

	FSaveSlotInfo* ExistingSlot = SlotIndex->Slots.FindByPredicate([this](const FSaveSlotInfo& Info)
		{
			return Info.SlotName == CurrSaveSlotName;
		});

	if (ExistingSlot)
	{
		ExistingSlot->SavedLevelName = SaveObject.SavedLevelName;
		ExistingSlot->SavedAt = SaveObject.SavedAt;
		ExistingSlot->AccumulatedPlayTimeSeconds = SaveObject.AccumulatedPlayTimeSeconds;
	}
	else
	{
		FSaveSlotInfo NewInfo;
		NewInfo.SlotName = CurrSaveSlotName;
		NewInfo.SavedLevelName = SaveObject.SavedLevelName;
		NewInfo.SavedAt = SaveObject.SavedAt;
		NewInfo.AccumulatedPlayTimeSeconds = SaveObject.AccumulatedPlayTimeSeconds;

		SlotIndex->Slots.Add(MoveTemp(NewInfo));
	}

	SlotIndex->Slots.Sort([](const FSaveSlotInfo& A, const FSaveSlotInfo& B)
		{
			return A.SavedAt > B.SavedAt;
		});
}

void USaveGameSubsystem::RemoveInvalidSlotEntries()
{
	if (!IsValid(SlotIndex)) { return; }
	SlotIndex->Slots.RemoveAll([this](const FSaveSlotInfo& Info)
		{
			return !UGameplayStatics::DoesSaveGameExist(Info.SlotName, SaveUserIndex);
		});
}

ACharacterPlayer* USaveGameSubsystem::FindPlayerCharacter() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) { return nullptr; }
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	return Cast<ACharacterPlayer>(PlayerPawn);
}

void USaveGameSubsystem::CaptureCurrentGameState(UFPSGameSave& SaveObject)
{
	SaveObject.SaveVersion = CurrentSaveVersion;
	SaveObject.SavedLevelName = GetCurrentLevelName();
	//SaveObject.CheckpointID = CurrentCheckpointID;
	SaveObject.CheckpointData = CurrentCheckpointData;
	SaveObject.CompletedTutorialIDs = CompletedTutorialIDs;
	SaveObject.SavedAt = FDateTime::Now();
	SaveObject.AccumulatedPlayTimeSeconds = GetAccumulatedPlayTimeSeconds();

	CapturePlayerData(SaveObject);
	//CaptureWorldActorData(SaveObject);
	CaptureWorldActorData_Upgrade(SaveObject);
}

void USaveGameSubsystem::CapturePlayerData(UFPSGameSave& SaveObject)
{
	ACharacterPlayer* Player = FindPlayerCharacter();

	if (!IsValid(Player))
	{
		UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Player data was not saved: Player was not found."));
		return;
	}

	ISaveablePlayerInterface* SaveablePlayer = Cast<ISaveablePlayerInterface>(Player);

	if (!SaveablePlayer)
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Player data was not saved: ""CharacterPlayer does not implement ""ISaveablePlayerInterface."));
		return;
	}

	SaveablePlayer->WritePlayerSaveData(SaveObject.PlayerData);
}

void USaveGameSubsystem::CaptureWorldActorData(UFPSGameSave& SaveObject)
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) { return; }

	SaveObject.WorldActorData.Reset();

	TSet<FGuid> UsedSaveIDs;
	TSet<AItem*> InventoryItems;
	CollectRuntimeInventoryItems(FindPlayerCharacter(), InventoryItems);
	TSet<FGuid> LegacyLinkedIDs;
	CollectLegacyLinkedIDs(World, LegacyLinkedIDs);

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || ShouldSkipWorldItem(Actor, InventoryItems, LegacyLinkedIDs)) { continue; }

		ISaveableActorInterface* SaveableActor = Cast<ISaveableActorInterface>(Actor);
		if (!SaveableActor) { continue; }

		const FGuid SaveID = SaveableActor->GetInstanceID();

		if (!SaveID.IsValid())
		{
			UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Actor '%s' implements SaveableActorInterface ""but has an invalid SaveID."), *Actor->GetName());
			continue;
		}

		if (UsedSaveIDs.Contains(SaveID))
		{
			UE_LOG(LogSaveGameSubsystem, Error, TEXT("Duplicate SaveID detected. Actor: %s, ID: %s"), *Actor->GetName(), *SaveID.ToString());
			continue;
		}

		UsedSaveIDs.Add(SaveID);
		SaveableActor->OnBeforeSave();

		FWorldActorSaveData ActorSaveData;
		ActorSaveData.InstanceID = SaveID;
		ActorSaveData.ActorClass = Actor->GetClass();
		ActorSaveData.bRuntimeSpawned = SaveableActor->IsRuntimeSpawned();

		if (SaveableActor->ShouldSaveTransform())
		{
			//UE_LOG(LogSaveGameSubsystem, Error, TEXT("void USaveGameSubsystem::CaptureWorldActorData(UFPSGameSave& SaveObject) Save Location"));
			ActorSaveData.ActorTransform = Actor->GetActorTransform();
		}

		FMemoryWriter MemoryWriter(ActorSaveData.ActorData, true);
		FSaveGameArchive Archive(MemoryWriter);

		Actor->Serialize(Archive);

		MemoryWriter.Close();

		SaveObject.WorldActorData.Add(MoveTemp(ActorSaveData));
	}
}

void USaveGameSubsystem::CaptureWorldActorData_Upgrade(UFPSGameSave& SaveObject)
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) { return; }

	SaveObject.WorldActorData.Reset();
	SaveObject.RuntimeSpawnedWorldActorData.Reset();

	TSet<FGuid> UsedSaveIDs;
	TSet<AItem*> InventoryItems;
	CollectRuntimeInventoryItems(FindPlayerCharacter(), InventoryItems);
	TSet<FGuid> LegacyLinkedIDs;
	CollectLegacyLinkedIDs(World, LegacyLinkedIDs);

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || ShouldSkipWorldItem(Actor, InventoryItems, LegacyLinkedIDs)) { continue; }
		ISaveableActorInterface* SaveableActor = Cast<ISaveableActorInterface>(Actor);
		if (!SaveableActor) { continue; }
		const FGuid SaveID = SaveableActor->GetInstanceID();
		if (!SaveID.IsValid())
		{
			UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Actor '%s' implements SaveableActorInterface ""but has an invalid SaveID."), *Actor->GetName());
			continue;
		}
		if (UsedSaveIDs.Contains(SaveID))
		{
			UE_LOG(LogSaveGameSubsystem, Error, TEXT("Duplicate SaveID detected. Actor: %s, ID: %s"), *Actor->GetName(), *SaveID.ToString());
			continue;
		}

		UsedSaveIDs.Add(SaveID);
		SaveableActor->OnBeforeSave();

		FWorldActorSaveData ActorSaveData;
		ActorSaveData.InstanceID = SaveID;
		ActorSaveData.ActorClass = Actor->GetClass();
		ActorSaveData.bRuntimeSpawned = SaveableActor->IsRuntimeSpawned();

		if (SaveableActor->ShouldSaveTransform())
		{
			ActorSaveData.ActorTransform = Actor->GetActorTransform();
		}

		FMemoryWriter MemoryWriter(ActorSaveData.ActorData, true);
		FSaveGameArchive Archive(MemoryWriter);

		Actor->Serialize(Archive);

		MemoryWriter.Close();

		if (SaveableActor->IsRuntimeSpawned())
		{
			SaveObject.RuntimeSpawnedWorldActorData.Add(MoveTemp(ActorSaveData));
		}
		else
		{
			SaveObject.WorldActorData.Add(MoveTemp(ActorSaveData));
		}
	}
}

bool USaveGameSubsystem::RestorePlayerData(const UFPSGameSave& SaveObject)
{
	ACharacterPlayer* Player = FindPlayerCharacter();

	if (!IsValid(Player))
	{
		UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Player data was not restored: Player was not found."));
		return false;
	}

	ISaveablePlayerInterface* SaveablePlayer = Cast<ISaveablePlayerInterface>(Player);

	if (!SaveablePlayer)
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Player data was not restored: CharacterPlayer does not implement ISaveablePlayerInterface."));
		return false;
	}

	UInventorySystemComponent* Inventory = Player->GetInventorySystemComponent();
	if (!IsValid(Inventory)) return false;
	Inventory->ResetInventoryRestoreStatus();
	SaveablePlayer->LoadPlayerSaveData(SaveObject.PlayerData);
	if (!Inventory->WasInventoryRestoreSuccessful()) return false;

	FTransform TargetTransform = SaveObject.PlayerData.PlayerTransform;

	// TODO: 향후 Respawn 기능 구현시 사용.
	//if (SaveObject.CheckpointData.IsValid())
	//{
	//	TargetTransform = SaveObject.CheckpointData.RespawnTransform;
	//}

	Player->SetActorTransform(
		TargetTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	//TODO: PlayerMovementComponent에서 Player의 이전 속도가 남지 않도록 초기화해야함

	return true;
}

void USaveGameSubsystem::RestoreRuntimeSpawnedWorldActorData(const UFPSGameSave& SaveObject)
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) { return; }
	TArray<ISaveableActorInterface*> RestoredActors;
	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	if (PoolSubsystem == nullptr) { return; }

	TSet<FGuid> InventoryIDs;
	CollectSavedInventoryIDs(SaveObject, InventoryIDs);
	for (const FWorldActorSaveData& ActorSaveData : SaveObject.RuntimeSpawnedWorldActorData)
	{
		if (!ActorSaveData.InstanceID.IsValid() || InventoryIDs.Contains(ActorSaveData.InstanceID)) { continue; }
		AActor* Actor = nullptr;

		if (ActorSaveData.bRuntimeSpawned)
		{
			Actor = PoolSubsystem->SpawnFromPool(ActorSaveData.ActorClass.Get(), FVector::ZeroVector, FRotator::ZeroRotator, true);
		}

		if (!IsValid(Actor))
		{
			UE_LOG(LogSaveGameSubsystem, Warning, TEXT("Failed to restore Actor. InstanceID=%s"), *ActorSaveData.InstanceID.ToString());
			continue;
		}

		ISaveableActorInterface* SaveableActor = Cast<ISaveableActorInterface>(Actor);

		if (!SaveableActor) { continue; }

		if (SaveableActor->ShouldSaveTransform())
		{
			Actor->SetActorTransform(ActorSaveData.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}

		FMemoryReader MemoryReader(ActorSaveData.ActorData, true);
		FSaveGameArchive Archive(MemoryReader);

		Actor->Serialize(Archive);

		RestoredActors.Add(SaveableActor);
	}

	for (ISaveableActorInterface* SaveableActor : RestoredActors)
	{
		if (SaveableActor)
		{
			SaveableActor->OnAfterLoad();
		}
	}
}

void USaveGameSubsystem::RestoreWorldActorData(const UFPSGameSave& SaveObject)
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) { return; }

	// Phase 1: deserialize EVERY saved world actor before applying side effects.
	// ExitDoor can unlock another door or enable an escape trigger in OnAfterLoad.
	// Doing that while other actors are still being deserialized may overwrite
	// their restored state, depending on TActorIterator order.
	TArray<TWeakObjectPtr<AActor>> RestoredActors;
	TSet<FGuid> InventoryIDs;
	CollectSavedInventoryIDs(SaveObject, InventoryIDs);

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) { continue; }

		ISaveableActorInterface* SaveableActor = Cast<ISaveableActorInterface>(Actor);
		if (!SaveableActor) { continue; }

		const FGuid SaveID = SaveableActor->GetInstanceID();
		if (!SaveID.IsValid() || InventoryIDs.Contains(SaveID)) { continue; }

		const FWorldActorSaveData* ActorSaveData = SaveObject.FindActorData(SaveID);
		if (!ActorSaveData) { continue; }

		if (SaveableActor->ShouldSaveTransform())
		{
			Actor->SetActorTransform(
				ActorSaveData->ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}

		FMemoryReader MemoryReader(ActorSaveData->ActorData, true);
		FSaveGameArchive Archive(MemoryReader);
		Actor->Serialize(Archive);
		MemoryReader.Close();

		RestoredActors.Add(Actor);
	}

	// Phase 2: all serialized properties are now available to OnAfterLoad.
	// World references (including ExitDoor's LinkedDoor) remain level-configured;
	// only the saveable actor state is deserialized.
	for (const TWeakObjectPtr<AActor>& ActorEntry : RestoredActors)
	{
		AActor* Actor = ActorEntry.Get();
		if (!IsValid(Actor)) { continue; }

		if (ISaveableActorInterface* SaveableActor = Cast<ISaveableActorInterface>(Actor))
		{
			SaveableActor->OnAfterLoad();
		}
	}
}

void USaveGameSubsystem::HandleAsyncSaveCompleted(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	bIsSaving = false;

	if (bSuccess)
	{
		LoadedSaveGame = SaveGameBeingWritten;

		if (IsValid(LoadedSaveGame))
		{
			UpdateCurrentSlotInfo(*LoadedSaveGame);
			SaveSlotIndex();
		}


		UE_LOG(
			LogSaveGameSubsystem,
			Log,
			TEXT(
				"Save completed successfully. Slot: %s, User: %d"
			),
			*SlotName,
			UserIndex
		);
	}
	else
	{
		UE_LOG(
			LogSaveGameSubsystem,
			Error,
			TEXT(
				"Save failed. Slot: %s, User: %d"
			),
			*SlotName,
			UserIndex
		);
	}

	SaveGameBeingWritten = nullptr;

	OnSaveGameCompleted.Broadcast(bSuccess);
}

FName USaveGameSubsystem::GetCurrentLevelName() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return NAME_None;
	}

	FString LevelName =
		UGameplayStatics::GetCurrentLevelName(
			World,
			true
		);

	return FName(*LevelName);
}

bool USaveGameSubsystem::ValidateLoadedSave(const UFPSGameSave* SaveObject) const
{
	if (!IsValid(SaveObject)) { return false; }

	if (SaveObject->SaveVersion > CurrentSaveVersion)
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Save version %d is newer than supported version %d."),
			SaveObject->SaveVersion,
			CurrentSaveVersion
		);
		return false;
	}

	if (SaveObject->SavedLevelName.IsNone())
	{
		return false;
	}
	return true;
}

void USaveGameSubsystem::LoadGameResultLog()
{
	if (UGameplayStatics::DoesSaveGameExist(GameResultLogSlotName, SaveUserIndex))
	{
		USaveGame* LoadedObject = UGameplayStatics::LoadGameFromSlot(GameResultLogSlotName, SaveUserIndex);
		GameResultLog = Cast<UGameResultLogSaveGame>(LoadedObject);
	}

	if (!IsValid(GameResultLog))
	{
		GameResultLog = Cast<UGameResultLogSaveGame>(UGameplayStatics::CreateSaveGameObject(UGameResultLogSaveGame::StaticClass()));
	}

	if (!IsValid(GameResultLog))
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Failed to create GameResultLog."));
		return;
	}

	GameResultLog->GameResults.Sort(
		[](const FGameResultSaveData& A, const FGameResultSaveData& B)
		{
			return A.CompletedAt > B.CompletedAt;
		}
	);
}

bool USaveGameSubsystem::SaveGameResultLog()
{
	if (!IsValid(GameResultLog)) { return false; }
	const bool bSuccess = UGameplayStatics::SaveGameToSlot(GameResultLog, GameResultLogSlotName, SaveUserIndex);
	if (!bSuccess)
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Failed to save game result log."));
		return false;
	}
	return true;
}

FGameResultSaveData USaveGameSubsystem::BuildCurrentGameResult(EGameEndReason EndReason) const
{
	FGameResultSaveData ResultData;

	ResultData.ResultID = FGuid::NewGuid();
	ResultData.OriginalSlotName = CurrSaveSlotName;
	ResultData.EndReason = EndReason;
	ResultData.FinalLevelName = GetCurrentLevelName();
	ResultData.TotalPlayTimeSeconds = GetAccumulatedPlayTimeSeconds();
	ResultData.CompletedAt = FDateTime::Now();

	return ResultData;
}

bool USaveGameSubsystem::ShouldArchiveAndDeleteSlot(EGameEndReason EndReason) const
{
	switch (EndReason)
	{
	case EGameEndReason::BossDefeated:
	case EGameEndReason::Escaped:
	case EGameEndReason::PlayerDead:
		return true;
	default:
		return false;
	}
}

bool USaveGameSubsystem::ArchiveCurrentGameResult(EGameEndReason EndReason)
{
	if (!ShouldArchiveAndDeleteSlot(EndReason)) { return false; }
	if (CurrSaveSlotName.IsEmpty())
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Archive failed: Current save slot is empty."));
		return false;
	}
	if (!IsValid(GameResultLog))
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Archive failed: GameResultLog is invalid."));
		return false;
	}
	StopPlayTimeTracking();
	const FString CompletedSlotName = CurrSaveSlotName;
	FGameResultSaveData ResultData = BuildCurrentGameResult(EndReason);

	if (!ResultData.IsValid())
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Archive failed: Invalid GameResultSaveData."));
		return false;
	}

	GameResultLog->GameResults.Add(ResultData);
	GameResultLog->GameResults.Sort(
		[](const FGameResultSaveData& A, const FGameResultSaveData& B)
		{
			return A.CompletedAt > B.CompletedAt;
		}
	);

	if (!SaveGameResultLog())
	{
		GameResultLog->GameResults.RemoveAll(
			[ResultID = ResultData.ResultID](const FGameResultSaveData& Data)
			{
				return Data.ResultID == ResultID;
			}
		);

		UE_LOG(
			LogSaveGameSubsystem,
			Error,
			TEXT("Game result could not be saved. Current game slot will NOT be deleted.")
		);
		return false;
	}

	if (!DeleteSave(CompletedSlotName))
	{
		UE_LOG(
			LogSaveGameSubsystem,
			Error,
			TEXT("Game result was saved, but completed slot could not be deleted. Slot=%s"),
			*CompletedSlotName
		);

		return false;
	}

	UE_LOG(
		LogSaveGameSubsystem,
		Log,
		TEXT("Game archived successfully. Slot=%s, ResultID=%s, PlayTime=%.2f"),
		*CompletedSlotName,
		*ResultData.ResultID.ToString(),
		ResultData.TotalPlayTimeSeconds
	);

	return true;
}

const TArray<FGameResultSaveData>& USaveGameSubsystem::GetGameResultLogs() const
{
	static const TArray<FGameResultSaveData> EmptyResults;

	if (!IsValid(GameResultLog))
	{
		return EmptyResults;
	}

	return GameResultLog->GameResults;
}

bool USaveGameSubsystem::DeleteGameResult(const FGuid& ResultID)
{
	if (!IsValid(GameResultLog)) { return false; }

	const int32 RemovedCount = GameResultLog->GameResults.RemoveAll(
		[&ResultID](const FGameResultSaveData& Data)
		{
			return Data.ResultID == ResultID;
		}
	);

	if (RemovedCount <= 0) { return false; }

	return SaveGameResultLog();
}

bool USaveGameSubsystem::ClearGameResultLogs()
{
	if (!IsValid(GameResultLog)) { return false; }

	const TArray<FGameResultSaveData> Backup = GameResultLog->GameResults;

	GameResultLog->GameResults.Reset();

	if (!SaveGameResultLog())
	{
		GameResultLog->GameResults = Backup;
		return false;
	}

	return true;
}