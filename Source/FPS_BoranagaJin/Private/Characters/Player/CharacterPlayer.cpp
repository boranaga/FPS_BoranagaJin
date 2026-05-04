// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Player/CharacterPlayer.h"
#include "Characters/Player/PlayerMovementComponent.h"
#include "Characters/Player/PlayerCameraComponent.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Items/Weapons/WeaponSystemComponent.h"
#include "GameModes/DefaultGameMode.h"
#include "Instance/DefaultGameInstance.h"
#include "Data/PlayerSoundData.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

ACharacterPlayer::ACharacterPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule Component"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->SetSimulatePhysics(false);
	CapsuleComponent->InitCapsuleSize(40.f, 90.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->SetNotifyRigidBodyCollision(true);


	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CapsuleComponent);
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	Camera->bUsePawnControlRotation = true;
	Camera->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
	DefaultCameraRelativeLocation = Camera->GetRelativeLocation();

	ArmMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Arm Mesh"));
	ArmMesh->SetupAttachment(Camera);

	HandsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Hands Mesh"));
	HandsMesh->SetupAttachment(ArmMesh);
	HandsMesh->SetLeaderPoseComponent(ArmMesh);

	MovementComponent = CreateDefaultSubobject<UPlayerMovementComponent>(TEXT("Movement Component"));
	MovementComponent->UpdatedComponent = RootComponent;
	MovementComponent->SetDefaultCapsuleValues(CapsuleComponent->GetScaledCapsuleRadius(), CapsuleComponent->GetScaledCapsuleHalfHeight());

	CameraMovementComponent = CreateDefaultSubobject<UPlayerCameraComponent>(TEXT("CameraMovement Component"));


	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// <WeaponSystem>
	WeaponSystem = CreateDefaultSubobject<UWeaponSystemComponent>(TEXT("WeaponSystem"));
	CapsuleComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel7, ECR_Ignore);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel8, ECR_Ignore);
	ArmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// FPSceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent")); //<JaeHyeong>
	// FPSceneCapture->SetupAttachment(Camera);

	// for damage interactions with enemies
	//AttackTokensComponent = CreateDefaultSubobject<UACPlayerAttackTokens>(TEXT("Attack Tokens Component"));
	//DamageSystemComponent = CreateDefaultSubobject<UACDamageSystem>(TEXT("Damage System Component"));

	// UIManager actor components
	//UIManager = CreateDefaultSubobject<UACUIMangerComponent>(TEXT("UI Manager Component"));
	//HitScreenManager = CreateDefaultSubobject<UACHitScreenManager>(TEXT("HitScreen Manager Component"));

	ForwardDashEffectComponent = CreateDefaultSubobject<UNiagaraComponent>("Forward Dash Effect Component");
	ForwardDashEffectComponent->SetupAttachment(Camera);
	ForwardDashEffectComponent->SetAutoActivate(false);

	BackwardDashEffectComponent = CreateDefaultSubobject<UNiagaraComponent>("Backward Dash Effect Component");
	BackwardDashEffectComponent->SetupAttachment(Camera);
	BackwardDashEffectComponent->SetAutoActivate(false);

	LeftDashEffectComponent = CreateDefaultSubobject<UNiagaraComponent>("Left Dash Effect Component");
	LeftDashEffectComponent->SetupAttachment(Camera);
	LeftDashEffectComponent->SetAutoActivate(false);

	RightDashEffectComponent = CreateDefaultSubobject<UNiagaraComponent>("Right Dash Effect Component");
	RightDashEffectComponent->SetupAttachment(Camera);
	RightDashEffectComponent->SetAutoActivate(false);

	WallRunAudioComponent = CreateDefaultSubobject<UAudioComponent>("WallRunAudioComponent");
	WallRunAudioComponent->SetupAttachment(RootComponent);
	WallRunAudioComponent->bAutoActivate = false;

	SlideAudioComponent = CreateDefaultSubobject<UAudioComponent>("SlideAudioComponent");
	SlideAudioComponent->SetupAttachment(RootComponent);
	SlideAudioComponent->bAutoActivate = false;
}

void ACharacterPlayer::BeginPlay()
{
	Super::BeginPlay();

	Camera->SetRelativeLocation(DefaultCameraRelativeLocation);

	// Crash the game if there is no data asset assigned
	checkf(PlayerSound_DataAsset, TEXT("Player sound data asset is not assigned"));

	WallRunAudioComponent->SetSound(PlayerSound_DataAsset->WallRunSound.Sound);
	WallRunAudioComponent->OnAudioPlaybackPercent.AddDynamic(this, &ACharacterPlayer::HandleWallRunAudioPlayback);
	SlideAudioComponent->SetSound(PlayerSound_DataAsset->SlideSound.Sound);
	SlideAudioComponent->OnAudioPlaybackPercent.AddDynamic(this, &ACharacterPlayer::HandleSlideAudioPlayback);


	//GetDamageSystemComponent()->OnDamaged.AddUObject(CameraMovementComponent, &UPlayerCameraComponent::OnDamaged);
	//GetDamageSystemComponent()->OnDeath.AddUObject(this, &ACharacterPlayer::OnDeath);

	GetPlayerMovementComponent()->OnPrimaryJumpDelegate.AddDynamic(this, &ACharacterPlayer::OnPrimaryJump);
	GetPlayerMovementComponent()->OnDoubleJumpDelegate.AddDynamic(this, &ACharacterPlayer::OnDoubleJump);
	GetPlayerMovementComponent()->OnWallJumpDelegate.AddDynamic(this, &ACharacterPlayer::OnWallJump);
	GetPlayerMovementComponent()->OnWallRunDelegate.AddDynamic(this, &ACharacterPlayer::OnWallRun);
	GetPlayerMovementComponent()->OnWallRunEndDelegate.AddDynamic(this, &ACharacterPlayer::OnWallRunEnd);
	GetPlayerMovementComponent()->OnSlideDelegate.AddDynamic(this, &ACharacterPlayer::OnSlide);
	GetPlayerMovementComponent()->OnSlideEndDelegate.AddDynamic(this, &ACharacterPlayer::OnSlideEnd);
	GetPlayerMovementComponent()->OnLandDelegate.AddDynamic(this, &ACharacterPlayer::OnLand);
	GetPlayerMovementComponent()->OnDashDelegate.AddDynamic(this, &ACharacterPlayer::OnDash);
	GetPlayerMovementComponent()->OnDashEndDelegate.AddDynamic(this, &ACharacterPlayer::OnDashEnd);


	CachedGameInstance = Cast<UCustomGameInstance>(GetGameInstance());
	//if (!CachedGameInstance)
	//{
	//	UE_LOG(LogTemp, Error, TEXT("ACharacterPlayer::BeginPlay - CachedGameInstance is invalid!!"));
	//}

	//if (AGameModeBase* GameMode = Cast<AGameModeBase>(GetWorld()->GetAuthGameMode()))
	//{
	//	UCheckpointSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UCheckpointSubsystem>();
	//	if (Subsystem && Subsystem->HasSavedCheckpoint())
	//	{
	//		FName CurrentMapName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));
	//		if (CurrentMapName == Subsystem->GetCurrentSave()->MapName)
	//		{
	//			GameMode->TeleportToLastCheckpoint();
	//		}
	//	}
	//}
}

void ACharacterPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	//GetDamageSystemComponent()->OnDeath.RemoveAll(this);
}

UCapsuleComponent* ACharacterPlayer::GetCapsuleComponent()
{
	return CapsuleComponent;
}

bool ACharacterPlayer::HasWeapon() const
{
	if (WeaponSystem)
	{
		return WeaponSystem->GetCurrentWeapon() != nullptr;
	}
	return false;
}

void ACharacterPlayer::UpdateLookInputVector2D(const FInputActionValue& InputValue)
{
	PlayerLookInputVector2D = InputValue.Get<FVector2D>();
}

void ACharacterPlayer::SetLookInputVector2DZero()
{
	PlayerLookInputVector2D = FVector2D::ZeroVector;
}


void ACharacterPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACharacterPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACharacterPlayer::HandleMoveInput);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ACharacterPlayer::HandleMoveInput);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACharacterPlayer::HandleLookInput);
		//EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Started, this, &ACharacterPlayer::LookInputTest);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacterPlayer::StartJumpInput);
		EnhancedInputComponent->BindAction(ShiftAction, ETriggerEvent::Started, this, &ACharacterPlayer::StartShiftInput);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ACharacterPlayer::StartCrouchInput);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ACharacterPlayer::StopCrouchInput);

		// Developer Action
		//EnhancedInputComponent->BindAction(TeleportToLastCheckpointAction, ETriggerEvent::Started, this, &ThisClass::StartTeleportToLastCheckpointInput);

		// <WeaponSystem>
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACharacterPlayer::UpdateLookInputVector2D);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::None, this, &ACharacterPlayer::SetLookInputVector2DZero);
	}
}

void ACharacterPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);

			UE_LOG(LogTemp, Error, TEXT("Player input mapping 222"));
		}
	}

	UE_LOG(LogTemp, Error, TEXT("Player input mapping!"));
}

void ACharacterPlayer::OnPrimaryJump()
{
	const FPlayerSoundData& Data = PlayerSound_DataAsset->PrimaryJumpSound;
	float VolumeMultiplier, PitchMultiplier;

	float Speed = GetPlayerMovementComponent()->Velocity.Size();

	CalculateMappedSoundValue(Data, Speed, VolumeMultiplier, PitchMultiplier);

	UGameplayStatics::SpawnSoundAttached(Data.Sound, GetRootComponent(), NAME_None,
		FVector(ForceInit), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset,
		false, VolumeMultiplier, PitchMultiplier);

	if (Data.bDebug)
	{
		GEngine->AddOnScreenDebugMessage(1256, Data.DebugDisplayDuration, FColor::Green,
			FString::Printf(TEXT("Primary Jump Sound Evaluation Speed: %.0f, "
				"Mapped Volume Multiplier: %.3f, Mapped Pitch Multiplier: %.3f"), Speed, VolumeMultiplier, PitchMultiplier));
	}
}

void ACharacterPlayer::OnDoubleJump()
{
	const FPlayerSoundData& Data = PlayerSound_DataAsset->DoubleJumpSound;
	float VolumeMultiplier, PitchMultiplier;
	float Speed = GetPlayerMovementComponent()->Velocity.Size();
	CalculateMappedSoundValue(Data, GetPlayerMovementComponent()->Velocity.Size(), VolumeMultiplier, PitchMultiplier);

	UGameplayStatics::SpawnSoundAttached(PlayerSound_DataAsset->DoubleJumpSound.Sound, GetRootComponent(), NAME_None,
		FVector(ForceInit), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset,
		false, VolumeMultiplier, PitchMultiplier);

	if (Data.bDebug)
	{
		if (!GEngine) return;
		GEngine->AddOnScreenDebugMessage(1257, Data.DebugDisplayDuration, FColor::Green,
			FString::Printf(TEXT("Double Jump Sound Evaluation Speed: %.0f, "
				"Mapped Volume Multiplier: %.3f, Mapped Pitch Multiplier: %.3f"), Speed, VolumeMultiplier, PitchMultiplier));
	}
}

void ACharacterPlayer::OnWallJump()
{
	const FPlayerSoundData& Data = PlayerSound_DataAsset->PrimaryJumpSound;
	float VolumeMultiplier, PitchMultiplier;

	float Speed = GetPlayerMovementComponent()->Velocity.Size();

	CalculateMappedSoundValue(Data, Speed, VolumeMultiplier, PitchMultiplier);

	UGameplayStatics::SpawnSoundAttached(Data.Sound, GetRootComponent(), NAME_None,
		FVector(ForceInit), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset,
		false, VolumeMultiplier, PitchMultiplier);

	if (Data.bDebug)
	{
		if (!GEngine) return;
		GEngine->AddOnScreenDebugMessage(1258, Data.DebugDisplayDuration, FColor::Green,
			FString::Printf(TEXT("Primary Jump Sound Evaluation Speed: %.0f, "
				"Mapped Volume Multiplier: %.3f, Mapped Pitch Multiplier: %.3f"), Speed, VolumeMultiplier, PitchMultiplier));
	}
}

void ACharacterPlayer::OnSlide()
{
	const FPlayerSoundData& Data = PlayerSound_DataAsset->SlideSound;
	float VolumeMultiplier, PitchMultiplier;
	float Speed = GetPlayerMovementComponent()->Velocity.Size();
	CalculateMappedSoundValue(Data, Speed, VolumeMultiplier, PitchMultiplier);

	SlideAudioComponent->VolumeMultiplier = VolumeMultiplier;
	SlideAudioComponent->PitchMultiplier = PitchMultiplier;

	if (Data.bDebug)
	{
		if (!GEngine) return;
		GEngine->AddOnScreenDebugMessage(1259, Data.DebugDisplayDuration, FColor::Green,
			FString::Printf(TEXT("Slide Sound Evaluation Speed: %.0f, "
				"Mapped Volume Multiplier: %.3f, Mapped Pitch Multiplier: %.3f"), Speed, VolumeMultiplier, PitchMultiplier));
	}

	float TimeSinceSlideEnd = GetWorld()->GetTimeSeconds() - SlideEndTime;
	if (TimeSinceSlideEnd > 0.75f)
	{
		SlideAudioComponent->SetBoolParameter("SkipStart", false);
		SlideAudioComponent->Play();
	}
	else
	{
		SlideAudioComponent->SetBoolParameter("SkipStart", true);
		SlideAudioComponent->Play();
	}

}

void ACharacterPlayer::OnSlideEnd()
{
	SlideAudioComponent->FadeOut(0.4f, 0.f);
	SlideEndTime = GetWorld()->GetTimeSeconds();
}

void ACharacterPlayer::OnWallRun()
{
	const FPlayerSoundData& Data = PlayerSound_DataAsset->WallRunSound;
	float VolumeMultiplier, PitchMultiplier;
	float Speed = GetPlayerMovementComponent()->Velocity.Size();
	CalculateMappedSoundValue(Data, Speed, VolumeMultiplier, PitchMultiplier);

	WallRunAudioComponent->VolumeMultiplier = VolumeMultiplier;
	WallRunAudioComponent->PitchMultiplier = PitchMultiplier;

	if (Data.bDebug)
	{
		if (!GEngine) return;
		GEngine->AddOnScreenDebugMessage(1260, Data.DebugDisplayDuration, FColor::Green,
			FString::Printf(TEXT("Wall Run Sound Evaluation Speed: %.0f, "
				"Mapped Volume Multiplier: %.3f, Mapped Pitch Multiplier: %.3f"), Speed, VolumeMultiplier, PitchMultiplier));
	}

	WallRunAudioComponent->Play();


}

void ACharacterPlayer::OnWallRunEnd()
{
	WallRunAudioComponent->FadeOut(0.4f, 0.f);
}

void ACharacterPlayer::OnLand(float ZSpeed)
{
	const FPlayerSoundData& Data = PlayerSound_DataAsset->LandSound;
	float VolumeMultiplier, PitchMultiplier;

	float Speed = FMath::Abs(ZSpeed);

	CalculateMappedSoundValue(Data, Speed, VolumeMultiplier, PitchMultiplier);

	UGameplayStatics::SpawnSoundAttached(Data.Sound, GetRootComponent(), NAME_None,
		FVector(ForceInit), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset,
		false, VolumeMultiplier, PitchMultiplier);

	if (Data.bDebug)
	{
		if (!GEngine) return;
		GEngine->AddOnScreenDebugMessage(1261, Data.DebugDisplayDuration, FColor::Green,
			FString::Printf(TEXT("Land Sound Evaluation Speed: %.0f, "
				"Mapped Volume Multiplier: %.3f, Mapped Pitch Multiplier: %.3f"), Speed, VolumeMultiplier, PitchMultiplier));
	}
}

void ACharacterPlayer::HandleWallRunAudioPlayback(const USoundWave* PlayingSoundWave, const float PlaybackPercent)
{
	const FPlayerSoundData& Data = PlayerSound_DataAsset->WallRunSound;
	float VolumeMultiplier, PitchMultiplier;
	float Speed = GetPlayerMovementComponent()->Velocity.Size();
	CalculateMappedSoundValue(Data, Speed, VolumeMultiplier, PitchMultiplier);

	WallRunAudioComponent->VolumeMultiplier = VolumeMultiplier;
	WallRunAudioComponent->PitchMultiplier = PitchMultiplier;

	if (Data.bDebug)
	{
		if (!GEngine) return;
		GEngine->AddOnScreenDebugMessage(1262, Data.DebugDisplayDuration, FColor::Green,
			FString::Printf(TEXT("Wall Run Sound Evaluation Speed: %.0f, "
				"Mapped Volume Multiplier: %.3f, Mapped Pitch Multiplier: %.3f"), Speed, VolumeMultiplier, PitchMultiplier));
	}
}

void ACharacterPlayer::HandleSlideAudioPlayback(const USoundWave* PlayingSoundWave, const float PlaybackPercent)
{
	const FPlayerSoundData& Data = PlayerSound_DataAsset->SlideSound;
	float VolumeMultiplier, PitchMultiplier;
	float Speed = GetPlayerMovementComponent()->Velocity.Size();
	CalculateMappedSoundValue(Data, Speed, VolumeMultiplier, PitchMultiplier);

	SlideAudioComponent->VolumeMultiplier = VolumeMultiplier;
	SlideAudioComponent->PitchMultiplier = PitchMultiplier;

	if (Data.bDebug)
	{
		if (!GEngine) return;
		GEngine->AddOnScreenDebugMessage(1263, Data.DebugDisplayDuration, FColor::Green,
			FString::Printf(TEXT("Slide Sound Evaluation Speed: %.0f, "
				"Mapped Volume Multiplier: %.3f, Mapped Pitch Multiplier: %.3f"), Speed, VolumeMultiplier, PitchMultiplier));
	}
}

void ACharacterPlayer::HandleMoveInput(const FInputActionValue& Value)
{
	if (!MovementComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Player movement component is not valid!"));
		return;
	}

	FVector2D InputVector = Value.Get<FVector2D>();

	const FRotator YawRotation = FRotator(0, GetControlRotation().Yaw, 0);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	MovementComponent->AddInputVector(ForwardDirection * InputVector.Y);
	MovementComponent->AddInputVector(RightDirection * InputVector.X);
	MovementComponent->SetMovementInputVector(InputVector);
}

void ACharacterPlayer::HandleLookInput(const FInputActionValue& Value)
{
	//UE_LOG(LogTemp, Error, TEXT("Player Mouse Input!"));

	if (MovementComponent->GetMovementState() == EMovementState::EMS_Downed ||
		MovementComponent->GetMovementState() == EMovementState::EMS_Dead)
	{
		return;
	}

	FVector2D InputVector = Value.Get<FVector2D>();

	if (CachedGameInstance)
	{
		AddControllerYawInput(InputVector.X * CachedGameInstance->MouseSensitivity);
		AddControllerPitchInput(InputVector.Y * CachedGameInstance->MouseSensitivity);
	}
	else
	{
		AddControllerYawInput(InputVector.X * 1.0f);
		AddControllerPitchInput(InputVector.Y * 1.0f);
	}
}

void ACharacterPlayer::LookInputTest(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Error, TEXT("LookInputTest(const FInputActionValue& Value)"));
}


void ACharacterPlayer::StartJumpInput()
{
	if (!MovementComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Player movement component is not valid!"));
		return;
	}

	MovementComponent->SetJumpPressed(true);
}


void ACharacterPlayer::StartShiftInput()
{
	if (!MovementComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Player movement component is not valid!"));
		return;
	}

	MovementComponent->SetShiftPressed(true);
}


void ACharacterPlayer::StartCrouchInput()
{
	if (!MovementComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Player movement component is not valid!"));
		return;
	}

	MovementComponent->SetCrouchPressed(true);
}

void ACharacterPlayer::StopCrouchInput()
{
	if (!MovementComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Player movement component is not valid!"));
		return;
	}

	MovementComponent->SetCrouchPressed(false);
}

void ACharacterPlayer::StartTeleportToLastCheckpointInput()
{
	//if (AGameModeBase* LevelGameMode = Cast<AGameModeBase>(GetWorld()->GetAuthGameMode()))
	//{
	//	LevelGameMode->TeleportToLastCheckpoint();
	//}
}

void ACharacterPlayer::CalculateMappedSoundValue(const FPlayerSoundData& Data, float Speed,
	float& OutVolumeMultiplier, float& OutPitchMultiplier)
{
	if (Data.bMapVolume)
	{
		TRange<float> VolumeSpeedRange = TRange<float>(Data.VolumeSpeedRange.Min, Data.VolumeSpeedRange.Max);
		TRange<float> VolumeRange = TRange<float>(Data.VolumeRange.Min, Data.VolumeRange.Max);
		OutVolumeMultiplier = FMath::GetMappedRangeValueClamped(VolumeSpeedRange, VolumeRange, Speed);
	}
	else
	{
		OutVolumeMultiplier = 1.f;
	}

	if (Data.bMapPitch)
	{
		TRange<float> PitchSpeedRange = TRange<float>(Data.PitchSpeedRange.Min, Data.PitchSpeedRange.Max);
		TRange<float> PitchRange = TRange<float>(Data.PitchRange.Min, Data.PitchRange.Max);
		OutPitchMultiplier = FMath::GetMappedRangeValueClamped(PitchSpeedRange, PitchRange, Speed);
	}
	else
	{
		OutPitchMultiplier = 1.f;
	}
}

void ACharacterPlayer::OnDamaged()
{
}

bool ACharacterPlayer::TakeDamage(const FDamageParams& DamageData, AActor* DamageCauser)
{
	//if (MovementComponent->GetIsInvincible())
	//{
	//	return false;
	//}

	//if (UIManager)
	//{
	//	UIManager->ShowDamageIndicator(DamageCauser);
	//}

	//GetPlayerMovementComponent()->NotifyDamageData(DamageData.DamageType, DamageData.ImpulseDirection, DamageData.ImpulseMagnitude);

	//if (DamageCauser)
	//{
	//	USoundBase* HitSound = nullptr;
	//	if (AEnemyBase* Enemy = Cast<AEnemyBase>(DamageCauser))
	//	{
	//		FName EnemyType = Enemy->GetEnemyType();
	//		if (EnemyType == "Melee")
	//		{
	//			HitSound = PlayerSound_DataAsset->MeleeEnemyHitSound;
	//		}
	//		else if (EnemyType == "Rifle")
	//		{
	//			HitSound = PlayerSound_DataAsset->RifleEnemyHitSound;
	//		}
	//		else if (EnemyType == "Charger")
	//		{
	//			HitSound = PlayerSound_DataAsset->ChargerEnemyHitSound;
	//		}
	//		else if (EnemyType == "Turret")
	//		{
	//			HitSound = PlayerSound_DataAsset->TurretEnemyHitSound;
	//		}
	//	}
	//	else if (ACharacterBossProto* Boss = Cast<ACharacterBossProto>(DamageCauser))
	//	{
	//		switch (DamageData.DamageType)
	//		{
	//		case EDamageType::Charge:
	//			HitSound = PlayerSound_DataAsset->BossDownedHitSound;
	//			break;
	//		default:
	//			HitSound = PlayerSound_DataAsset->BossNormalHitSound;
	//			break;
	//		}
	//	}

	//	if (HitSound)
	//	{
	//		UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, GetActorLocation());
	//	}
	//}



	//return GetDamageSystemComponent()->TakeDamage(DamageData, DamageCauser);
	//------------------------------------
	return false;
}

//void ACharacterPlayer::GravityLaunchPlayer(const FVector& Direction, float ForceAmount)
//{
//	GetPlayerMovementComponent()->NotifyGravityLaunchForce(Direction, ForceAmount);
//}
//
//void ACharacterPlayer::JumpPadLaunchPlayer(float ForceAmount)
//{
//	GetPlayerMovementComponent()->NotifyJumpPadLaunchForce(ForceAmount);
//}
//
//void ACharacterPlayer::RequestMovementDataModification(const TArray<FPlayerMovementDataModifier>& Modifiers)
//{
//	GetPlayerMovementComponent()->NotifyMovementDataModification(Modifiers);
//}
//
//void ACharacterPlayer::RequestMovementKeyHoldModification(const TArray<FPlayerKeyHoldModifier>& Modifiers)
//{
//	GetPlayerMovementComponent()->NotifyMovementKeyHoldModification(Modifiers);
//}
//
//void ACharacterPlayer::RequestResetModification()
//{
//	GetPlayerMovementComponent()->NotifyResetModification();
//}


void ACharacterPlayer::OnDeath()
{
	GEngine->AddOnScreenDebugMessage(10, 15.0f, FColor::Yellow, TEXT("Player Dead"));
	GetPlayerMovementComponent()->NotifyDeath();
}

void ACharacterPlayer::OnDash(FVector2D MovementInput)
{
	const FPlayerSoundData& Data = PlayerSound_DataAsset->DashSound;
	float VolumeMultiplier, PitchMultiplier;

	float Speed = GetPlayerMovementComponent()->Velocity.Size();

	CalculateMappedSoundValue(Data, Speed, VolumeMultiplier, PitchMultiplier);

	UGameplayStatics::SpawnSoundAttached(Data.Sound, GetRootComponent(), NAME_None,
		FVector(ForceInit), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset,
		false, VolumeMultiplier, PitchMultiplier);

	if (Data.bDebug)
	{
		if (!GEngine) return;
		GEngine->AddOnScreenDebugMessage(77777, Data.DebugDisplayDuration, FColor::Green,
			FString::Printf(TEXT("Dash Sound Evaluation Speed: %.0f, "
				"Mapped Volume Multiplier: %.3f, Mapped Pitch Multiplier: %.3f"), Speed, VolumeMultiplier, PitchMultiplier));
	}


	if (MovementInput.IsZero())
	{
		ForwardDashEffectComponent->Activate();

		if (BackwardDashEffectComponent->IsActive())
			BackwardDashEffectComponent->Deactivate();
		if (LeftDashEffectComponent->IsActive())
			LeftDashEffectComponent->Deactivate();
		if (RightDashEffectComponent->IsActive())
			RightDashEffectComponent->Deactivate();
	}
	else
	{
		if (MovementInput.Y > 0)
		{
			ForwardDashEffectComponent->Activate();

			if (BackwardDashEffectComponent->IsActive())
				BackwardDashEffectComponent->Deactivate();
			if (LeftDashEffectComponent->IsActive())
				LeftDashEffectComponent->Deactivate();
			if (RightDashEffectComponent->IsActive())
				RightDashEffectComponent->Deactivate();
		}
		else if (MovementInput.Y < 0)
		{
			BackwardDashEffectComponent->Activate();

			if (ForwardDashEffectComponent->IsActive())
				ForwardDashEffectComponent->Deactivate();
			if (LeftDashEffectComponent->IsActive())
				LeftDashEffectComponent->Deactivate();
			if (RightDashEffectComponent->IsActive())
				RightDashEffectComponent->Deactivate();
		}
		else
		{
			if (MovementInput.X < 0)
			{
				LeftDashEffectComponent->Activate();
				if (ForwardDashEffectComponent->IsActive())
					ForwardDashEffectComponent->Deactivate();
				if (BackwardDashEffectComponent->IsActive())
					BackwardDashEffectComponent->Deactivate();
				if (RightDashEffectComponent->IsActive())
					RightDashEffectComponent->Deactivate();
			}
			else if (MovementInput.X > 0)
			{
				RightDashEffectComponent->Activate();

				if (ForwardDashEffectComponent->IsActive())
					ForwardDashEffectComponent->Deactivate();
				if (BackwardDashEffectComponent->IsActive())
					BackwardDashEffectComponent->Deactivate();
				if (LeftDashEffectComponent->IsActive())
					LeftDashEffectComponent->Deactivate();
			}
		}
	}

}

void ACharacterPlayer::OnDashEnd()
{
	ForwardDashEffectComponent->Deactivate();
	BackwardDashEffectComponent->Deactivate();
	LeftDashEffectComponent->Deactivate();
	RightDashEffectComponent->Deactivate();
}


