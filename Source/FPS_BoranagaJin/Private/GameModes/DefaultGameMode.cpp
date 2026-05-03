


#include "GameModes/DefaultGameMode.h"

//#include "ActorComponents/DamageComponent/ACDamageSystem.h"
//#include "Characters/PawnBasePlayer/PawnPlayer.h"
#include "Engine/PlayerStartPIE.h"
//#include "Instance/CheckpointSubsystem.h"
#include "Kismet/GameplayStatics.h"
//#include "SaveGame/SaveGame.h"



void ADefaultGameMode::BeginPlay()
{
	Super::BeginPlay();

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