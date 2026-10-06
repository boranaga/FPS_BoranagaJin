#include "Items/Weapons/ThrowableWeapon/ThrowableWeapon.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeaponProjectile.h"
#include "Items/InventorySystemComponent.h"
#include "Characters/Player/CharacterPlayer.h"
#include "PlayerUISubsystem.h"
#include "ObjectPoolSubsystem.h"

#include "UI/AmmoCounterWidget.h"
#include "UI/WeaponAimUIWidget.h"

#include "Data/ThrowableWeaponPrimaryDataAsset.h"
#include "Data/ThrowableWeaponData.h"

#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Kismet/GameplayStatics.h"

AThrowableWeapon::AThrowableWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	MuzzlePoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Muzzle"));
	MuzzlePoint->SetupAttachment(GetRootComponent(), FName(TEXT("Muzzle")));
	MuzzlePoint->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MuzzlePoint->SetCollisionObjectType(ECC_GameTraceChannel5); //Weapon
	MuzzlePoint->SetCollisionResponseToAllChannels(ECR_Ignore);

	//---------------------------------------------------------------------------------

	bIsWeapon = true;
	bIsStackable = false;
}

void AThrowableWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	SetState(EThrowableWeaponStateType::Idle);

	if (MuzzlePoint)
	{
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
		MuzzlePoint->AttachToComponent(GetRootComponent(), AttachmentRules);
	}
}

void AThrowableWeapon::InitItem(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor)
{
	Character = NewCharacter;
	if (bWasInitialized) { return; }
	bWasInitialized = true;
	ItemPickUp = PickUpActor;
	LoadItemData();

	if (Character)
	{
		CharacterAnimInstance = Character->GetArmMesh()->GetAnimInstance();
		//InitializeCamera(Character);
		LoadData();
	}   

	CharacterController = Cast<APlayerController>(Character->GetController());
	if (CharacterController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(CharacterController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(InputMappingContext, 1);
		}
	}

	InitUI();

	InitProjectiles(FireData.ProjectileClass, 5);
}

void AThrowableWeapon::InitItemPost(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor)
{
	Character = NewCharacter;
	if (bWasInitialized) { return; }
	bWasInitialized = true;
	ItemPickUp = PickUpActor;
	LoadItemData();

	if (Character)
	{
		CharacterAnimInstance = Character->GetArmMesh()->GetAnimInstance();
		//InitializeCamera(Character);
		LoadDataPost();
	}
	InitUI();

	CharacterController = Cast<APlayerController>(Character->GetController());
	if (CharacterController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(CharacterController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(InputMappingContext, 1);
		}
	}

	InitProjectiles(FireData.ProjectileClass, 5);
}

void AThrowableWeapon::InitCam(ACharacterPlayer* TargetCharacter)
{
}

void AThrowableWeapon::InitUI()
{
	if (!CharacterController)
	{
		CharacterController = Cast<APlayerController>(Character->GetController());
	}
	if (!CharacterController) { return; }
	UPlayerUISubsystem* UISubsystem = CharacterController->GetLocalPlayer()->GetSubsystem<UPlayerUISubsystem>();
	if (!IsValid(UISubsystem)) { return; }
	if (AimUIWidgetClass)
	{
		AimUIWidget = UISubsystem->CreateAndRegisterWidget(AimUIWidgetClass);
	}
	if (AmmoCounterWidgetClass)
	{
		AmmoCounterWidget = UISubsystem->CreateAndRegisterWidget(AmmoCounterWidgetClass);
		if (AmmoCounterWidget)
		{
			//TODO: ThrowableWeapon 전용 AmmoCounter Widget 만들기
			AmmoCounterWidget->UpdateAmmoCount(LeftAmmo);
			AmmoCounterWidget->UpdateTotalAmmo(TotalAmmo);
		}
	}
}

void AThrowableWeapon::LoadData()
{
	if (WeaponDataTable.IsNull() || WeaponRowName.IsNone()) return;
	LoadedWeaponTable = WeaponDataTable.LoadSynchronous();
	if (!LoadedWeaponTable) return;

	FThrowableWeaponData* WeaponData = LoadedWeaponTable->FindRow<FThrowableWeaponData>(WeaponRowName, TEXT("LoadWeaponData"));
	if (!WeaponData) return;
	if (!WeaponData->ThrowableWeaponPDA.IsValid()) return;

	UAssetManager& AM = UAssetManager::Get();

	TSharedPtr<FStreamableHandle> H = AM.LoadPrimaryAsset(WeaponData->ThrowableWeaponPDA);
	if (H.IsValid()) { H->WaitUntilComplete(); }

	auto* Obj = UAssetManager::Get().GetPrimaryAssetObject(WeaponData->ThrowableWeaponPDA);
	UThrowableWeaponPrimaryDataAsset* Def = Cast<UThrowableWeaponPrimaryDataAsset>(Obj);
	if (!Def) return;

	FireData.ProjectileClass = Def->ProjectileClass.LoadSynchronous();

	TArray<FSoftObjectPath> Paths;
	auto Push = [&Paths](const FSoftObjectPath& P) {if (P.IsValid()) Paths.Add(P); };

	// <Effects>
	Push(Def->FireEffect.ToSoftObjectPath());
	Push(Def->ChargeEffect.ToSoftObjectPath());

	// <Sound>
	Push(Def->FireSound.ToSoftObjectPath());
	Push(Def->ChargeSound.ToSoftObjectPath());

	if (Paths.Num() > 0)
	{
		auto& SM = UAssetManager::GetStreamableManager();
		TWeakObjectPtr <AThrowableWeapon > WeakThis(this);

		WeaponAssetsHandle = SM.RequestAsyncLoad(
			Paths,
			FStreamableDelegate::CreateWeakLambda(this, [this, WeakThis, Def]() {
				if (!WeakThis.IsValid()) return;

				FireData.MuzzleFireEffect = Def->FireEffect.Get();
				ChargeEffect = Def->ChargeEffect.Get();
				FireData.FireSound = Def->FireSound.Get();
				ChargeSound = Def->ChargeSound.Get();

				bWeaponAssetsReady = true;
				//UE_LOG(LogTemp, Warning, TEXT("AsyncLoad Weapon Data Complete!"));
				WeaponAssetsHandle.Reset();
				}));
	}
	else
	{
		bWeaponAssetsReady = true;
	}

	//-------------------------------------
	// <WeaponSocket>
	WeaponSocketName = WeaponData->WeaponSocket;
	bIsTwoHandedWeapon = WeaponData->bIsTwoHandedWeapon;

	// <Action>
	LeftMouseAction = WeaponData->LeftMouseAction;
	RightMouseAction = WeaponData->RightMouseAction;

	// <Effect>
	ChargeEffectLocation = WeaponData->ChargeEffectLocation;
	ChargeEffectRotation = WeaponData->ChargeEffectRotation;
	ChargeEffenctScale = WeaponData->ChargeEffenctScale;

	// <Reload>
	ReloadingTime = WeaponData->ReloadingTime;
	MaxAmmo = WeaponData->MaxAmmo;
	FireData.AmmoCost = WeaponData->AmmoConsumedPerShot;

	// <HitScan>
	bIsHitScan = WeaponData->bIsHitScan;

	// <SingleShot>
	SingleShotDelay = WeaponData->SingleShotDelay;

	// <BurstShot>
	BurstShotDelay = WeaponData->BurstShotDelay;
	BurstShotFireRate = WeaponData->BurstShotFireRate;
	BurstShotCount = WeaponData->BurstShotCount;

	// <Animation>
	RightHandSocketTransform_Default = WeaponData->RightHandSocketTransform_Idle;
	RightHandSocketTransform_Crouch = WeaponData->RightHandSocketTransform_Crouch;
	RightHandSocketTransform_Aim_Default = WeaponData->RightHandSocketTransform_Aim_Idle;
	RightHandSocketTransform_Aim_Crouch = WeaponData->RightHandSocketTransform_Aim_Crouch;

	// <Camera Shake>
	ChargingCameraShakeClass = WeaponData->ChargingCameraShakeClass;

	FireData.CamShake = WeaponData->FiringCameraShakeClass;

	// <Charging>
	bAutoFireAtMaxChargeTime = WeaponData->bAutoFireAtMaxChargeTime;
	ChargeTimeThreshold = WeaponData->ChargeTimeThreshold;
	MaxChargeTime = WeaponData->MaxChargeTime;
	ChargingAdditionalDamageBase = WeaponData->ChargingAdditionalDamageBase;

	// <Penetration>
	MaxPenetrableObjectsNum = WeaponData->MaxPenetrableObjectsNum;

	//---------------------------------------

	TotalAmmo = MaxAmmo;
}

void AThrowableWeapon::LoadDataPost()
{
	if (WeaponDataTable.IsNull() || WeaponRowName.IsNone()) return;
	LoadedWeaponTable = WeaponDataTable.LoadSynchronous();
	if (!LoadedWeaponTable) return;

	FThrowableWeaponData* WeaponData = LoadedWeaponTable->FindRow<FThrowableWeaponData>(WeaponRowName, TEXT("LoadWeaponData"));
	if (!WeaponData) return;
	if (!WeaponData->ThrowableWeaponPDA.IsValid()) return;

	UAssetManager& AM = UAssetManager::Get();

	TSharedPtr<FStreamableHandle> H = AM.LoadPrimaryAsset(WeaponData->ThrowableWeaponPDA);
	if (H.IsValid()) { H->WaitUntilComplete(); }

	auto* Obj = UAssetManager::Get().GetPrimaryAssetObject(WeaponData->ThrowableWeaponPDA);
	UThrowableWeaponPrimaryDataAsset* Def = Cast<UThrowableWeaponPrimaryDataAsset>(Obj);
	if (!Def) return;

	FireData.ProjectileClass = Def->ProjectileClass.LoadSynchronous();

	TArray<FSoftObjectPath> Paths;
	auto Push = [&Paths](const FSoftObjectPath& P) {if (P.IsValid()) Paths.Add(P); };

	// <Effects>
	Push(Def->FireEffect.ToSoftObjectPath());
	Push(Def->ChargeEffect.ToSoftObjectPath());

	// <Sound>
	Push(Def->FireSound.ToSoftObjectPath());
	Push(Def->ChargeSound.ToSoftObjectPath());

	if (Paths.Num() > 0)
	{
		auto& SM = UAssetManager::GetStreamableManager();
		TWeakObjectPtr <AThrowableWeapon > WeakThis(this);

		WeaponAssetsHandle = SM.RequestAsyncLoad(
			Paths,
			FStreamableDelegate::CreateWeakLambda(this, [this, WeakThis, Def]() {
				if (!WeakThis.IsValid()) return;

				FireData.MuzzleFireEffect = Def->FireEffect.Get();
				ChargeEffect = Def->ChargeEffect.Get();
				FireData.FireSound = Def->FireSound.Get();
				ChargeSound = Def->ChargeSound.Get();

				bWeaponAssetsReady = true;
				//UE_LOG(LogTemp, Warning, TEXT("AsyncLoad Weapon Data Complete!"));
				WeaponAssetsHandle.Reset();
				}));
	}
	else
	{
		bWeaponAssetsReady = true;
	}

	//-------------------------------------
	// <WeaponSocket>
	WeaponSocketName = WeaponData->WeaponSocket;
	bIsTwoHandedWeapon = WeaponData->bIsTwoHandedWeapon;

	// <Action>
	LeftMouseAction = WeaponData->LeftMouseAction;
	RightMouseAction = WeaponData->RightMouseAction;

	// <Effect>
	ChargeEffectLocation = WeaponData->ChargeEffectLocation;
	ChargeEffectRotation = WeaponData->ChargeEffectRotation;
	ChargeEffenctScale = WeaponData->ChargeEffenctScale;

	// <Reload>
	ReloadingTime = WeaponData->ReloadingTime;
	MaxAmmo = WeaponData->MaxAmmo;
	FireData.AmmoCost = WeaponData->AmmoConsumedPerShot;

	// <HitScan>
	bIsHitScan = WeaponData->bIsHitScan;

	// <SingleShot>
	SingleShotDelay = WeaponData->SingleShotDelay;

	// <BurstShot>
	BurstShotDelay = WeaponData->BurstShotDelay;
	BurstShotFireRate = WeaponData->BurstShotFireRate;
	BurstShotCount = WeaponData->BurstShotCount;

	// <Animation>
	RightHandSocketTransform_Default = WeaponData->RightHandSocketTransform_Idle;
	RightHandSocketTransform_Crouch = WeaponData->RightHandSocketTransform_Crouch;
	RightHandSocketTransform_Aim_Default = WeaponData->RightHandSocketTransform_Aim_Idle;
	RightHandSocketTransform_Aim_Crouch = WeaponData->RightHandSocketTransform_Aim_Crouch;

	// <Camera Shake>
	ChargingCameraShakeClass = WeaponData->ChargingCameraShakeClass;

	FireData.CamShake = WeaponData->FiringCameraShakeClass;

	// <Charging>
	bAutoFireAtMaxChargeTime = WeaponData->bAutoFireAtMaxChargeTime;
	ChargeTimeThreshold = WeaponData->ChargeTimeThreshold;
	MaxChargeTime = WeaponData->MaxChargeTime;
	ChargingAdditionalDamageBase = WeaponData->ChargingAdditionalDamageBase;

	// <Penetration>
	MaxPenetrableObjectsNum = WeaponData->MaxPenetrableObjectsNum;
}

void AThrowableWeapon::OnItemStateRestored()
{
	//TODO: Def
}

bool AThrowableWeapon::AttachItemToPlayer(ACharacterPlayer* TargetCharacter)
{
	Character = TargetCharacter;
	if (Character == nullptr) { return false; }

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);

	if (WeaponSocketName.IsNone())
	{
		AttachToComponent(Character->GetArmMesh(), AttachmentRules, FName(TEXT("Gun")));
	}
	else
	{
		AttachToComponent(Character->GetArmMesh(), AttachmentRules, WeaponSocketName);
	}

	//---------------------------------------------
	// Set Up Widget UI Class
	ActivateAimUIWidget(true);
	ActivateAmmoCounterWidget(true);
	//ActivateTargetingSkillWidget(true);

	return true;
}

void AThrowableWeapon::DetachItemFromPlayer()
{
	if (Character == nullptr) { return; }
	else
	{
		//ActivateCrosshairWidget(false);
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}

void AThrowableWeapon::Equip(ACharacterPlayer* TargetCharacter)
{
	if (CurrentState == EThrowableWeaponStateType::Reloading)
	{
		CancelReload();
	}
	else if (CurrentState == EThrowableWeaponStateType::Firing)
	{
		BurstShotFired = 0;
		GetWorld()->GetTimerManager().ClearTimer(SingleShotTimer);
		GetWorld()->GetTimerManager().ClearTimer(BurstShotTimer);
	}

	SetState(EThrowableWeaponStateType::Switching);

	AttachItemToPlayer(TargetCharacter);
	//GetWorld()->GetTimerManager().SetTimer(SwitchingTimer, [this, TargetCharacter, bEquip]() {EndWeaponSwitch(TargetCharacter, bEquip); }, WeaponSwitchingRate, false);
	TWeakObjectPtr WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(SwitchingTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis, TargetCharacter]()
		{
			if (auto* HardThis = WeakThis.Get())
			{
				HardThis->OnEquipEnded(TargetCharacter);
			}

		}), WeaponSwitchingRate, false);

	StartAnimation(AM_Equip_Character, WeaponSwitchingRate);
}

void AThrowableWeapon::Unequip(ACharacterPlayer* TargetCharacter)
{
	if (CurrentState == EThrowableWeaponStateType::Reloading)
	{
		CancelReload();
	}
	else if (CurrentState == EThrowableWeaponStateType::Firing)
	{
		BurstShotFired = 0;
		GetWorld()->GetTimerManager().ClearTimer(SingleShotTimer);
		GetWorld()->GetTimerManager().ClearTimer(BurstShotTimer);
	}

	SetState(EThrowableWeaponStateType::Switching);

	GetWorld()->GetTimerManager().SetTimer(SwitchingTimer, [this, TargetCharacter]() {OnUnequipEnded(TargetCharacter); }, WeaponSwitchingRate, false);
	StartAnimation(AM_Unequip_Character, WeaponSwitchingRate);

	AM_Unequip_Character->BlendOut.SetBlendTime(1000.f);
	AM_Unequip_Character->bEnableAutoBlendOut = false;
}

void AThrowableWeapon::OnEquipEnded(ACharacterPlayer* TargetCharacter)
{
	if (!TargetCharacter) { return; }

	SetInputActionBinding();
	SetState(EThrowableWeaponStateType::Idle);
}

void AThrowableWeapon::OnUnequipEnded(ACharacterPlayer* TargetCharacter)
{
	if (!TargetCharacter) { return; }

	ResetInputActionBinding();
	DetachWeaponFromPlayer();
	SetState(EThrowableWeaponStateType::Unequiped);
	if (UInventorySystemComponent* InventorySystem = TargetCharacter->GetInventorySystemComponent())
	{
		InventorySystem->SwitchToNextItem();
	}
}

void AThrowableWeapon::DetachWeaponFromPlayer()
{
	if (Character == nullptr)
	{
		return;
	}
	else
	{
		//ActivateCrosshairWidget(false);
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}

void AThrowableWeapon::SetState(EThrowableWeaponStateType NewState)
{
	if (CurrentState == NewState) { return; }
	CurrentState = NewState;
	switch (CurrentState)
	{
	case EThrowableWeaponStateType::Idle:
		break;
	case EThrowableWeaponStateType::Aiming:
		break;
	case EThrowableWeaponStateType::Firing:
		break;
	case EThrowableWeaponStateType::Switching:
		break;
	case EThrowableWeaponStateType::Reloading:
		break;
	case EThrowableWeaponStateType::Unequiped:
		break;
	default:
		break;
	}
}

void AThrowableWeapon::StartAim()
{
	//TODO: Def
}

void AThrowableWeapon::StopAim()
{
	//TODO: Def
}

void AThrowableWeapon::Throw(const FVector& ThrowDirection)
{
	//TODO: Def
}

void AThrowableWeapon::SetInputActionBinding()
{
	if (Character)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
			{
				// <LeftMouseAction>
				if (LeftMouseAction == EWeaponAction::WeaponAction_SingleShot)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						LeftSingleShotAction,
						ETriggerEvent::Started,
						[this](const FInputActionValue& InputActionValue, int32 NumPenetrable)
						{
							HandleSingleFire(NumPenetrable);
						},
						MaxPenetrableObjectsNum
					));
				}
				else if (LeftMouseAction == EWeaponAction::WeaponAction_BurstShot)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						LeftBurstShotAction,
						ETriggerEvent::Started,
						[this](const FInputActionValue& InputActionValue, int32 NumPenetrable)
						{
							HandleBurstFire(NumPenetrable);
						},
						MaxPenetrableObjectsNum
					));
				}
				else if (LeftMouseAction == EWeaponAction::WeaponAction_Charge)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(LeftChargeAction, ETriggerEvent::Triggered, this, &AThrowableWeapon::StartCharge));
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(LeftChargeAction, ETriggerEvent::Completed, this, &AThrowableWeapon::StopCharge));
				}

				// <RightMouseAction>
				if (RightMouseAction == EWeaponAction::WeaponAction_SingleShot)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						RightSingleShotAction,
						ETriggerEvent::Started,
						[this](const FInputActionValue& InputActionValue, int32 NumPenetrable)
						{
							HandleSingleFire(NumPenetrable);
						},
						MaxPenetrableObjectsNum
					));
				}
				else if (RightMouseAction == EWeaponAction::WeaponAction_BurstShot)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						RightBurstShotAction,
						ETriggerEvent::Started,
						[this](const FInputActionValue& InputActionValue, int32 NumPenetrable)
						{
							HandleBurstFire(NumPenetrable);
						},
						MaxPenetrableObjectsNum
					));
				}
				else if (RightMouseAction == EWeaponAction::WeaponAction_Charge)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightChargeAction, ETriggerEvent::Triggered, this, &AThrowableWeapon::StartCharge));
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightChargeAction, ETriggerEvent::Completed, this, &AThrowableWeapon::StopCharge));
				}
			}
		}
	}
}

void AThrowableWeapon::ResetInputActionBinding()
{
	if (Character)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
			{
				for (FInputBindingHandle* bindinghandle : InputActionBindingHandles)
				{
					EnhancedInputComponent->RemoveBinding(*bindinghandle);
				}
				InputActionBindingHandles.Empty();
			}
		}
	}
}

void AThrowableWeapon::StartSingleShot(int32 NumPenetrable, float AdditionalDamage)
{
	if (bIsHitScan) { FireSingleHitScan(&FireData, NumPenetrable, AdditionalDamage); }
	else { FireSingleProjectile(&FireData, NumPenetrable, AdditionalDamage); }

	TWeakObjectPtr WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(SingleShotTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (auto* HardThis = WeakThis.Get())
			{
				HardThis->StopSingleShot();
			}

		}), SingleShotDelay, false);

}

void AThrowableWeapon::StopSingleShot()
{
	SetState(EThrowableWeaponStateType::Idle);
}

void AThrowableWeapon::StartReload()
{
	//TODO: 기존 상태(aiming 등에서 reloading으로 전환될 때 기존 상태에 대한 처리 필요)

	StartAnimation(AM_Reload_Character, ReloadingTime);
	TWeakObjectPtr WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(ReloadingTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (auto* HardThis = WeakThis.Get())
			{
				HardThis->StopReload();
			}
		}), ReloadingTime, false);
}

bool AThrowableWeapon::AddAmmo(int32 NumAmmo)
{
	if (TotalAmmo < MaxAmmo)
	{
		int32 NewTotalAmmo = FMath::Clamp(TotalAmmo + NumAmmo, 0, MaxAmmo);
		TotalAmmo = NewTotalAmmo;
		if (AmmoCounterWidget)
		{
			AmmoCounterWidget->UpdateAmmoCount(LeftAmmo);
			AmmoCounterWidget->UpdateTotalAmmo(TotalAmmo);
		}
		return true;
	}
	return false;
}

void AThrowableWeapon::AutoReload()
{
	if (bCanAutoReload)
	{
		if (!HasAmmo() && TotalAmmo > 0)
		{
			SetState(EThrowableWeaponStateType::Reloading);
		}
	}
}

void AThrowableWeapon::ReloadingEnd()
{
	if (LeftAmmo < MaxAmmo)
	{
		ReloadAmmo();
	}
}

void AThrowableWeapon::InitProjectiles(TSubclassOf<AThrowableWeaponProjectile> ProjectileClass, int32 NumObject)
{
	if (ProjectileClass == nullptr) { return; }
	if (NumObject < 0) { return; }
	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	if (PoolSubsystem == nullptr) { return; }
	PoolSubsystem->PrewarmPool(ProjectileClass, NumObject);
}

//AThrowableWeaponProjectile* AThrowableWeapon::GetProjectileFromPool(TSubclassOf<AThrowableWeaponProjectile> ProjectileClass)
//{
//	//TODO: Def
//}

void AThrowableWeapon::ReturnProjectile(AThrowableWeaponProjectile* Projectile)
{
	//TODO: 여기서 이걸 왜함? 
}

void AThrowableWeapon::ActivateAimUIWidget(bool bFlag)
{
	//TODO: UISubSystem과 통합
	if (bFlag)
	{
		if (AimUIWidget)
		{
			AimUIWidget->AddToViewport();
			AimUIWidget->ResetAimUISize();
		}
	}
	else
	{
		if (AimUIWidget)
		{
			AimUIWidget->RemoveFromViewport();
		}
	}
}

void AThrowableWeapon::ActivateAmmoCounterWidget(bool bFlag)
{
	//TODO: UISubSystem과 통합
	if (bFlag)
	{
		if (AmmoCounterWidget)
		{
			AmmoCounterWidget->AddToViewport();
		}
	}
	else
	{
		if (AmmoCounterWidget)
		{
			AmmoCounterWidget->RemoveFromViewport();
		}
	}
}

void AThrowableWeapon::ApplyCameraShake(TSubclassOf<UWeaponCamShakeBase> CamShakeClass, float Scale)
{
	if (Character && CamShakeClass)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			PlayerController->PlayerCameraManager->StartCameraShake(CamShakeClass, Scale);
		}
	}
}

FTransform AThrowableWeapon::GetMuzzlePointTransform() const
{
	if (MuzzlePoint)
	{
		return MuzzlePoint->GetComponentTransform();
	}
	return FTransform();
}

void AThrowableWeapon::EnterState(EThrowableWeaponStateType NewState)
{
	switch (NewState)
	{
	case EThrowableWeaponStateType::Idle:
		AutoReload();
		break;
	case EThrowableWeaponStateType::Aiming:
		break;
	case EThrowableWeaponStateType::Firing:
		break;
	case EThrowableWeaponStateType::Switching:
		// ZoomOut();
		break;
	case EThrowableWeaponStateType::Reloading:
		StartReload();
		break;
	case EThrowableWeaponStateType::Charging:
		break;
	case EThrowableWeaponStateType::Unequiped:
		//ForceStopCamModification();
		ActivateAimUIWidget(false);
		ActivateAmmoCounterWidget(false);
		break;
	default:
		break;
	}
}

void AThrowableWeapon::ExitState(EThrowableWeaponStateType OldState)
{
	switch (OldState)
	{
	case EThrowableWeaponStateType::Idle:
		break;
	case EThrowableWeaponStateType::Aiming:
		break;
	case EThrowableWeaponStateType::Firing:
		break;
	case EThrowableWeaponStateType::Switching:
		break;
	case EThrowableWeaponStateType::Reloading:
		break;
	case EThrowableWeaponStateType::Charging:
		break;
	case EThrowableWeaponStateType::Unequiped:
		break;
	default:
		break;
	}
}

void AThrowableWeapon::UpdateState(float DeltaTime)
{
	switch (CurrentState)
	{
	case EThrowableWeaponStateType::Idle:
		break;
	case EThrowableWeaponStateType::Aiming:
		break;
	case EThrowableWeaponStateType::Firing:
		break;
	case EThrowableWeaponStateType::Switching:
		break;
	case EThrowableWeaponStateType::Reloading:
		break;
	case EThrowableWeaponStateType::Charging:
		break;
	case EThrowableWeaponStateType::Unequiped:
		break;
	default:
		break;
	}
}

void AThrowableWeapon::UpdateIdle(float DeltaTime)
{
}

void AThrowableWeapon::UpdateAiming(float DeltaTime)
{
}

void AThrowableWeapon::UpdateThrown(float DeltaTime)
{
}

void AThrowableWeapon::UpdateSwitching(float DeltaTime)
{
}

void AThrowableWeapon::OnEnterIdle()
{
}

void AThrowableWeapon::OnEnterAiming()
{
}

void AThrowableWeapon::OnEnterThrown()
{
}

void AThrowableWeapon::StartFuse()
{
}

void AThrowableWeapon::OnFuseExpired()
{
}

void AThrowableWeapon::ActivateThrowable()
{
}

void AThrowableWeapon::ResetThrowable()
{
}

void AThrowableWeapon::FireSingleProjectile(FThrowableWeaponFireParams* InFireData, int32 NumPenetrable, float AdditionalDamage)
{
	if (CurrentState == EThrowableWeaponStateType::Unequiped) return;
	if (!InFireData) return;
	if (!Character) return;
	if (Character->GetController() == nullptr) return;

	const auto* Cam = Character->GetCameraComponent();
	if (!Cam) return;

	if (InFireData->AmmoCost > 0)
	{
		if (!HasAmmo(InFireData->AmmoCost))
		{
			return;
		}
		ConsumeAmmo(InFireData->AmmoCost);
	}

	FVector ProjectileStartLocation = Cam->GetComponentLocation();
	FVector ProjectileDirection = Cam->GetForwardVector();

	if (InFireData->ProjectileClass != nullptr)
	{
		const FVector SpawnLocation = ProjectileStartLocation;
		FVector MuzzleLocation;
		if (MuzzlePoint)
		{
			MuzzleLocation = MuzzlePoint->GetComponentLocation();
		}
		else
		{
			return;
		}

		const FRotator SpawnRotation = ProjectileDirection.Rotation();

		UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
		if (PoolSubsystem == nullptr) { return; }
		AThrowableWeaponProjectile* NewProjectile = nullptr;
		NewProjectile = PoolSubsystem->SpawnFromPool(InFireData->ProjectileClass, SpawnLocation, SpawnRotation);
		NewProjectile->InitProjectile(Character, this, AdditionalDamage, NumPenetrable, false);

		NewProjectile->InitProjectileMovement(ProjectileStartLocation, ProjectileDirection, MuzzleLocation);


		if (bWeaponAssetsReady)
		{
			//TODO: SpawnRotation 정상화
			if (InFireData->MuzzleFireEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					InFireData->MuzzleFireEffect,
					MuzzleLocation,
					SpawnRotation,
					FVector(1.0f),
					true, true, ENCPoolMethod::AutoRelease);
			}
		}
	}

	ReportFiringSoundToAI();
	if (InFireData->FireSound) //TODO: SoundSystem에 통합시키기
	{
		UGameplayStatics::PlaySoundAtLocation(this, InFireData->FireSound, Character->GetActorLocation());
	}

	StartFireAnimation(AM_Fire_Character);

	// <CamShake>
	ApplyCameraShake(InFireData->CamShake);
}

void AThrowableWeapon::FireSingleHitScan(FThrowableWeaponFireParams* InFireData, int32 NumPenetrable, float AdditionalDamage)
{
	if (CurrentState == EThrowableWeaponStateType::Unequiped) return;
	if (!InFireData) return;
	if (!Character) return;
	if (Character->GetController() == nullptr) return;

	const auto* Cam = Character->GetCameraComponent();
	if (!Cam) return;

	if (InFireData->AmmoCost > 0)
	{
		if (!HasAmmo(InFireData->AmmoCost))
		{
			return;
		}
		ConsumeAmmo(InFireData->AmmoCost);
	}

	FVector ProjectileStartLocation = Cam->GetComponentLocation();
	FVector ProjectileDirection = Cam->GetForwardVector();

	if (InFireData->ProjectileClass != nullptr)
	{
		const FVector SpawnLocation = ProjectileStartLocation;
		FVector MuzzleLocation;
		if (MuzzlePoint)
		{
			MuzzleLocation = MuzzlePoint->GetComponentLocation();
		}
		else
		{
			return;
		}
		const FRotator SpawnRotation = ProjectileStartLocation.Rotation();


		UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
		if (PoolSubsystem == nullptr) { return; }
		AThrowableWeaponProjectile* NewProjectile = nullptr;
		NewProjectile = PoolSubsystem->SpawnFromPool(InFireData->ProjectileClass, SpawnLocation, SpawnRotation);
		NewProjectile->InitProjectile(Character, this, AdditionalDamage, NumPenetrable, true);

		NewProjectile->LaunchHitScan(ProjectileStartLocation, ProjectileDirection, MuzzleLocation);

		if (bWeaponAssetsReady)
		{
			//TODO: SpawnRotation 정상화
			if (InFireData->MuzzleFireEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					InFireData->MuzzleFireEffect,
					MuzzleLocation,
					SpawnRotation,
					FVector(1.0f),
					true, true, ENCPoolMethod::AutoRelease);
			}
		}
	}

	ReportFiringSoundToAI();
	if (InFireData != nullptr && InFireData->FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, InFireData->FireSound, Character->GetActorLocation());
	}

	if (AM_Fire_Character)
	{
		StartFireAnimation(AM_Fire_Character);
	}

	// <CamShake>
	ApplyCameraShake(InFireData->CamShake);
}

void AThrowableWeapon::HandleSingleFire(int32 NumPenetrable)
{
	if (CurrentState == EThrowableWeaponStateType::Idle)
	{
		SetState(EThrowableWeaponStateType::Firing);
		StartSingleShot(NumPenetrable);
	}
}

void AThrowableWeapon::HandleBurstFire(int32 NumPenetrable)
{
	if (CurrentState == EThrowableWeaponStateType::Idle)
	{
		SetState(EThrowableWeaponStateType::Firing);
		StartBurstFire(NumPenetrable);
	}
}

void AThrowableWeapon::StartBurstFire(int32 NumPenetrable, float AdditionalDamage)
{
	if (BurstShotFired < BurstShotCount)
	{
		if (bIsHitScan) { FireSingleHitScan(&FireData, NumPenetrable, AdditionalDamage); }
		else { FireSingleProjectile(&FireData, NumPenetrable, AdditionalDamage); }

		BurstShotFired++;	
		TWeakObjectPtr WeakThis = this;
		GetWorld()->GetTimerManager().SetTimer(BurstShotTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis, NumPenetrable]()
			{
				if (auto* HardThis = WeakThis.Get())
				{
					HardThis->StartBurstFire(NumPenetrable);
				}

			}), BurstShotFireRate, true);
	}
	else
	{
		StopBurstFire();
	}
}

void AThrowableWeapon::StopBurstFire()
{
	BurstShotFired = 0;
	if (GetWorld()->GetTimerManager().IsTimerActive(BurstShotTimer))
	{
		GetWorld()->GetTimerManager().ClearTimer(BurstShotTimer);
	}
	TWeakObjectPtr WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(BurstShotTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (auto* HardThis = WeakThis.Get())
			{
				HardThis->EndBurstShot();
			}

		}), BurstShotDelay, false);
}

void AThrowableWeapon::EndBurstShot()
{
	SetState(EThrowableWeaponStateType::Idle);
}

void AThrowableWeapon::StartCharge()
{
	if (CurrentState == EThrowableWeaponStateType::Idle)
	{
		SetState(EThrowableWeaponStateType::Charging);

		SpawnChargeEffect(ChargeEffectLocation, ChargeEffectRotation, ChargeEffenctScale);
		//PlayWeaponSound(ChargeSound); //TODO: SoundSystem에 통합

		UpdateCharge();
	}
}

void AThrowableWeapon::UpdateCharge()
{
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	ElapsedChargeTime += DeltaSeconds;

	float ChargingCamShakeScale = FMath::Clamp((ElapsedChargeTime / MaxChargeTime), 0.1f, 3.f); //TODO: magic number 변수화
	ApplyCameraShake(ChargingCameraShakeClass, ChargingCamShakeScale);

	if (bAutoFireAtMaxChargeTime)
	{
		if (ElapsedChargeTime > MaxChargeTime)
		{
			StopCharge();
		}
		else
		{
			TWeakObjectPtr WeakThis = this;
			GetWorld()->GetTimerManager().SetTimer(ChargingTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
				{
					if (auto* HardThis = WeakThis.Get())
					{
						HardThis->UpdateCharge();
					}

				}), DeltaSeconds, false);
		}
	}
	else
	{
		TWeakObjectPtr WeakThis = this;
		GetWorld()->GetTimerManager().SetTimer(ChargingTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
			{
				if (auto* HardThis = WeakThis.Get())
				{
					HardThis->UpdateCharge();
				}

			}), DeltaSeconds, false);
	}
}

void AThrowableWeapon::StopCharge()
{
	if (CurrentState == EThrowableWeaponStateType::Charging)
	{
		GetWorld()->GetTimerManager().ClearTimer(ChargingTimer);

		DestroyChargeEffect();
		//StopWeaponSound();

		SetState(EThrowableWeaponStateType::Firing);

		float ChargingAdditionalDamage = 0.f;
		int32 PenetrableObjectsNum = 0;

		if (ElapsedChargeTime > ChargeTimeThreshold)
		{
			ChargingAdditionalDamage = ((ElapsedChargeTime - ChargeTimeThreshold) / (MaxChargeTime - ChargeTimeThreshold)) * ChargingAdditionalDamageBase;
			PenetrableObjectsNum = ((ElapsedChargeTime - ChargeTimeThreshold) / (MaxChargeTime - ChargeTimeThreshold)) * MaxPenetrableObjectsNum;
		}

		StartSingleShot(PenetrableObjectsNum, ChargingAdditionalDamage);

		ElapsedChargeTime = 0.f;
	}
}

void AThrowableWeapon::HandleReload()
{
	if (CurrentState == EThrowableWeaponStateType::Idle)
	{
		if (LeftAmmo < MaxAmmo && TotalAmmo > 0)
		{
			SetState(EThrowableWeaponStateType::Reloading);
			ReportReloadingSoundToAI();
		}
	}
}

void AThrowableWeapon::CancelReload()
{
	CancelAnimation(AM_Reload_Character);
	GetWorld()->GetTimerManager().ClearTimer(ReloadingTimer);
}

void AThrowableWeapon::StopReload()
{
	ReloadAmmo();
	SetState(EThrowableWeaponStateType::Idle);
}

void AThrowableWeapon::InterruptReloadAndFire() //TODO: 한번 살펴보기
{
	//if (BufferedFireRequest.IsSet())
	//{
	//	SetState(EThrowableWeaponStateType::Idle);

	//	const FBufferedFireRequest& Request = BufferedFireRequest.GetValue();
	//	if (Request.ActionName == EWeaponAction::WeaponAction_SingleShot)
	//	{
	//		HandleSingleFire(Request.bIsLeftInput, Request.bSingleProjectile, Request.NumPenetrable);
	//		BufferedFireRequest.Reset();
	//		//UE_LOG(LogTemp, Log, TEXT("Buffered fire executed after reload"));
	//	}
	//	else
	//	{
	//		HandleBurstFire(Request.bIsLeftInput, Request.bSingleProjectile, Request.NumPenetrable);
	//		BufferedFireRequest.Reset();
	//		//UE_LOG(LogTemp, Log, TEXT("Buffered fire executed after reload"));
	//	}
	//}
	//else if (bFireInputDuringReload)
	//{
	//	bFireInputDuringReload = false;
	//	SetState(IdleState);
	//}
}

void AThrowableWeapon::ConsumeAmmo(int32 AmmoCost)
{
	if (LeftAmmo > 0)
	{
		if (LeftAmmo >= AmmoCost)
		{
			LeftAmmo -= AmmoCost;
		}
		else
		{
			LeftAmmo = 0;
		}

		//TODO: PlayerUISubsystem에 편입
		if (AmmoCounterWidget)
		{
			AmmoCounterWidget->UpdateAmmoCount(LeftAmmo);
		}
	}
}

void AThrowableWeapon::ReloadAmmo() //TODO: 한 개씩 reload 되도록 변경해야함.
{
	LeftAmmo += 1;
	TotalAmmo -= 1;

	if (AmmoCounterWidget)
	{
		AmmoCounterWidget->UpdateAmmoCount(LeftAmmo);
		AmmoCounterWidget->UpdateTotalAmmo(TotalAmmo);
	}
}

bool AThrowableWeapon::HasAmmo()
{
	return LeftAmmo > 0;
}

bool AThrowableWeapon::HasAmmo(int32 AmmoCost)
{
	return LeftAmmo >= AmmoCost;
}

void AThrowableWeapon::StartFireAnimation(UAnimMontage* CharacterFireAnimation)
{
	if (CharacterAnimInstance != nullptr)
	{
		if (!CharacterAnimInstance->Montage_IsPlaying(CharacterFireAnimation))
		{
			CharacterAnimInstance->Montage_Play(CharacterFireAnimation, 0.5f);
		}
	}
}

void AThrowableWeapon::StartAnimation(UAnimMontage* CharacterAnimation, float CharacterAnimPlayRate, FName StartSection)
{
	if (CharacterAnimInstance != nullptr && CharacterAnimation != nullptr)
	{
		if (!CharacterAnimInstance->Montage_IsPlaying(CharacterAnimation))
		{
			//TODO: Blend
			//FMontageBlendSettings BlendSettings;
			//BlendSettings.Blend.BlendTime = 0.1f;

			CharacterAnimation->BlendIn.SetBlendOption(EAlphaBlendOption::Linear);
			CharacterAnimation->BlendIn.SetAlpha(10.f);
			CharacterAnimation->BlendOut.SetBlendOption(EAlphaBlendOption::Linear);
			CharacterAnimation->BlendOut.SetAlpha(10.f);

			CharacterAnimInstance->Montage_Play(CharacterAnimation, CharacterAnimation->GetPlayLength() / CharacterAnimPlayRate);
			if (!StartSection.IsNone())
			{
				CharacterAnimInstance->Montage_JumpToSection(StartSection, CharacterAnimation);
			}
		}
	}
}

void AThrowableWeapon::CancelAnimation(UAnimMontage* CharacterAnimation)
{
	if (CharacterAnimInstance != nullptr)
	{
		if (CharacterAnimInstance->Montage_IsPlaying(CharacterAnimation))
		{
			CharacterAnimInstance->Montage_Stop(0.f, CharacterAnimation);
		}
	}
}

void AThrowableWeapon::SpawnChargeEffect(FVector SpawnLocation, FRotator SpawnRotation, FVector EffectScale)
{
	if (ChargeEffect)
	{
		ChargeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			ChargeEffect,
			MuzzlePoint,
			FName(TEXT("Muzzle")),
			SpawnLocation,
			FRotator(0, 0, 0),
			EAttachLocation::KeepRelativeOffset,
			true);

		ChargeEffectComponent->SetRelativeScale3D(EffectScale);

		//TODO: We need to find a way to fix the speed. This method doesn't work.
		ChargeEffectComponent->SetSeekDelta(20.f / MaxChargeTime);
	}
}
void AThrowableWeapon::DestroyChargeEffect()
{
	if (ChargeEffectComponent)
	{
		ChargeEffectComponent->Deactivate();
		ChargeEffectComponent->DestroyComponent();
		ChargeEffectComponent = nullptr;
	}
}

void AThrowableWeapon::ReportFiringSoundToAI()
{
	//UAISense_Hearing::ReportNoiseEvent(
	//	GetWorld(),
	//	GetActorLocation(),
	//	FiringSoundLoudness,
	//	this,
	//	FiringSoundRange,
	//	TEXT("Firing")
	//);
}


void AThrowableWeapon::ReportReloadingSoundToAI()
{
	//UAISense_Hearing::ReportNoiseEvent(
	//	GetWorld(),
	//	GetActorLocation(),
	//	ReloadingSoundLoudness,
	//	this,
	//	ReloadingSoundRange,
	//	TEXT("Reloading")
	//);
}