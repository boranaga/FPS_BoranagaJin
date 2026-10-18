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
		AimUIWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	if (AmmoCounterWidgetClass)
	{
		AmmoCounterWidget = UISubsystem->CreateAndRegisterWidget(AmmoCounterWidgetClass);
		if (AmmoCounterWidget)
		{
			//TODO: ThrowableWeapon 전용 AmmoCounter Widget 만들기
			AmmoCounterWidget->UpdateAmmoCount(LeftAmmo);
			AmmoCounterWidget->UpdateTotalAmmo(TotalAmmo);
			AmmoCounterWidget->SetVisibility(ESlateVisibility::Hidden);
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
	UE_LOG(LogTemp, Error, TEXT("void AThrowableWeapon::Equip(ACharacterPlayer* TargetCharacter)"));

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

	//---------------------------------------------
	// Set Up Widget UI Class
	ActivateAimUIWidget(false);
	ActivateAmmoCounterWidget(true);

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
	//if (bThrowInProgress)
	//{
	//	CancelPendingThrow();
	//}
	
	//---------------------------------

	if (CurrentState == EThrowableWeaponStateType::Reloading)
	{
		CancelReload();
	}
	else if (CurrentState == EThrowableWeaponStateType::Firing)
	{
		CancelPendingThrow();

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
	UE_LOG(LogTemp, Error, TEXT("void AThrowableWeapon::OnEquipEnded(ACharacterPlayer* TargetCharacter) 1"));

	if (!TargetCharacter) { return; }

	SetInputActionBinding();
	SetState(EThrowableWeaponStateType::Idle);

	UE_LOG(LogTemp, Error, TEXT("void AThrowableWeapon::OnEquipEnded(ACharacterPlayer* TargetCharacter) 2"));
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
	ExitState(CurrentState);
	CurrentState = NewState;
	EnterState(CurrentState);
}

void AThrowableWeapon::StartAim()
{
	//TODO: Def
}

void AThrowableWeapon::StopAim()
{
	//TODO: Def
}

void AThrowableWeapon::Throw(const FVector& BaseThrowDirection)
{
	//if (CurrentState == EThrowableWeaponStateType::Firing)
	//{
	//	return;
	//}


	if (BaseThrowDirection.IsNearlyZero())
	{
		SetState(EThrowableWeaponStateType::Idle);
		return;
	}

	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		SetState(EThrowableWeaponStateType::Idle);
		return;
	}

	//bThrowInProgress = true;

	// Throw 시작 당시 모드를 확정
	PendingFireMode = CurrentFireMode;

	// Throw 시작 당시 카메라 방향을 기준으로
	// 실제 발사 방향까지 확정
	PendingThrowDirection = CalculateThrowDirection(BaseThrowDirection, PendingFireMode);

	// Throw animation
	PlayThrowAnimation();

	const float ActualReleaseDelay = FMath::Clamp(ThrowReleaseDelay, 0.f, ThrowAnimationDuration);

	if (ActualReleaseDelay <= KINDA_SMALL_NUMBER)
	{
		ExecuteThrow();
	}
	else
	{
		World->GetTimerManager().SetTimer(
			ThrowReleaseTimerHandle,
			this,
			&AThrowableWeapon::ExecuteThrow,
			ActualReleaseDelay,
			false
		);
	}

	if (ThrowAnimationDuration <= KINDA_SMALL_NUMBER)
	{
		FinishThrowSequence();
	}
	else
	{
		World->GetTimerManager().SetTimer(
			ThrowSequenceTimerHandle,
			this,
			&AThrowableWeapon::FinishThrowSequence,
			ThrowAnimationDuration,
			false
		);
	}

	UE_LOG(LogTemp, Error, TEXT("void AThrowableWeapon::Throw(const FVector& BaseThrowDirection)"));
}

void AThrowableWeapon::ToggleFireMode()
{
	if (CurrentState == EThrowableWeaponStateType::Firing)
	{
		return;
	}

	if (CurrentState == EThrowableWeaponStateType::Switching ||
		CurrentState == EThrowableWeaponStateType::Reloading ||
		CurrentState == EThrowableWeaponStateType::Unequiped)
	{
		return;
	}

	switch (CurrentFireMode)
	{
	case EThrowableWeaponFireMode::Upward:
		SetFireMode(EThrowableWeaponFireMode::Downward);

		UE_LOG(LogTemp, Error, TEXT("void AThrowableWeapon::ToggleFireMode() -> Downward"));
		break;

	case EThrowableWeaponFireMode::Downward:
		SetFireMode(EThrowableWeaponFireMode::Upward);

		UE_LOG(LogTemp, Error, TEXT("void AThrowableWeapon::ToggleFireMode() -> Upward"));
		break;

	default:
		SetFireMode(EThrowableWeaponFireMode::Upward);
		break;
	}
}

void AThrowableWeapon::SetFireMode(EThrowableWeaponFireMode NewFireMode)
{
	if (NewFireMode == EThrowableWeaponFireMode::None)
	{
		return;
	}

	if (CurrentFireMode == NewFireMode)
	{
		return;
	}

	CurrentFireMode = NewFireMode;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Throwable FireMode changed: %s"),
		*UEnum::GetValueAsString(CurrentFireMode)
	);
}

void AThrowableWeapon::ExecuteThrow()
{
	//if (!bThrowInProgress)
	//{
	//	return;
	//}

	if (PendingFireMode == EThrowableWeaponFireMode::None)
	{
		return;
	}

	const float ThrowSpeed = GetThrowStrength(PendingFireMode);

	FireSingleProjectile(&FireData, PendingThrowDirection, ThrowSpeed);
}

void AThrowableWeapon::FinishThrowSequence()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			ThrowReleaseTimerHandle
		);

		World->GetTimerManager().ClearTimer(
			ThrowSequenceTimerHandle
		);
	}

	//bThrowInProgress = false;

	PendingThrowDirection = FVector::ForwardVector;
	PendingFireMode = EThrowableWeaponFireMode::None;

	if (CurrentState == EThrowableWeaponStateType::Firing)
	{
		SetState(EThrowableWeaponStateType::Idle);
	}
}

void AThrowableWeapon::PlayThrowAnimation()
{
	if (!IsValid(CharacterAnimInstance))
	{
		return;
	}

	UAnimMontage* ThrowMontage =
		GetThrowMontage();

	if (!IsValid(ThrowMontage))
	{
		return;
	}

	const float PlayRate =
		CalculateThrowAnimationPlayRate(
			ThrowMontage
		);

	CharacterAnimInstance->Montage_Play(
		ThrowMontage,
		PlayRate
	);
}

void AThrowableWeapon::CancelPendingThrow()
{
	UWorld* World = GetWorld();

	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(
			ThrowReleaseTimerHandle
		);

		World->GetTimerManager().ClearTimer(
			ThrowSequenceTimerHandle
		);
	}

	UAnimMontage* ThrowMontage =
		GetThrowMontage();

	if (IsValid(CharacterAnimInstance) &&
		IsValid(ThrowMontage))
	{
		CharacterAnimInstance->Montage_Stop(
			0.1f,
			ThrowMontage
		);
	}

	//bThrowInProgress = false;

	PendingThrowDirection =
		FVector::ForwardVector;

	PendingFireMode =
		EThrowableWeaponFireMode::None;
}

UAnimMontage* AThrowableWeapon::GetThrowMontage() const
{
	switch (PendingFireMode)
	{
	case EThrowableWeaponFireMode::Upward:
		return AM_Throw_Up_Character;

	case EThrowableWeaponFireMode::Downward:
		return AM_Throw_Down_Character;

	default:
		return nullptr;
	}
}

float AThrowableWeapon::CalculateThrowAnimationPlayRate(
	UAnimMontage* Montage) const
{
	if (!IsValid(Montage))
	{
		return 1.f;
	}

	if (ThrowAnimationDuration <= KINDA_SMALL_NUMBER)
	{
		return 1.f;
	}

	const float OriginalDuration =
		Montage->GetPlayLength();

	if (OriginalDuration <= KINDA_SMALL_NUMBER)
	{
		return 1.f;
	}

	return
		OriginalDuration /
		ThrowAnimationDuration;
}

FVector AThrowableWeapon::CalculateThrowDirection(const FVector& BaseDirection, EThrowableWeaponFireMode FireMode) const
{
	if (BaseDirection.IsNearlyZero())
	{
		return FVector::ForwardVector;
	}

	FRotator ThrowRotation =
		BaseDirection.GetSafeNormal().Rotation();

	switch (FireMode)
	{
	case EThrowableWeaponFireMode::Upward:

		ThrowRotation.Pitch += UpwardPitchOffset;
		break;

	case EThrowableWeaponFireMode::Downward:

		ThrowRotation.Pitch += DownwardPitchOffset;
		break;

	default:

		break;
	}

	return ThrowRotation.Vector().GetSafeNormal();
}

float AThrowableWeapon::GetThrowStrength(EThrowableWeaponFireMode FireMode) const
{
	switch (FireMode)
	{
	case EThrowableWeaponFireMode::Upward:
		return UpwardThrowStrength;

	case EThrowableWeaponFireMode::Downward:
		return DownwardThrowStrength;

	default:
		return UpwardThrowStrength;
	}
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
				else if (LeftMouseAction == EWeaponAction::WeaponAction_Hold)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(LeftHoldAction, ETriggerEvent::Started, this, &AThrowableWeapon::PrepareToFire));
				}
				else if (LeftMouseAction == EWeaponAction::WeaponAction_SwitchFireMode)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(LeftSwitchFireModeAction, ETriggerEvent::Started, this, &AThrowableWeapon::ToggleFireMode));
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
				else if (RightMouseAction == EWeaponAction::WeaponAction_Hold)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightHoldAction, ETriggerEvent::Started, this, &AThrowableWeapon::PrepareToFire));
				}
				else if (RightMouseAction == EWeaponAction::WeaponAction_SwitchFireMode)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightSwitchFireModeAction, ETriggerEvent::Started, this, &AThrowableWeapon::ToggleFireMode));
				}

				// <R_SwitchFireModeAction>
				if (R_SwitchFireModeAction == EWeaponAction::WeaponAction_SwitchFireMode)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(SwitchFireModeAction, ETriggerEvent::Started, this, &AThrowableWeapon::ToggleFireMode));
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
			UE_LOG(LogTemp, Error, TEXT("void AThrowableWeapon::ActivateAimUIWidget(bool bFlag): AimUIWidget"));

			AimUIWidget->SetVisibility(ESlateVisibility::Visible);
			AimUIWidget->ResetAimUISize();
		}
	}
	else
	{
		if (AimUIWidget)
		{
			AimUIWidget->SetVisibility(ESlateVisibility::Hidden);
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
			AmmoCounterWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else
	{
		if (AmmoCounterWidget)
		{
			AmmoCounterWidget->SetVisibility(ESlateVisibility::Hidden);
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
		if (AimUIWidget)
		{
			AimUIWidget->SetVisibility(ESlateVisibility::Hidden);
		}
		AutoReload();
		break;
	case EThrowableWeaponStateType::Aiming:
		if (AimUIWidget)
		{
			AimUIWidget->SetVisibility(ESlateVisibility::Visible);
		}
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
	//// <Old>

	//if (CurrentState == EThrowableWeaponStateType::Unequiped) return;
	//if (!InFireData) return;
	//if (!Character) return;
	//if (Character->GetController() == nullptr) return;

	//const auto* Cam = Character->GetCameraComponent();
	//if (!Cam) return;

	//if (InFireData->AmmoCost > 0)
	//{
	//	if (!HasAmmo(InFireData->AmmoCost))
	//	{
	//		return;
	//	}
	//	ConsumeAmmo(InFireData->AmmoCost);
	//}

	//FVector ProjectileStartLocation = Cam->GetComponentLocation();
	//FVector ProjectileDirection = Cam->GetForwardVector();

	//if (InFireData->ProjectileClass != nullptr)
	//{
	//	const FVector SpawnLocation = ProjectileStartLocation;
	//	FVector MuzzleLocation;
	//	if (MuzzlePoint)
	//	{
	//		MuzzleLocation = MuzzlePoint->GetComponentLocation();
	//	}
	//	else
	//	{
	//		return;
	//	}

	//	const FRotator SpawnRotation = ProjectileDirection.Rotation();

	//	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	//	if (PoolSubsystem == nullptr) { return; }
	//	AThrowableWeaponProjectile* NewProjectile = nullptr;
	//	NewProjectile = PoolSubsystem->SpawnFromPool(InFireData->ProjectileClass, SpawnLocation, SpawnRotation);
	//	NewProjectile->InitProjectile(Character, this, AdditionalDamage, NumPenetrable, false);

	//	NewProjectile->InitProjectileMovement(ProjectileStartLocation, ProjectileDirection, MuzzleLocation);


	//	if (bWeaponAssetsReady)
	//	{
	//		//TODO: SpawnRotation 정상화
	//		if (InFireData->MuzzleFireEffect)
	//		{
	//			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
	//				GetWorld(),
	//				InFireData->MuzzleFireEffect,
	//				MuzzleLocation,
	//				SpawnRotation,
	//				FVector(1.0f),
	//				true, true, ENCPoolMethod::AutoRelease);
	//		}
	//	}
	//}

	//ReportFiringSoundToAI();
	//if (InFireData->FireSound) //TODO: SoundSystem에 통합시키기
	//{
	//	UGameplayStatics::PlaySoundAtLocation(this, InFireData->FireSound, Character->GetActorLocation());
	//}

	//StartFireAnimation(AM_Throw_Up_Character);

	//// <CamShake>
	//ApplyCameraShake(InFireData->CamShake);
	 
	//------------------------------------------------------

	// <New>

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

	StartFireAnimation(AM_Throw_Up_Character);

	// <CamShake>
	ApplyCameraShake(InFireData->CamShake);




}

void AThrowableWeapon::FireSingleProjectile(FThrowableWeaponFireParams* InFireData, const FVector& ThrowDirection, float ThrowSpeed, int32 NumPenetrable, float AdditionalDamage)
{
	if (!InFireData)
	{
		return;
	}

	if (!IsValid(Character))
	{
		return;
	}

	if (!InFireData->ProjectileClass)
	{
		return;
	}

	if (!IsValid(MuzzlePoint))
	{
		return;
	}

	if (InFireData->AmmoCost > 0)
	{
		if (!HasAmmo(InFireData->AmmoCost))
		{
			return;
		}

		ConsumeAmmo(InFireData->AmmoCost);
	}

	UWorld* World = GetWorld();
	if (!IsValid(World)) { return; }

	UObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<UObjectPoolSubsystem>();
	if (!IsValid(PoolSubsystem)) { return; }

	const FVector SpawnLocation = MuzzlePoint->GetComponentLocation();

	const FVector NormalizedDirection = ThrowDirection.GetSafeNormal();

	const FRotator SpawnRotation = NormalizedDirection.Rotation();

	AThrowableWeaponProjectile* NewProjectile =
		PoolSubsystem->SpawnFromPool(
			InFireData->ProjectileClass,
			SpawnLocation,
			SpawnRotation
		);

	if (!IsValid(NewProjectile)) { return; }

	NewProjectile->InitProjectile(Character, this, AdditionalDamage, NumPenetrable, false);
	NewProjectile->LoadProjectileData(); // TEMP

	NewProjectile->LaunchProjectile(SpawnLocation, NormalizedDirection, ThrowSpeed);

	//NewProjectile->InitProjectileMovement(SpawnLocation, NormalizedDirection, SpawnLocation);

	// ---------------------------------------------------------
	// Effect
	// ---------------------------------------------------------

	if (bWeaponAssetsReady &&
		InFireData->MuzzleFireEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			InFireData->MuzzleFireEffect,
			SpawnLocation,
			SpawnRotation,
			FVector::OneVector,
			true,
			true,
			ENCPoolMethod::AutoRelease
		);
	}

	// ---------------------------------------------------------
	// Sound
	// ---------------------------------------------------------

	ReportFiringSoundToAI();

	if (InFireData->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			InFireData->FireSound,
			SpawnLocation
		);
	}

	// ---------------------------------------------------------
	// Camera
	// ---------------------------------------------------------

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

	if (AM_Throw_Up_Character)
	{
		StartFireAnimation(AM_Throw_Up_Character);
	}

	// <CamShake>
	ApplyCameraShake(InFireData->CamShake);
}

void AThrowableWeapon::HandleSingleFire(int32 NumPenetrable)
{
	if (CurrentState == EThrowableWeaponStateType::Aiming)
	{
		// <Old>
		//SetState(EThrowableWeaponStateType::Firing);
		//StartSingleShot(NumPenetrable);


		UE_LOG(LogTemp, Error, TEXT("void AThrowableWeapon::HandleSingleFire(int32 NumPenetrable)"));

		//--------------------------

		//if (CurrentState == EThrowableWeaponStateType::Firing)
		//{
		//	return;
		//}

		//if (CurrentState != EThrowableWeaponStateType::Idle &&
		//	CurrentState != EThrowableWeaponStateType::Aiming)
		//{
		//	return;
		//}

		if (!IsValid(Character)) { return; }

		if (!HasAmmo(FireData.AmmoCost))
		{
			return;
		}

		UCameraComponent* Camera = Character->GetCameraComponent();
		if (!IsValid(Camera)) { return; }

		SetState(EThrowableWeaponStateType::Firing);

		const FVector BaseThrowDirection = Camera->GetForwardVector();

		Throw(BaseThrowDirection);
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

void AThrowableWeapon::PrepareToFire()
{
	if (CurrentState == EThrowableWeaponStateType::Idle)
	{
		UE_LOG(LogTemp, Error, TEXT("void AThrowableWeapon::PrepareToFire() -> Aim"));

		SetState(EThrowableWeaponStateType::Aiming);
	}
	else if (CurrentState == EThrowableWeaponStateType::Aiming)
	{
		UE_LOG(LogTemp, Error, TEXT("void AThrowableWeapon::PrepareToFire() -> Idle"));

		SetState(EThrowableWeaponStateType::Idle);
	}
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