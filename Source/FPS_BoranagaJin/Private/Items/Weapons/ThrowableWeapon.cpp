// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/ThrowableWeapon.h"
#include "Characters/Player/CharacterPlayer.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AThrowableWeapon::AThrowableWeapon()
{
}

void AThrowableWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AThrowableWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AThrowableWeapon::InitItem(ACharacterPlayer* NewCharacter)
{
	Super::InitItem(NewCharacter);
	if (Character)
	{
		CharacterAnimInstance = Character->GetArmMesh()->GetAnimInstance();
		//InitializeCamera(Character);
		LoadWeaponData_Upgrade();
	}
	InitializeUI();

	CharacterController = Cast<APlayerController>(Character->GetController());
	if (CharacterController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(CharacterController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}
	}

	SetMeshVisibility(false);
	SetAimSocketRelativeTransform();

	InitProjectiles(FireData_L.ProjectileClass, 10);
	InitProjectiles(FireData_R.ProjectileClass, 10);
}

void AThrowableWeapon::LoadWeaponData_Upgrade()
{
	Super::LoadWeaponData_Upgrade();
}

void AThrowableWeapon::SetInputActionBinding()
{
	//if (Character)
	//{
	//	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	//	{
	//		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
	//		{
	//			// <LeftMouseAction>
	//			if (LeftMouseAction == EWeaponAction::WeaponAction_SingleShot)
	//			{
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
	//					LeftSingleShotAction,
	//					ETriggerEvent::Started,
	//					[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
	//					{
	//						HandleSingleFire(bIsLeftInput, bSingleProjectile, NumPenetrable);
	//					},
	//					true, !bEnableMultiProjectile_L, MaxPenetrableObjectsNum_Left
	//				));
	//			}
	//			else if (LeftMouseAction == EWeaponAction::WeaponAction_BurstShot)
	//			{
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
	//					LeftBurstShotAction,
	//					ETriggerEvent::Started,
	//					[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
	//					{
	//						HandleBurstFire(bIsLeftInput, bSingleProjectile, NumPenetrable);
	//					},
	//					true, !bEnableMultiProjectile_L, MaxPenetrableObjectsNum_Left
	//				));
	//			}
	//			else if (LeftMouseAction == EWeaponAction::WeaponAction_FullAutoShot)
	//			{
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
	//					LeftFullAutoShotAction,
	//					ETriggerEvent::Triggered,
	//					[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
	//					{
	//						StartFullAutoShot(bIsLeftInput, bSingleProjectile, NumPenetrable);
	//					},
	//					true, !bEnableMultiProjectile_L, MaxPenetrableObjectsNum_Left
	//				));
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(LeftFullAutoShotAction, ETriggerEvent::Completed, this, &AWeaponTargeting::StopFullAutoShot));
	//			}
	//			else if (LeftMouseAction == EWeaponAction::WeaponAction_Charge)
	//			{
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(LeftChargeAction, ETriggerEvent::Triggered, this, &AWeaponTargeting::StartCharge));
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(LeftChargeAction, ETriggerEvent::Completed, this, &AWeaponTargeting::StopCharge));
	//			}
	//			else if (LeftMouseAction == EWeaponAction::WeaponAction_Zoom)
	//			{
	//				// No Left Zoom
	//			}

	//			// <RightMouseAction>
	//			if (RightMouseAction == EWeaponAction::WeaponAction_SingleShot)
	//			{
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
	//					RightSingleShotAction,
	//					ETriggerEvent::Started,
	//					[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
	//					{
	//						HandleSingleFire(bIsLeftInput, bSingleProjectile, NumPenetrable);
	//					},
	//					false, !bEnableMultiProjectile_R, MaxPenetrableObjectsNum_Right
	//				));
	//			}
	//			else if (RightMouseAction == EWeaponAction::WeaponAction_BurstShot)
	//			{
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
	//					RightBurstShotAction,
	//					ETriggerEvent::Started,
	//					[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
	//					{
	//						HandleBurstFire(bIsLeftInput, bSingleProjectile, NumPenetrable);
	//					},
	//					false, !bEnableMultiProjectile_R, MaxPenetrableObjectsNum_Right
	//				));
	//			}
	//			else if (RightMouseAction == EWeaponAction::WeaponAction_FullAutoShot)
	//			{
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
	//					RightFullAutoShotAction,
	//					ETriggerEvent::Started,
	//					[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
	//					{
	//						StartFullAutoShot(bIsLeftInput, bSingleProjectile, NumPenetrable);
	//					},
	//					false, !bEnableMultiProjectile_R, MaxPenetrableObjectsNum_Right
	//				));
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightFullAutoShotAction, ETriggerEvent::Completed, this, &AWeaponTargeting::StopFullAutoShot));
	//			}
	//			else if (RightMouseAction == EWeaponAction::WeaponAction_Charge)
	//			{
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightChargeAction, ETriggerEvent::Triggered, this, &AWeaponTargeting::StartCharge));
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightChargeAction, ETriggerEvent::Completed, this, &AWeaponTargeting::StopCharge));
	//			}
	//			else if (RightMouseAction == EWeaponAction::WeaponAction_Zoom)
	//			{
	//				InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightZoomAction, ETriggerEvent::Started, this, &AWeaponTargeting::ZoomToggle));
	//			}
	//		}
	//	}
	//}
}
