#include "Items/FlashlightItem.h"
#include "Items/BatteryItem.h"
#include "Items/InventorySystemComponent.h"
#include "Characters/Player/CharacterPlayer.h"

#include "Components/SpotLightComponent.h"
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

	AM_Unequip_Character->BlendOut.SetBlendTime(1000.f);
	AM_Unequip_Character->bEnableAutoBlendOut = false;
}

void AFlashlightItem::OnEquipEnded(ACharacterPlayer* UsingCharacter)
{
	if (!UsingCharacter) { return; }
	TurnOnFlashlight();
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

	UseItem(Character); //FlashlightLight->SetVisibility(true); //TODO: UseItem을 바로 사용하지 않는 방식으로 바꿔야함

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