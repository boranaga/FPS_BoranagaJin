// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/WeaponSystemComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Items/Weapons/WeaponPickUp.h"
#include "Characters/Player/CharacterPlayer.h"

#include "Items/Weapons/WeaponName.h"
#include "Items/WeaponState/WeaponBaseState.h"
#include "Items/WeaponState/WeaponIdleState.h"
#include "Items/WeaponState/WeaponFiringState.h"
#include "Items/WeaponState/WeaponUnequippedState.h"
#include "Items/WeaponState/WeaponReloadingState.h"
#include "Items/WeaponState/WeaponPumpActionReloadState.h"
#include "Items/WeaponState/WeaponSwitchingState.h"
//#include "Items/WeaponState/WeaponTargetingState.h"
#include "Items/WeaponState/WeaponChargingState.h"
#include "Items/WeaponState/WeaponWaitingState.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

UWeaponSystemComponent::UWeaponSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UWeaponSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializePlayerReference();
	LoadWSCData();
	//InitStartingWeapons_Ordering();
	InitStartingWeapons_New();
}

void UWeaponSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SearchWeapon(); //TODO: 굳이 Tick으로 할 이유가 없을 듯. 필요시에 트리거로 작동하도록 해야함
	//CalculateScreenCenterWorldPositionAndDirection(ScreenCenterWorldLocation, ScreenCenterWorldDirection);
	CalculateTargetRightHandPosition();
}

#pragma region PlayerReference
void UWeaponSystemComponent::InitializePlayerReference()
{
	ACharacterPlayer* NewPlayerOwner = Cast<ACharacterPlayer>(GetOwner());
	if (IsValid(NewPlayerOwner))
	{
		PlayerOwner = NewPlayerOwner;
		PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		//PlayerController = Cast<APlayerController>(PlayerOwner->GetController())

		// Set up action bindings
		if (PlayerController)
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
				Subsystem->AddMappingContext(WeaponSystemMappingContext, 1);
			}

			if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
			{
				// Inereact With WeaponPickUp
				EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &UWeaponSystemComponent::PickUpWeapon);

				// Switch Weapon
				EnhancedInputComponent->BindAction(SwitchWeaponUpAction, ETriggerEvent::Started, this, &UWeaponSystemComponent::SwitchToNextWeapon);
				EnhancedInputComponent->BindAction(SwitchWeaponDownAction, ETriggerEvent::Started, this, &UWeaponSystemComponent::SwitchToPreviousWeapon);

				EnhancedInputComponent->BindActionValueLambda(SwitchWeapon1Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 0);
				EnhancedInputComponent->BindActionValueLambda(SwitchWeapon2Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 1);
				EnhancedInputComponent->BindActionValueLambda(SwitchWeapon3Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 2);
			}
		}
	}
}
//bool UWeaponSystemComponent::IsSceneCaptureActive()
//{
//	return bUseSceneCapture;
//}
//void UWeaponSystemComponent::UnlockWeapon(EWeaponName NewWeaponName)
//{
//	//UE_LOG(LogTemp, Error, TEXT("UWeaponSystemComponent::UnlockWeapon(EWeaponName NewWeaponName)"));
//	if (!DTWSC) return;
//	const TMap<EWeaponName, TSubclassOf<AWeapon>> WeaponClasses = DTWSC->WeaponClasses;
//
//	for (int32 i = 0; i < WeaponInventory.Num(); i++)
//	{
//		if (WeaponInventory[i]->GetWeaponName() == NewWeaponName)
//		{
//			int32 PrevIdx = CurrentWeaponIndex;
//			CurrentWeaponIndex = i;
//			CurrentWeapon = WeaponInventory[i];
//			CurrentWeapon->SwitchWeapon(PlayerOwner, true);
//			OnWeaponSwitched.Broadcast(PrevIdx, CurrentWeaponIndex);
//
//			return;
//		}
//	}
//
//	for (int32 i = 0; i < SkillWeaponInventory.Num(); i++)
//	{
//		if (SkillWeaponInventory[i]->GetWeaponName() == NewWeaponName)
//		{
//			int32 PrevIdx = CurrentSkillWeaponIndex;
//			CurrentSkillWeaponIndex = i;
//			CurrentSkillWeapon = SkillWeaponInventory[i];
//			return;
//		}
//	}
//
//	UWorld* World = GetWorld();
//	if (!World) return;
//	const TSubclassOf<AWeapon>* NewWeaponClass = WeaponClasses.Find(NewWeaponName);
//	if (!NewWeaponClass) return;
//	FActorSpawnParameters ActorSpawnParams;
//	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//	AWeapon* NewWeapon = World->SpawnActor<AWeapon>(NewWeaponClass->Get(), FTransform(), ActorSpawnParams);
//	if (!NewWeapon) return;
//	NewWeapon->InitWeapon(Cast<ACharacterPlayer>(GetOwner()));
//	AddNewWeaponToInventory(NewWeapon);
//
//	OwnedWeapons.FindOrAdd(NewWeaponName) = true;
//
//	SaveInventory();
//
//	if (CurrentWeapon == nullptr)
//	{
//		int32 PrevIdx = CurrentWeaponIndex;
//		CurrentWeaponIndex = WeaponInventory.Num() - 1;
//		CurrentWeapon = NewWeapon;
//		CurrentWeapon->SwitchWeapon(PlayerOwner, true);
//		OnWeaponSwitched.Broadcast(PrevIdx, CurrentWeaponIndex);
//	}
//}


void UWeaponSystemComponent::LoadWSCData()
{
	//DTWSC = WSCDataTableHandle.GetRow<FWeaponSystemComponentData>("");
	//if (DTWSC)
	//{
	//	bUseSceneCapture = DTWSC->bUseSceneCapture;
	//}

	//if (bUseSceneCapture)
	//{
	//	if (!PlayerOwner) { return; }
	//	USceneCaptureComponent2D* FPSceneCapture = PlayerOwner->GetSceneCaptureComponent();
	//	if (!FPSceneCapture) { return; }
	//	FPSceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	//	FPSceneCapture->ShowOnlyComponent(PlayerOwner->GetArmMesh());
	//	FPSceneCapture->ShowOnlyComponent(PlayerOwner->GetHandsMesh());

	//	PlayerOwner->GetArmMesh()->SetVisibleInSceneCaptureOnly(true);
	//	PlayerOwner->GetHandsMesh()->SetVisibleInSceneCaptureOnly(true);

	//	if (FPHUD) { FPHUD->AddToViewport(); }
	//}
}
//void UWeaponSystemComponent::InitStartingWeapons()
//{
//	if (!DTWSC) return;
//	const TMap<EWeaponName, bool> WeaponOwnerShipMap = DTWSC->WeaponOwnerShipMap;
//	const TMap<EWeaponName, TSubclassOf<AWeapon>> WeaponClasses = DTWSC->WeaponClasses;
//
//	UCustomGameInstance* GameInstance = Cast<UCustomGameInstance>(GetWorld()->GetGameInstance());
//
//	for (auto& Elem : WeaponOwnerShipMap)
//	{
//		bool bDoesGameInstanceHasWeapon = false;
//		if (GameInstance)
//		{
//			if (GameInstance->OwnedWeapons.Contains(Elem.Key))
//			{
//				if (GameInstance->OwnedWeapons[Elem.Key])
//				{
//					bDoesGameInstanceHasWeapon = true;
//					//UE_LOG(LogTemp, Error, TEXT("bDoesGameInstanceHasWeapon"));
//				}
//			}
//		}
//
//		if (Elem.Value || bDoesGameInstanceHasWeapon)
//		{
//			UWorld* World = GetWorld();
//			if (!World) return;
//			if (!WeaponClasses.Find(Elem.Key)) continue;
//			FActorSpawnParameters ActorSpawnParams;
//			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//			AWeapon* NewWeapon = World->SpawnActor<AWeapon>(WeaponClasses.Find(Elem.Key)->Get(), FTransform(), ActorSpawnParams);
//			if (!NewWeapon) continue;
//			NewWeapon->InitWeapon(Cast<ACharacterPlayer>(GetOwner()));
//
//			AddNewWeaponToInventory(NewWeapon);
//
//			if (!GameInstance) return;
//			if (GameInstance->OwnedWeapons.Contains(Elem.Key))
//			{
//				GameInstance->OwnedWeapons[Elem.Key] = true;
//			}
//			else
//			{
//				GameInstance->OwnedWeapons.Emplace(Elem.Key, true);
//			}
//		}
//	}
//
//	for (int32 i = 0; i < WeaponInventory.Num(); i++)
//	{
//		if (WeaponInventory[i]->GetWeaponName() == DTWSC->StartingWeaponName)
//		{
//			//CurrentWeaponIndex = i;
//			//ChangeWeapon(CurrentWeaponIndex);
//			//CurrentWeaponIndex = 0;
//			//SwitchToIndex(i);
//
//			int32 PrevIdx = CurrentWeaponIndex;
//			CurrentWeaponIndex = i;
//			CurrentWeapon = WeaponInventory[i];
//			CurrentWeapon->SwitchWeapon(PlayerOwner, true);
//			OnWeaponSwitched.Broadcast(PrevIdx, CurrentWeaponIndex);
//
//			return;
//		}
//	}
//
//	AWeapon* NewWeapon;
//
//	if (DTWSC->WeaponClasses.Contains(DTWSC->StartingWeaponName))
//	{
//		UWorld* const World = GetWorld();
//		if (World != nullptr && PlayerOwner != nullptr)
//		{
//			FActorSpawnParameters ActorSpawnParams;
//			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//			NewWeapon = GetWorld()->SpawnActor<AWeapon>(DTWSC->WeaponClasses[DTWSC->StartingWeaponName], PlayerOwner->GetActorTransform(), ActorSpawnParams);
//			NewWeapon->InitWeapon(PlayerOwner);
//		}
//	}
//
//	WeaponInventory.AddUnique(NewWeapon);
//
//	if (CurrentWeapon == nullptr)
//	{
//		//CurrentWeapon = NewWeapon;
//		//CurrentWeapon->SwitchWeapon(PlayerOwner, true);
//
//		int32 PrevIdx = CurrentWeaponIndex;
//		CurrentWeaponIndex = WeaponInventory.Num() - 1;
//		CurrentWeapon = NewWeapon;
//		CurrentWeapon->SwitchWeapon(PlayerOwner, true);
//		OnWeaponSwitched.Broadcast(PrevIdx, CurrentWeaponIndex);
//	}
//}

//void UWeaponSystemComponent::InitStartingWeapons_Ordering()
//{
//	// if (!DTWSC) return;
//	// const TMap<EWeaponName, bool> WeaponOwnerShipMap = DTWSC->WeaponOwnerShipMap;
//	// const TMap<EWeaponName, TSubclassOf<AWeapon>> WeaponClasses = DTWSC->WeaponClasses;
//	//
//	// UCustomGameInstance* GameInstance = Cast<UCustomGameInstance>(GetWorld()->GetGameInstance());
//	//
//	// for (EWeaponName WeaponName : TEnumRange<EWeaponName>())
//	// {
//	// 	bool bDoesGameInstanceHasWeapon = false;
//	// 	if (GameInstance)
//	// 	{
//	// 		if (GameInstance->OwnedWeapons.Contains(WeaponName))
//	// 		{
//	// 			if (GameInstance->OwnedWeapons[WeaponName])
//	// 			{
//	// 				bDoesGameInstanceHasWeapon = true;
//	// 				//UE_LOG(LogTemp, Error, TEXT("bDoesGameInstanceHasWeapon"));
//	// 			}
//	// 		}
//	// 	}
//	//
//	// 	if (WeaponOwnerShipMap[WeaponName] || bDoesGameInstanceHasWeapon)
//	// 	{
//	// 		UWorld* World = GetWorld();
//	// 		if (!World) return;
//	// 		if (!WeaponClasses.Find(WeaponName)) continue;
//	// 		FActorSpawnParameters ActorSpawnParams;
//	// 		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//	// 		AWeapon* NewWeapon = World->SpawnActor<AWeapon>(WeaponClasses.Find(WeaponName)->Get(), FTransform(), ActorSpawnParams);
//	// 		if (!NewWeapon) continue;
//	// 		NewWeapon->InitWeapon(Cast<ACharacterPlayer>(GetOwner()));
//	//
//	// 		AddNewWeaponToInventory(NewWeapon);
//	//
//	// 		if (!GameInstance) return;
//	// 		if (GameInstance->OwnedWeapons.Contains(WeaponName))
//	// 		{
//	// 			GameInstance->OwnedWeapons[WeaponName] = true;
//	// 		}
//	// 		else
//	// 		{
//	// 			GameInstance->OwnedWeapons.Emplace(WeaponName, true);
//	// 		}
//	// 	}
//	// }
//	//
//	//
//	//
//	// for (int32 i = 0; i < WeaponInventory.Num(); i++)
//	// {
//	// 	//UE_LOG(LogTemp, Error, TEXT("(int32 i = 0; i < WeaponInventory.Num(); i++)"));
//	//
//	// 	//UE_LOG(LogTemp, Log, TEXT("Weapon: %s"), *UEnum::GetValueAsString(WeaponInventory[i]->GetWeaponName()));
//	//
//	// 	if (WeaponInventory[i]->GetWeaponName() == DTWSC->StartingWeaponName)
//	// 	{
//	// 		//UE_LOG(LogTemp, Error, TEXT("Already has Starting weapon"));
//	// 		//CurrentWeaponIndex = i;
//	// 		//ChangeWeapon(CurrentWeaponIndex);
//	// 		//CurrentWeaponIndex = 0;
//	// 		//SwitchToIndex(i);
//	//
//	// 		int32 PrevIdx = CurrentWeaponIndex;
//	// 		CurrentWeaponIndex = i;
//	// 		CurrentWeapon = WeaponInventory[i];
//	// 		CurrentWeapon->SwitchWeapon(PlayerOwner, true);
//	// 		OnWeaponSwitched.Broadcast(PrevIdx, CurrentWeaponIndex);
//	// 		return;
//	// 	}
//	// }
//	//
//	// //AWeapon* NewWeapon;
//	//
//	// //if (DTWSC->WeaponClasses.Contains(DTWSC->StartingWeaponName))
//	// //{
//	// //	UWorld* const World = GetWorld();
//	// //	if (World != nullptr && PlayerOwner != nullptr)
//	// //	{
//	// //		FActorSpawnParameters ActorSpawnParams;
//	// //		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//	// //		NewWeapon = GetWorld()->SpawnActor<AWeapon>(DTWSC->WeaponClasses[DTWSC->StartingWeaponName], PlayerOwner->GetActorTransform(), ActorSpawnParams);
//	// //		NewWeapon->InitWeapon(PlayerOwner);
//	// //	}
//	// //}
//	//
//	// //WeaponInventory.AddUnique(NewWeapon);
//	//
//	// //if (CurrentWeapon == nullptr)
//	// //{
//	// //	//CurrentWeapon = NewWeapon;
//	// //	//CurrentWeapon->SwitchWeapon(PlayerOwner, true);
//	//
//	// //	int32 PrevIdx = CurrentWeaponIndex;
//	// //	CurrentWeaponIndex = WeaponInventory.Num() - 1;
//	// //	CurrentWeapon = NewWeapon;
//	// //	CurrentWeapon->SwitchWeapon(PlayerOwner, true);
//	// //	OnWeaponSwitched.Broadcast(PrevIdx, CurrentWeaponIndex);
//	// //}
//
//	// <suhyeon>  weapon save data changed
//
//	//UE_LOG(LogTemp, Error, TEXT("UWeaponSystemComponent::InitStartingWeapons_Ordering()"));
//
//	if (!DTWSC)
//	{
//		return;
//	}
//	const TMap<EWeaponName, TSubclassOf<AWeapon>> WeaponClasses = DTWSC->WeaponClasses;
//
//
//	UCheckpointSubsystem* CheckpointSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCheckpointSubsystem>();
//	if (!CheckpointSubsystem) { return; }
//	UCustumSaveGame* CurrentSave = CheckpointSubsystem->GetCurrentSave();
//
//	const bool bIsValidContinue = (CurrentSave && CurrentSave->OwnedWeapons.Num() > 0);
//
//	const TMap<EWeaponName, bool>& WeaponOwnerShipMap = (bIsValidContinue)
//		? CurrentSave->OwnedWeapons
//		: DTWSC->WeaponOwnerShipMap;
//
//	for (EWeaponName WeaponName : TEnumRange<EWeaponName>())
//	{
//		const FString WeaponNameString = UEnum::GetValueAsString(WeaponName);
//		const bool* bIsOwnedPtr = WeaponOwnerShipMap.Find(WeaponName);
//
//		if (bIsOwnedPtr && *bIsOwnedPtr)
//		{
//			UWorld* World = GetWorld();
//			if (!World)
//			{
//				continue;
//			}
//
//			const TSubclassOf<AWeapon>* WeaponClassPtr = WeaponClasses.Find(WeaponName);
//			if (!WeaponClassPtr || !(*WeaponClassPtr))
//			{
//				continue;
//			}
//
//			FActorSpawnParameters ActorSpawnParams;
//			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//			AWeapon* NewWeapon = World->SpawnActor<AWeapon>(*WeaponClassPtr, FTransform(), ActorSpawnParams);
//
//			if (!NewWeapon)
//			{
//				continue;
//			}
//
//			NewWeapon->InitWeapon(Cast<ACharacterPlayer>(GetOwner()));
//			AddNewWeaponToInventory(NewWeapon);
//			OwnedWeapons.FindOrAdd(WeaponName) = true;
//		}
//	}
//
//	SaveInventory();
//
//	for (int32 i = 0; i < WeaponInventory.Num(); i++)
//	{
//		if (WeaponInventory[i] && WeaponInventory[i]->GetWeaponName() == DTWSC->StartingWeaponName)
//		{
//			int32 PrevIdx = CurrentWeaponIndex;
//			CurrentWeaponIndex = i;
//			CurrentWeapon = WeaponInventory[i];
//			CurrentWeapon->SwitchWeapon(PlayerOwner, true);
//
//			OnWeaponSwitched.Broadcast(PrevIdx, CurrentWeaponIndex);
//			return;
//		}
//	}
//}

//void UWeaponSystemComponent::InitStartingWeapons_Fuck()
//{
//	if (!DTWSC)
//	{
//		return;
//	}
//	const TMap<EWeaponName, TSubclassOf<AWeapon>> WeaponClasses = DTWSC->WeaponClasses;
//
//	TMap<EWeaponName, bool> NewWeaponOwnerShip;
//
//	FName CurrentMapName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));
//
//	if (CurrentMapName == FName("new_rhkdwls5") || CurrentMapName == FName("rhkdwls5_partition"))
//	{
//		UE_LOG(LogTemp, Error, TEXT("Level 1"));
//
//		OwnedWeapons.FindOrAdd(EWeaponName::WeaponName_Rifle) = true;
//		OwnedWeapons.FindOrAdd(EWeaponName::WeaponName_ShotGun) = true;
//	}
//	else
//	{
//		UE_LOG(LogTemp, Error, TEXT("incorrect Name"));
//
//		OwnedWeapons.FindOrAdd(EWeaponName::WeaponName_Rifle) = true;
//		OwnedWeapons.FindOrAdd(EWeaponName::WeaponName_ShotGun) = true;
//		OwnedWeapons.FindOrAdd(EWeaponName::WeaponName_MissileLauncher) = true;
//	}
//
//
//	//const TMap<EWeaponName, bool>& WeaponOwnerShipMap = (bIsValidContinue)
//	//	? CurrentSave->OwnedWeapons
//	//	: DTWSC->WeaponOwnerShipMap;
//
//	for (EWeaponName WeaponName : TEnumRange<EWeaponName>())
//	{
//		const bool* bIsOwnedPtr = OwnedWeapons.Find(WeaponName);
//		if (bIsOwnedPtr && *bIsOwnedPtr)
//		{
//			UWorld* World = GetWorld();
//			if (!World)
//			{
//				continue;
//			}
//
//			const TSubclassOf<AWeapon>* WeaponClassPtr = WeaponClasses.Find(WeaponName);
//			if (!WeaponClassPtr || !(*WeaponClassPtr))
//			{
//				continue;
//			}
//
//			FActorSpawnParameters ActorSpawnParams;
//			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//			AWeapon* NewWeapon = World->SpawnActor<AWeapon>(*WeaponClassPtr, FTransform(), ActorSpawnParams);
//
//			if (!NewWeapon)
//			{
//				continue;
//			}
//
//			NewWeapon->InitWeapon(Cast<ACharacterPlayer>(GetOwner()));
//			AddNewWeaponToInventory(NewWeapon);
//			OwnedWeapons.FindOrAdd(WeaponName) = true;
//		}
//	}
//
//	//SaveInventory();
//
//	for (int32 i = 0; i < WeaponInventory.Num(); i++)
//	{
//		if (WeaponInventory[i] && WeaponInventory[i]->GetWeaponName() == DTWSC->StartingWeaponName)
//		{
//			int32 PrevIdx = CurrentWeaponIndex;
//			CurrentWeaponIndex = i;
//			CurrentWeapon = WeaponInventory[i];
//			CurrentWeapon->SwitchWeapon(PlayerOwner, true);
//
//			OnWeaponSwitched.Broadcast(PrevIdx, CurrentWeaponIndex);
//			return;
//		}
//	}
//}

void UWeaponSystemComponent::InitStartingWeapons_New()
{
	//if (!DTWSC)
	//{
	//	return;
	//}
	//const TMap<EWeaponName, TSubclassOf<AWeapon>> WeaponClasses = DTWSC->WeaponClasses;

	//TMap<EWeaponName, bool> NewWeaponOwnerShip;

	//FName CurrentMapName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));

	//if (CurrentMapName == FName("new_rhkdwls5") || CurrentMapName == FName("rhkdwls5_partition"))
	//{
	//	UE_LOG(LogTemp, Error, TEXT("Level 1"));

	//	OwnedWeapons.FindOrAdd(EWeaponName::WeaponName_Rifle) = true;
	//	OwnedWeapons.FindOrAdd(EWeaponName::WeaponName_ShotGun) = true;
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Error, TEXT("incorrect Name"));

	//	OwnedWeapons.FindOrAdd(EWeaponName::WeaponName_Rifle) = true;
	//	OwnedWeapons.FindOrAdd(EWeaponName::WeaponName_ShotGun) = true;
	//	OwnedWeapons.FindOrAdd(EWeaponName::WeaponName_MissileLauncher) = true;
	//}


	////const TMap<EWeaponName, bool>& WeaponOwnerShipMap = (bIsValidContinue)
	////	? CurrentSave->OwnedWeapons
	////	: DTWSC->WeaponOwnerShipMap;

	//for (EWeaponName WeaponName : TEnumRange<EWeaponName>())
	//{
	//	const bool* bIsOwnedPtr = OwnedWeapons.Find(WeaponName);
	//	if (bIsOwnedPtr && *bIsOwnedPtr)
	//	{
	//		UWorld* World = GetWorld();
	//		if (!World)
	//		{
	//			continue;
	//		}

	//		const TSubclassOf<AWeapon>* WeaponClassPtr = WeaponClasses.Find(WeaponName);
	//		if (!WeaponClassPtr || !(*WeaponClassPtr))
	//		{
	//			continue;
	//		}

	//		FActorSpawnParameters ActorSpawnParams;
	//		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	//		AWeapon* NewWeapon = World->SpawnActor<AWeapon>(*WeaponClassPtr, FTransform(), ActorSpawnParams);

	//		if (!NewWeapon)
	//		{
	//			continue;
	//		}

	//		NewWeapon->InitWeapon(Cast<APawnPlayer>(GetOwner()));
	//		AddNewWeaponToInventory(NewWeapon);
	//		OwnedWeapons.FindOrAdd(WeaponName) = true;
	//	}
	//}

	////SaveInventory();

	//for (int32 i = 0; i < WeaponInventory.Num(); i++)
	//{
	//	if (WeaponInventory[i] && WeaponInventory[i]->GetWeaponName() == DTWSC->StartingWeaponName)
	//	{
	//		int32 PrevIdx = CurrentWeaponIndex;
	//		CurrentWeaponIndex = i;
	//		CurrentWeapon = WeaponInventory[i];
	//		CurrentWeapon->SwitchWeapon(PlayerOwner, true);

	//		OnWeaponSwitched.Broadcast(PrevIdx, CurrentWeaponIndex);
	//		return;
	//	}
	//}
}

void UWeaponSystemComponent::SaveInventory()
{
	// MEMO: 급해서 일단 비활성화
	//UCheckpointSubsystem* CheckpointSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCheckpointSubsystem>();
	//check(CheckpointSubsystem);
	//FName CurrentMapName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));
	//CheckpointSubsystem->SaveCheckpoint(CurrentMapName, PlayerOwner->GetActorTransform(), -1);
	////GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("Saved Checkpoint"));

	//------------------------------------
}
#pragma endregion

#pragma region SearchWeapon
bool UWeaponSystemComponent::SearchWeapon()
{
	TArray<TEnumAsByte<EObjectTypeQuery>> traceObjectTypes;
	traceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Visibility)); //TODO: Customize Collision Channel

	TArray<AActor*> ignoreActors;
	ignoreActors.Init(PlayerOwner, 1);

	FVector sphereSpwanLocation = PlayerOwner->GetActorLocation();

	TArray<AActor*> overlappedActors;

	bool bIsWeaponInViewPort = UKismetSystemLibrary::SphereOverlapActors(GetWorld(), sphereSpwanLocation, SearchWeaponRadius, traceObjectTypes, nullptr, ignoreActors, overlappedActors);

	float MinDistanceToWeapon = SearchWeaponRadius;
	AWeaponPickUp* NearestWeapon = nullptr;

	for (AActor* overlappedActor : overlappedActors)
	{
		AWeaponPickUp* WeaponObject = Cast<AWeaponPickUp>(overlappedActor);
		if (WeaponObject != nullptr)
		{
			if (IsInViewport(GetScreenPositionOfWorldLocation(WeaponObject->GetActorLocation()).Get<0>(), SearchWeaponViewportRatio_Width, SearchWeaponViewportRatio_Height))
			{
				float DistanceToWeapon = PlayerOwner->GetDistanceTo(WeaponObject);
				if (DistanceToWeapon < MinDistanceToWeapon)
				{
					MinDistanceToWeapon = DistanceToWeapon;
					NearestWeapon = WeaponObject;

					// TODO: Overlapped weapon UI on/off
				}
			}
		}
	}

	OverlappedWeapon = NearestWeapon;

	return bIsWeaponInViewPort;
}

TTuple<FVector2D, bool> UWeaponSystemComponent::GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const
{
	FVector2D ScreenLocation = FVector2D::ZeroVector;
	bool bResult = UGameplayStatics::ProjectWorldToScreen(PlayerController, SearchLocation, ScreenLocation);

	return MakeTuple(ScreenLocation, bResult);
}

bool UWeaponSystemComponent::IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width, float ScreenRatio_Height) const
{
	FVector2D ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();

	bool bIsInWidth = true;
	bool bIsInHeight = true;

	// Check Width
	if (ScreenRatio_Width == 0.0f || UKismetMathLibrary::Abs(ScreenRatio_Width) > 1.0f || (ScreenRatio_Width == (1.0f - ScreenRatio_Width)))
	{
		if (ActorScreenPosition.X >= 0.0f && ActorScreenPosition.X <= ViewportSize.X)
		{
			bIsInWidth = true;
		}
		else
		{
			bIsInWidth = false;
		}
	}
	else
	{
		float LargeScreenRatio_Width;
		float SmallScreenRatio_Width;

		if (ScreenRatio_Width < (1.0f - ScreenRatio_Width))
		{
			LargeScreenRatio_Width = 1.0f - ScreenRatio_Width;
			SmallScreenRatio_Width = ScreenRatio_Width;
		}
		else
		{
			LargeScreenRatio_Width = ScreenRatio_Width;
			SmallScreenRatio_Width = 1.0f - ScreenRatio_Width;
		}

		if (ActorScreenPosition.X >= ViewportSize.X * SmallScreenRatio_Width && ActorScreenPosition.X <= ViewportSize.X * LargeScreenRatio_Width)
		{
			bIsInWidth = true;
		}
		else
		{
			bIsInWidth = false;
		}
	}

	// Check Height
	if (ScreenRatio_Height == 0.0f || UKismetMathLibrary::Abs(ScreenRatio_Height) > 1.0f || (ScreenRatio_Height == (1.0f - ScreenRatio_Height)))
	{
		if (ActorScreenPosition.Y >= 0.0f && ActorScreenPosition.Y <= ViewportSize.Y)
		{
			bIsInHeight = true;
		}
		else
		{
			bIsInHeight = false;
		}
	}
	else
	{
		float LargeScreenRatio_Height;
		float SmallScreenRatio_Height;

		if (ScreenRatio_Height < (1.0f - ScreenRatio_Height))
		{
			LargeScreenRatio_Height = 1.0f - ScreenRatio_Height;
			SmallScreenRatio_Height = ScreenRatio_Height;
		}
		else
		{
			LargeScreenRatio_Height = ScreenRatio_Height;
			SmallScreenRatio_Height = 1.0f - ScreenRatio_Height;
		}

		if (ActorScreenPosition.Y >= ViewportSize.Y * SmallScreenRatio_Height && ActorScreenPosition.Y <= ViewportSize.Y * LargeScreenRatio_Height)
		{
			bIsInHeight = true;
		}
		else
		{
			bIsInHeight = false;
		}
	}

	// Return
	if (bIsInWidth && bIsInHeight)
	{
		return true;
	}
	else
	{
		return false;
	}
}

#pragma region Interaction

void UWeaponSystemComponent::PickUpWeapon()
{
	if (OverlappedWeapon != nullptr)
	{
		if (CurrentWeapon == nullptr || (CurrentWeapon != nullptr && CurrentWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Idle))
		{
			if (OverlappedWeapon->IsMagazine())
			{
				ObtainAmmo(OverlappedWeapon);
			}
			else
			{
				ObtainNewWeapon(OverlappedWeapon);
			}
		}
	}
}
bool UWeaponSystemComponent::ObtainNewWeapon(AWeaponPickUp* NewWeaponPickUp)
{
	if (!NewWeaponPickUp || !PlayerOwner)
	{
		return false;
	}
	const EWeaponName NewWeaponName = NewWeaponPickUp->GetWeaponName();
	const int32 NewAmmo = NewWeaponPickUp->GetAmmo();

	auto TryAddAmmoIfAlreadyOwned = [&](TArray<AWeapon*>& Inventory) -> bool
		{
			for (AWeapon* Weapon : Inventory)
			{
				if (Weapon && Weapon->GetWeaponName() == NewWeaponName)
				{
					//UE_LOG(LogTemp, Warning, TEXT("Already Possess"));
					if (Weapon->AddAmmo(NewAmmo))
					{
						NewWeaponPickUp->DestroyWeaponPickUp();
					}
					return true;
				}
			}
			return false;
		};

	if (TryAddAmmoIfAlreadyOwned(WeaponInventory) || TryAddAmmoIfAlreadyOwned(SkillWeaponInventory))
	{
		return false;
	}

	AWeapon* NewWeapon = NewWeaponPickUp->SpawnWeapon(PlayerOwner);
	if (!NewWeapon) { return false; }
	NewWeaponPickUp->DestroyWeaponPickUp();

	if (NewWeapon->IsSkillWeapon())
	{
		SkillWeaponInventory.AddUnique(NewWeapon);
		CurrentSkillWeapon = NewWeapon;
		CurrentSkillWeapon->EquipWeapon(PlayerOwner, true);

		//suhyeon 여기에 skillweapon획득 델리게이트 연결
		// 스킬 무기 장착 후 델리게이트를 브로드캐스트
		OnSkillWeaponEquipped.Broadcast(CurrentSkillWeapon);
	}
	else
	{
		WeaponInventory.AddUnique(NewWeapon);
		if (!CurrentWeapon)
		{
			CurrentWeapon = NewWeapon;
			CurrentWeapon->SwitchWeapon(PlayerOwner, true);
		}
		OnWeaponPickedUp.Broadcast(NewWeaponPickUp->GetWeaponName()); /** suhyeon **/
	}

	OwnedWeapons.FindOrAdd(NewWeaponName) = true;
	SaveInventory();

	return true;
}

bool UWeaponSystemComponent::ObtainAmmo(AWeaponPickUp* MagazinePickUp)
{
	for (AWeapon* WeaponInPossession : WeaponInventory)
	{
		if (WeaponInPossession->GetWeaponName() == MagazinePickUp->GetWeaponName())
		{
			if (WeaponInPossession->AddAmmo(MagazinePickUp->GetAmmo()))
			{
				MagazinePickUp->DestroyWeaponPickUp();
				return true;
			}
		}
	}

	for (AWeapon* WeaponInPossession : SkillWeaponInventory)
	{
		if (WeaponInPossession->GetWeaponName() == MagazinePickUp->GetWeaponName())
		{
			if (WeaponInPossession->AddAmmo(MagazinePickUp->GetAmmo()))
			{
				MagazinePickUp->DestroyWeaponPickUp();
				return true;
			}
		}
	}

	return false;
}

#pragma endregion

void UWeaponSystemComponent::ZoomIn(bool bZoomIn)
{
	bIsZoomIn = bZoomIn;

	//UE_LOG(LogTemp, Warning, TEXT("ZOOM Toggled!!!"));
}

bool UWeaponSystemComponent::IsWeaponModifyingCamFov()
{
	if (CurrentWeapon)
	{
		if (CurrentWeapon->IsModifyingPlayerCamFov() || bIsZoomIn)
		{
			return true;
		}
	}
	return false;
}

#pragma endregion

#pragma region Aiming

void UWeaponSystemComponent::SetRightHandToAimSocketOffset(FVector offset)
{
	RightHandToAimSocketOffset = offset;
}

FVector UWeaponSystemComponent::CalculateScreenCenterWorldPositionAndDirection(FVector& OutWorldPosition, FVector& OutWorldDirection) const
{
	//APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return FVector::ZeroVector;
	}

	// ȭ�� ũ�� ��������
	FVector2D ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();

	// ȭ�� �߾� ��ǥ ���?
	FVector2D ScreenCenter(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);

	// ȭ�� �߽��� ���� ��ġ�� ���� ��������
	FVector WorldPosition, WorldDirection;
	if (PlayerController->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, WorldPosition, WorldDirection))
	{
		OutWorldPosition = WorldPosition;
		OutWorldDirection = WorldDirection;
		return WorldPosition + (WorldDirection * 15.0f); // TODO: ���� �Ÿ��� �������? ��ġ ���� �����ϵ��� �����ϱ�
	}

	return FVector::ZeroVector;
}

FVector UWeaponSystemComponent::CalculateTargetRightHandPosition()
{
	const FVector AimLocation = CalculateScreenCenterWorldPositionAndDirection(ScreenCenterWorldLocation, ScreenCenterWorldDirection);

	const FRotator MeshRotation = PlayerOwner->GetArmMesh()->GetComponentRotation();

	const FVector TargetRightHandLocation = AimLocation - MeshRotation.RotateVector(RightHandToAimSocketOffset);

	TargetRightHandWorldLocation = TargetRightHandLocation;

	return TargetRightHandLocation;
}

FTransform UWeaponSystemComponent::GetWeaponAimSocketRelativeTransform()
{
	return FTransform();
}
#pragma endregion


#pragma region SwitchWeapon
bool UWeaponSystemComponent::IsCurrentSkillWeaponTargeting()
{
	if (CurrentWeapon && CurrentWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Waiting) { return true; }
	else { return false; }
}
void UWeaponSystemComponent::SwitchToPreviousWeapon()
{
	if (IsCurrentSkillWeaponTargeting()
		|| CurrentWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
		|| CurrentWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
		return;
	}
	if (WeaponInventory.Num() > 1)
	{
		const int32 PrevIndex = CurrentWeaponIndex;

		CurrentWeaponIndex--;
		if (CurrentWeaponIndex < 0)
		{
			CurrentWeaponIndex = WeaponInventory.Num() + CurrentWeaponIndex;
		}
		ChangeWeapon(CurrentWeaponIndex);

		UE_LOG(LogTemp, Warning, TEXT("Broadcasting weapon switch: %d -> %d"), PrevIndex, CurrentWeaponIndex);
		OnWeaponSwitched.Broadcast(PrevIndex, CurrentWeaponIndex);
	}
}
void UWeaponSystemComponent::SwitchToNextWeapon()
{
	if (IsCurrentSkillWeaponTargeting()
		|| CurrentWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
		|| CurrentWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
		return;
	}
	if (WeaponInventory.Num() > 1)
	{
		const int32 PrevIndex = CurrentWeaponIndex;

		CurrentWeaponIndex = (CurrentWeaponIndex + 1) % WeaponInventory.Num();
		ChangeWeapon(CurrentWeaponIndex);

		UE_LOG(LogTemp, Warning, TEXT("Broadcasting weapon switch: %d -> %d"), PrevIndex, CurrentWeaponIndex);
		OnWeaponSwitched.Broadcast(PrevIndex, CurrentWeaponIndex);
	}
}
void UWeaponSystemComponent::SwitchToIndex(int32 idx)
{
	if (IsCurrentSkillWeaponTargeting()
		|| CurrentWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
		|| CurrentWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
		return;
	}
	if (WeaponInventory.IsValidIndex(idx) && CurrentWeaponIndex != idx)
	{
		const int32 PrevIndex = CurrentWeaponIndex;
		CurrentWeaponIndex = idx;
		ChangeWeapon(CurrentWeaponIndex);

		UE_LOG(LogTemp, Warning, TEXT("Broadcasting weapon switch: %d -> %d"), PrevIndex, CurrentWeaponIndex);
		OnWeaponSwitched.Broadcast(PrevIndex, CurrentWeaponIndex);
	}
}
void UWeaponSystemComponent::SwitchToOtherWeapon()
{
	WeaponInventory[CurrentWeaponIndex]->SwitchWeapon(PlayerOwner, true);
	CurrentWeapon = WeaponInventory[CurrentWeaponIndex];
}
void UWeaponSystemComponent::ChangeWeapon(int32 WeaponIndex)
{
	if (WeaponInventory.IsValidIndex(WeaponIndex))
	{
		if (IsValid(CurrentWeapon))
		{
			CurrentWeapon->SwitchWeapon(PlayerOwner, false);
		}
	}
}
#pragma endregion

void UWeaponSystemComponent::AddNewWeaponToInventory(AWeapon* NewWeapon)
{
	if (!NewWeapon) { return; }
	if (NewWeapon->IsSkillWeapon())
	{
		SkillWeaponInventory.AddUnique(NewWeapon);
		if (!CurrentSkillWeapon)
		{
			CurrentSkillWeaponIndex = SkillWeaponInventory.Num() - 1;
			CurrentSkillWeapon = NewWeapon;
			CurrentSkillWeapon->EquipWeapon(PlayerOwner, true);
		}
		// 스킬 무기 장착 후 skillweapon획득 델리게이트를 브로드캐스트
		OnSkillWeaponEquipped.Broadcast(CurrentSkillWeapon);
	}
	else
	{
		WeaponInventory.AddUnique(NewWeapon);
		//if (!CurrentWeapon)
		//{
		//	int32 PrevIdx = CurrentWeaponIndex;
		//	CurrentWeaponIndex = WeaponInventory.Num() - 1;
		//	CurrentWeapon = NewWeapon;
		//	CurrentWeapon->SwitchWeapon(PlayerOwner, true);
		//	OnWeaponSwitched.Broadcast(PrevIdx, CurrentWeaponIndex);
		//}
	}
}

#pragma region Control
bool UWeaponSystemComponent::TryTakeControl(AWeapon* NewWeapon)
{
	if (!CurrentWeapon)
	{
		ControllingWeapon = NewWeapon;
		return true;
	}
	else
	{
		EWeaponStateType CurrWeaponStateType = CurrentWeapon->GetCurrentState()->GetWeaponStateType();
		if (CurrWeaponStateType == EWeaponStateType::WeaponStateType_Idle) //TODO: 다른 state일 때 스킬 어떻게 작동해야 하는지
		{
			CurrentWeapon->ChangeState(CurrentWeapon->WaitingState);
			ControllingWeapon = NewWeapon;
			return true;
		}
		else
		{
			return false;
		}
	}
}
void UWeaponSystemComponent::ReleaseControl()
{
	if (!CurrentWeapon)
	{
		ControllingWeapon = nullptr;
	}
	else
	{
		EWeaponStateType CurrWeaponStateType = CurrentWeapon->GetCurrentState()->GetWeaponStateType();
		if (CurrWeaponStateType == EWeaponStateType::WeaponStateType_Waiting)
		{
			CurrentWeapon->ChangeState(CurrentWeapon->IdleState);
			ControllingWeapon = CurrentWeapon;
		}
		else
		{
		}
	}
}
#pragma endregion

