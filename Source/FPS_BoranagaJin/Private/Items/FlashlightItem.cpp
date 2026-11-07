#include "Items/FlashlightItem.h"
#include "Items/BatteryItem.h"
#include "Items/InventorySystemComponent.h"
#include "Characters/Player/CharacterPlayer.h"
#include "Characters/Player/PlayerCameraComponent.h"

#include "Components/SpotLightComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

AFlashlightItem::AFlashlightItem()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(RootComponent, FName(TEXT("ItemMesh")));
	ItemMesh->SetVisibility(false);

	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetCollisionObjectType(ECC_GameTraceChannel5); //Weapon
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	FlashlightLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("FlashlightLight"));

	ItemName = EItemName::ItemName_FlashLight;

	if (ItemMesh)
	{
		FlashlightLight->SetupAttachment(ItemMesh);
	}
	else
	{
		FlashlightLight->SetupAttachment(RootComponent);
	}

	FlashlightLight->SetVisibility(false);
	FlashlightLight->SetIntensity(5000.0f);
	FlashlightLight->SetAttenuationRadius(2000.0f);
	FlashlightLight->SetInnerConeAngle(12.0f);
	FlashlightLight->SetOuterConeAngle(25.0f);
	FlashlightLight->SetCastShadows(true);

	CurrentBatteryCharge = MaxBatteryCharge;
	CurrentDurability = MaxDurability;
}

void AFlashlightItem::BeginPlay()
{
	Super::BeginPlay();
	CurrentBatteryCharge = FMath::Clamp(CurrentBatteryCharge, 0.0f, MaxBatteryCharge);
	CurrentDurability = FMath::Clamp(CurrentDurability, 0.0f, MaxDurability);
	SetLightEnabled(false);
}

void AFlashlightItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetInputActionBinding();
	StopZoom();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BatteryDrainTimerHandle);
		World->GetTimerManager().ClearTimer(FlickerTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AFlashlightItem::InitItem(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor)
{
	//Super::InitItem(NewCharacter, PickUpActor);

	Character = NewCharacter;
	if (bWasInitialized) { return; }
	bWasInitialized = true;
	ItemPickUp = PickUpActor;
	LoadItemData();

	if (Character)
	{
		CharacterAnimInstance = Character->GetArmMesh()->GetAnimInstance();
	}

	//SetAimSocketRelativeTransform();
}

bool AFlashlightItem::UseItem(ACharacterPlayer* UsingCharacter)
{
	if (!IsValid(UsingCharacter))
	{
		return false;
	}

	// 손전등 전원 조작은 장착 상태에서만 허용한다.
	if (!bIsEquipped)
	{
		return false;
	}

	if (Character != UsingCharacter)
	{
		Character = UsingCharacter;
	}

	ToggleFlashlight();
	return true;
}

void AFlashlightItem::Equip(ACharacterPlayer* TargetCharacter)
{
	AttachItemToPlayer(TargetCharacter);
	TWeakObjectPtr WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(SwitchingTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis, TargetCharacter]()
		{
			if (auto* HardThis = WeakThis.Get())
			{
				HardThis->OnEquipEnded(TargetCharacter);
			}

		}), SwitchingRate, false);

	StartAnimation(AM_Equip_Character, nullptr, SwitchingRate);
}

void AFlashlightItem::Unequip(ACharacterPlayer* TargetCharacter)
{
	// Unequip 시작과 동시에 손전등 전용 입력 및 Zoom을 해제한다.
	bIsEquipped = false;
	ResetInputActionBinding();
	StopZoom();

	// 전원 OFF 자체는 Unequip을 뜻하지 않지만,
	// 실제 Unequip 시에는 빛을 끄는 정책을 유지한다.
	TurnOffFlashlight();

	TWeakObjectPtr WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(SwitchingTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (auto* HardThis = WeakThis.Get())
			{
				HardThis->OnUnequipEnded();
			}

		}), SwitchingRate, false);


	StartAnimation(AM_Unequip_Character, nullptr, SwitchingRate);

	if (IsValid(AM_Unequip_Character))
	{
		AM_Unequip_Character->BlendOut.SetBlendTime(1000.f);
		AM_Unequip_Character->bEnableAutoBlendOut = false;
	}
}

void AFlashlightItem::OnEquipEnded(ACharacterPlayer* UsingCharacter)
{
	if (!IsValid(UsingCharacter)) { return; }

	Character = UsingCharacter;
	bIsEquipped = true;

	// 장착과 전원 상태는 서로 독립적이다.
	// 장착 완료 시 자동으로 켜지 않고, 좌클릭에서만 ToggleFlashlight()를 호출한다.
	SetInputActionBinding();
}

void AFlashlightItem::OnUnequipEnded()
{
	DetachFromPlayer();

	if (UInventorySystemComponent* WeaponSystem = Cast<ACharacterPlayer>(Character)->GetInventorySystemComponent())
	{
		WeaponSystem->SwitchToNextItem();
	}
}

bool AFlashlightItem::AttachItemToPlayer(ACharacterPlayer* TargetCharacter)
{
	Character = TargetCharacter;
	if (Character == nullptr) { return false; }

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);

	AttachToComponent(Character->GetArmMesh(), AttachmentRules, FName(TEXT("FlashLight")));

	ItemMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	TArray<USceneComponent*> AttachedChildren;
	ItemMesh->GetChildrenComponents(true, AttachedChildren);
	for (auto Child : AttachedChildren)
	{
		if (UPrimitiveComponent* PrimitiveChild = Cast<UPrimitiveComponent>(Child))
		{
			PrimitiveChild->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
		}
	}

	//---------------------------------------------
	//RightHandToAimSocketOffset = WeaponMesh->GetSocketLocation(FName(TEXT("Aim"))) - Character->GetArmMesh()->GetBoneLocation(FName(TEXT("hand_r")));
	//if (Character->GetInventorySystemComponent() && Character->GetInventorySystemComponent()->GetClass()->ImplementsInterface(UWeaponInterface::StaticClass()))
	//{
	//	Character->GetInventorySystemComponent()->SetRightHandToAimSocketOffset(RightHandToAimSocketOffset);
	//}

	ItemMesh->SetVisibility(true);

	// 장착만 수행한다. 전원 ON/OFF는 좌클릭 UseItem()에서 별도로 처리한다.
	SetLightEnabled(false);

	return true;
}

void AFlashlightItem::DetachFromPlayer()
{
	if (Character == nullptr)
	{
		return;
	}
	else
	{
		ItemMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::None;
		TArray<USceneComponent*> AttachedChildren;
		ItemMesh->GetChildrenComponents(true, AttachedChildren);
		for (auto Child : AttachedChildren)
		{
			if (UPrimitiveComponent* PrimitiveChild = Cast<UPrimitiveComponent>(Child))
			{
				PrimitiveChild->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::None;
			}
		}

		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		ItemMesh->SetVisibility(false);
	}
}

void AFlashlightItem::StartAnimation(UAnimMontage* CharacterAnimation, UAnimMontage* WeaponAnimation, float CharacterAnimPlayRate, FName StartSection)
{
	if (CharacterAnimInstance != nullptr && CharacterAnimation != nullptr)
	{
		if (!CharacterAnimInstance->Montage_IsPlaying(CharacterAnimation))
		{
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



#pragma region Input
void AFlashlightItem::SetInputActionBinding()
{
	if (!bIsEquipped || !IsValid(Character))
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (!IsValid(PlayerController))
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent);
	if (!IsValid(EnhancedInputComponent))
	{
		return;
	}

	// Equip이 반복되어도 동일 입력이 중복으로 바인딩되지 않도록 한다.
	ResetInputActionBinding();

	if (IsValid(LeftMouseAction))
	{
		FEnhancedInputActionEventBinding& Binding = EnhancedInputComponent->BindAction(
			LeftMouseAction,
			ETriggerEvent::Started,
			this,
			&AFlashlightItem::HandleLeftMouseInput
		);

		InputActionBindingHandles.Add(Binding.GetHandle());
	}

	if (IsValid(RightMouseAction))
	{
		FEnhancedInputActionEventBinding& Binding = EnhancedInputComponent->BindAction(
			RightMouseAction,
			ETriggerEvent::Started,
			this,
			&AFlashlightItem::HandleRightMouseInput
		);

		InputActionBindingHandles.Add(Binding.GetHandle());
	}
}

void AFlashlightItem::ResetInputActionBinding()
{
	if (InputActionBindingHandles.IsEmpty())
	{
		return;
	}

	if (!IsValid(Character))
	{
		InputActionBindingHandles.Empty();
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (!IsValid(PlayerController))
	{
		InputActionBindingHandles.Empty();
		return;
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent);
	if (!IsValid(EnhancedInputComponent))
	{
		InputActionBindingHandles.Empty();
		return;
	}

	for (const uint32 BindingHandle : InputActionBindingHandles)
	{
		EnhancedInputComponent->RemoveBindingByHandle(BindingHandle);
	}

	InputActionBindingHandles.Empty();
}

void AFlashlightItem::HandleLeftMouseInput()
{
	if (!bIsEquipped || !IsValid(Character))
	{
		return;
	}

	// 좌클릭: 전원 ON/OFF만 변경.
	// OFF 상태가 되어도 bIsEquipped에는 영향을 주지 않는다.
	UseItem(Character);
}

void AFlashlightItem::HandleRightMouseInput()
{
	if (!bIsEquipped)
	{
		return;
	}

	// 우클릭: Zoom ON/OFF 토글.
	ToggleZoom();
}
#pragma endregion

#pragma region Zoom
void AFlashlightItem::ToggleZoom()
{
	if (!bIsEquipped)
	{
		return;
	}

	if (bIsZoomed)
	{
		StopZoom();
	}
	else
	{
		StartZoom();
	}
}

bool AFlashlightItem::StartZoom()
{
	if (!bIsEquipped || !IsValid(Character))
	{
		return false;
	}

	if (bIsZoomed)
	{
		return true;
	}

	UPlayerCameraComponent* PlayerCameraComponent = GetPlayerCameraComponent();
	if (!IsValid(PlayerCameraComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: PlayerCameraComponent is invalid. Zoom request failed."), *GetName());
		return false;
	}

	FPlayerCameraZoomRequest ZoomRequest;
	ZoomRequest.TargetFOV = ZoomTargetFOV;
	ZoomRequest.BlendInSpeed = ZoomBlendInSpeed;
	ZoomRequest.BlendOutSpeed = ZoomBlendOutSpeed;
	ZoomRequest.Priority = ZoomPriority;

	ZoomRequestHandle = PlayerCameraComponent->RequestZoom(ZoomRequest);
	if (!ZoomRequestHandle.IsValid())
	{
		return false;
	}

	// 이 값이 true가 되는 순간 GetRightHandSocketTransform()도
	// RightHandSocketTransform_Zoom을 반환하게 된다.
	bIsZoomed = true;
	return true;
}

void AFlashlightItem::StopZoom()
{
	if (ZoomRequestHandle.IsValid())
	{
		if (UPlayerCameraComponent* PlayerCameraComponent = GetPlayerCameraComponent())
		{
			PlayerCameraComponent->ReleaseZoom(ZoomRequestHandle);
		}

		ZoomRequestHandle.Invalidate();
	}

	// 원래 손 위치 Getter로 복귀.
	bIsZoomed = false;
}

UPlayerCameraComponent* AFlashlightItem::GetPlayerCameraComponent() const
{
	if (!IsValid(Character))
	{
		return nullptr;
	}

	return Character->FindComponentByClass<UPlayerCameraComponent>();
}
#pragma endregion


void AFlashlightItem::ToggleFlashlight()
{
	if (bIsFlashlightOn)
	{
		TurnOffFlashlight();
		return;
	}
	TurnOnFlashlight();
}

bool AFlashlightItem::TurnOnFlashlight()
{
	if (bIsFlashlightOn) { return true; }
	if (!HasBatteryCharge())
	{
		SetLightEnabled(false);
		return false;
	}
	bIsFlashlightOn = true;
	StartBatteryDrain();

	if (bIsBroken)
	{
		StartFlickering();
	}
	else
	{
		SetLightEnabled(true);
	}
	return true;
}

void AFlashlightItem::TurnOffFlashlight()
{
	bIsFlashlightOn = false;
	StopBatteryDrain();
	StopFlickering();
	SetLightEnabled(false);
}

void AFlashlightItem::SetLightEnabled(bool bEnabled)
{
	if (!FlashlightLight) { return; }
	const bool bCanEnable = bEnabled && bIsFlashlightOn && HasBatteryCharge();
	FlashlightLight->SetVisibility(bCanEnable, true);
}

void AFlashlightItem::StartBatteryDrain()
{
	if (!GetWorld()) { return; }
	if (BatteryDrainAmount <= 0.0f || BatteryDrainInterval <= 0.0f)
	{
		return;
	}
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (TimerManager.IsTimerActive(BatteryDrainTimerHandle))
	{
		return;
	}

	TimerManager.SetTimer(
		BatteryDrainTimerHandle,
		this,
		&AFlashlightItem::DrainBattery,
		BatteryDrainInterval,
		true
	);
}

void AFlashlightItem::StopBatteryDrain()
{
	if (!GetWorld()) { return; }
	GetWorld()->GetTimerManager().ClearTimer(BatteryDrainTimerHandle);
}

void AFlashlightItem::DrainBattery()
{
	if (!bIsFlashlightOn)
	{
		StopBatteryDrain();
		return;
	}
	CurrentBatteryCharge = FMath::Max(0.0f, CurrentBatteryCharge - BatteryDrainAmount);
	if (!HasBatteryCharge())
	{
		HandleBatteryDepleted();
	}
}

void AFlashlightItem::HandleBatteryDepleted()
{
	CurrentBatteryCharge = 0.0f;
	bIsFlashlightOn = false;

	StopBatteryDrain();
	StopFlickering();
	SetLightEnabled(false);

	UE_LOG(LogTemp, Log, TEXT("%s: Flashlight battery depleted."), *GetName());
}

float AFlashlightItem::GetBatteryPercent() const
{
	if (MaxBatteryCharge <= UE_KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp(CurrentBatteryCharge / MaxBatteryCharge, 0.0f, 1.0f);
}

bool AFlashlightItem::ReplaceBattery(ABatteryItem* BatteryItem)
{
	if (!IsValid(BatteryItem))
	{
		return false;
	}
	if (!BatteryItem->HasCharge())
	{
		return false;
	}
	const float NewCharge = BatteryItem->ConsumeBattery();

	return ReplaceBatteryByCharge(NewCharge);
}

bool AFlashlightItem::ReplaceBatteryByCharge(float NewBatteryCharge)
{
	if (NewBatteryCharge <= 0.0f)
	{
		return false;
	}

	const bool bWasTurnedOn = bIsFlashlightOn;

	/*
	 * 배터리 "교체"이므로 기존 잔량에 더하지 않고
	 * 새로운 배터리의 충전량으로 덮어쓴다.
	 */
	CurrentBatteryCharge = FMath::Clamp(
		NewBatteryCharge,
		0.0f,
		MaxBatteryCharge
	);

	if (!HasBatteryCharge())
	{
		return false;
	}

	/*
	 * 배터리 고갈 시 bIsFlashlightOn이 false가 되므로,
	 * 기본적으로 새 배터리를 넣었다고 자동으로 켜지는 것은 아니다.
	 *
	 * 교체 전 켜져 있던 경우만 다시 켜는 정책이다.
	 */
	if (bWasTurnedOn)
	{
		TurnOnFlashlight();
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s: Battery replaced. Charge: %.1f / %.1f"),
		*GetName(),
		CurrentBatteryCharge,
		MaxBatteryCharge
	);

	return true;
}

void AFlashlightItem::ApplyFlashlightDamage(float DamageAmount)
{
	if (DamageAmount <= 0.0f || bIsBroken)
	{
		return;
	}

	CurrentDurability = FMath::Max(
		0.0f,
		CurrentDurability - DamageAmount
	);

	if (CurrentDurability <= UE_KINDA_SMALL_NUMBER)
	{
		BreakFlashlight();
	}
}

void AFlashlightItem::BreakFlashlight()
{
	if (bIsBroken)
	{
		return;
	}

	bIsBroken = true;
	CurrentDurability = 0.0f;

	if (bIsFlashlightOn && HasBatteryCharge())
	{
		StartFlickering();
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("%s: Flashlight has broken."),
		*GetName()
	);
}

void AFlashlightItem::RepairFlashlight()
{
	bIsBroken = false;
	CurrentDurability = MaxDurability;

	StopFlickering();

	if (bIsFlashlightOn && HasBatteryCharge())
	{
		SetLightEnabled(true);
	}
	else
	{
		SetLightEnabled(false);
	}
}

void AFlashlightItem::StartFlickering()
{
	if (!bIsBroken ||
		!bIsFlashlightOn ||
		!HasBatteryCharge())
	{
		return;
	}

	ScheduleNextFlicker();
}

void AFlashlightItem::StopFlickering()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FlickerTimerHandle);
	}

	/*
	 * 깜빡임이 끝났더라도 정상 상태이고 전원이 켜져 있다면
	 * 조명을 다시 켠다.
	 */
	if (!bIsBroken && bIsFlashlightOn && HasBatteryCharge())
	{
		SetLightEnabled(true);
	}
	else
	{
		SetLightEnabled(false);
	}
}

void AFlashlightItem::ScheduleNextFlicker()
{
	if (!GetWorld())
	{
		return;
	}

	const float SafeMinInterval =
		FMath::Max(0.01f, MinFlickerInterval);

	const float SafeMaxInterval =
		FMath::Max(SafeMinInterval, MaxFlickerInterval);

	const float NextInterval = FMath::FRandRange(
		SafeMinInterval,
		SafeMaxInterval
	);

	GetWorld()->GetTimerManager().SetTimer(
		FlickerTimerHandle,
		this,
		&AFlashlightItem::UpdateFlicker,
		NextInterval,
		false
	);
}

void AFlashlightItem::UpdateFlicker()
{
	if (!bIsBroken ||
		!bIsFlashlightOn ||
		!HasBatteryCharge())
	{
		StopFlickering();
		return;
	}

	const bool bShouldLightBeOn =
		FMath::FRand() <= FlickerOnProbability;

	SetLightEnabled(bShouldLightBeOn);

	ScheduleNextFlicker();
}