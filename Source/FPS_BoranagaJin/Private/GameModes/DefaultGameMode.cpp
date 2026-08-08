


#include "GameModes/DefaultGameMode.h"
#include "ObjectPoolSubsystem.h"
#include "GameFlowSubsystem.h"
#include "SaveSystem/SaveGameSubsystem.h"

#include "Engine/PlayerStartPIE.h"
#include "Kismet/GameplayStatics.h"

//#include "ActorComponents/DamageComponent/ACDamageSystem.h"
//#include "Characters/PawnBasePlayer/PawnPlayer.h"
//#include "SaveGame/SaveGame.h"
//#include "Instance/CheckpointSubsystem.h"


void ADefaultGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UObjectPoolSubsystem* Pool = GetWorld()->GetSubsystem<UObjectPoolSubsystem>())
	{
		//UE_LOG(LogTemp, Warning, TEXT("UObjectPoolSubsystem Created!!!"));

		//Pool->PrewarmPool(ProjectileClass, 50);
		//Pool->PrewarmPool(ItemPickupClass, 30);
	}

	StartGameplay();

	//----------------------------------------------------------------------------------------

	// GameMode::BeginPlay() 시점에 플레이어의 BeginPlay() 초기화가 완전히 끝나지 않았거나,
	// 인벤토리 초기화가 다음 프레임에 수행된다면 타이머로 한 프레임 늦추는 편이 안전할 것임.
	GetWorldTimerManager().SetTimerForNextTick(this, &ADefaultGameMode::TryApplyLoadedGame);


	//----------------------------------------------------------------------------------------


	//UE_LOG(LogTemp, Error, TEXT("ALevelGameMode::BeginPlay()"));


	//UCheckpointSubsystem* Subsystem = GetGameInstance()->GetSubsystem<USuraCheckpointSubsystem>();
	//if (!Subsystem) return;

	//APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	//if (!PlayerController) return;

	//FName CurrentMapName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));


	//if (Subsystem->HasSavedCheckpoint())
	//{
	//	USaveGame* CurrentSave = Subsystem->GetCurrentSave();
	//	if (CurrentSave->MapName != CurrentMapName)
	//	{
	//		if (AActor* PlayerStartActor = ChoosePlayerStart(UGameplayStatics::GetPlayerController(this, 0)))
	//		{
	//			CurrentSave->MapName = CurrentMapName;
	//			CurrentSave->SpawnTransform = PlayerStartActor->GetActorTransform();
	//			CurrentSave->CheckpointOrderIndex = -1;

	//			UGameplayStatics::SaveGameToSlot(CurrentSave, Subsystem->GetCheckpointSlotName(), 0);
	//		}
	//	}
	//}
	//else
	//{
	//	USaveGame* NewSave = Cast<USuraSaveGame>(UGameplayStatics::CreateSaveGameObject(USuraSaveGame::StaticClass()));
	//	if (AActor* PlayerStartActor = ChoosePlayerStart(UGameplayStatics::GetPlayerController(this, 0)))
	//	{
	//		NewSave->MapName = CurrentMapName;
	//		NewSave->SpawnTransform = PlayerStartActor->GetActorTransform();
	//		NewSave->CheckpointOrderIndex = -1;
	//		NewSave->PlayedVideo = EVideo::None;
	//		UGameplayStatics::SaveGameToSlot(NewSave, Subsystem->GetCheckpointSlotName(), 0);
	//		Subsystem->SetCurrentSave(NewSave);
	//	}

	//}


	//
	// if (!Subsystem->HasSavedCheckpoint())
	// {
	// 	CurrentSave->MapName = CurrentMapName;
	// 	CurrentSave->SpawnTransform =
	// 		ChoosePlayerStart_Implementation(UGameplayStatics::GetPlayerController(this, 0))->GetActorTransform();
	// 	CurrentSave->CheckpointOrderIndex = -1;
	// }

}

void ADefaultGameMode::NotifyPlayerDied()
{
	if (UGameFlowSubsystem* GameFlow = GetGameInstance()->GetSubsystem<UGameFlowSubsystem>())
	{
		GameFlow->HandlePlayerDeath();
	}
}

void ADefaultGameMode::NotifyObjectiveCompleted()
{

}

void ADefaultGameMode::StartGameplay()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameFlowSubsystem* GameFlow = GameInstance->GetSubsystem<UGameFlowSubsystem>())
		{
			// 별도 공개 함수로 Playing 상태 전환
		}
	}
}

void ADefaultGameMode::TryApplyLoadedGame()
{
	USaveGameSubsystem* SaveSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<USaveGameSubsystem>() : nullptr;

	if (SaveSubsystem && SaveSubsystem->HasPendingLoad())
	{
		SaveSubsystem->ApplyLoadedGame();
	}
}

//void AGameModeBase::RespawnToLastCheckpoint(APawnPlayer* Player)
//{
//	UCheckpointSubsystem* Subsystem = GetGameInstance()->GetSubsystem<USuraCheckpointSubsystem>();
//	ensure(Subsystem);
//
//	float MaxHealth = Player->GetDamageSystemComponent()->GetMaxHealth();
//	Player->GetDamageSystemComponent()->SetHealth(MaxHealth);
//	Player->GetDamageSystemComponent()->SetIsDead(false);
//
//	FTransform SpawnTransform = Subsystem->GetCurrentSave()->SpawnTransform;
//	Player->TeleportTo(SpawnTransform.GetLocation(), SpawnTransform.Rotator());
//}
//
//void AGameModeBase::OnPlayerFellOutOfWorld(APawnPlayer* Player)
//{
//	UCheckpointSubsystem* Subsystem = GetGameInstance()->GetSubsystem<USuraCheckpointSubsystem>();
//	ensure(Subsystem);
//
//	float CurrentHealth = Player->GetDamageSystemComponent()->GetHealth();
//	float DamageAmount = CurrentHealth > PlayerMinimumRespawnHealth ? PlayerFellRespawnHealthReduction : 0.f;
//
//	FDamageData DamageData;
//	DamageData.DamageAmount = DamageAmount;
//
//	Player->TakeDamage(DamageData, this);
//
//	FTransform SpawnTransform = Subsystem->GetCurrentSave()->SpawnTransform;
//	Player->TeleportTo(SpawnTransform.GetLocation(), SpawnTransform.Rotator());
//}
//
//void AGameModeBase::TeleportToLastCheckpoint()
//{
//	UCheckpointSubsystem* Subsystem = GetGameInstance()->GetSubsystem<USuraCheckpointSubsystem>();
//	ensure(Subsystem);
//
//	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
//	if (!IsValid(PlayerPawn)) return;
//	FTransform SpawnTransform = Subsystem->GetCurrentSave()->SpawnTransform;
//	PlayerPawn->TeleportTo(SpawnTransform.GetLocation(), SpawnTransform.Rotator());
//}