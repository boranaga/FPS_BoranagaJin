


#include "Items/Weapons/WeaponTargeting.h"
#include "Items/Weapons/Weapon.h"
#include "Items/Weapons/WeaponSystemComponent.h"
#include "Items/WeaponState/WeaponIdleState.h"
#include "Items/WeaponState/WeaponTargetingState.h"
#include "Items/WeaponState/WeaponFiringState.h"
#include "Items/WeaponState/WeaponPumpActionReloadState.h"
#include "Items/WeaponState/WeaponUnequippedState.h"
#include "Data/WeaponData.h"
#include "Data/PlayerWeaponPrimaryDataAsset.h"
#include "UI/TargetMarkerWidget.h"
#include "UI/TargetingSkillWidget.h"
#include "Characters/Player/CharacterPlayer.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

AWeaponTargeting::AWeaponTargeting()
{
}

void AWeaponTargeting::BeginPlay()
{
	Super::BeginPlay();
}

void AWeaponTargeting::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateGlobalTimeDiation(DeltaTime); //TODO: If the player tries to switch weapons, reset the global time dilation to its default value
	UpdateTargetingSkillUI(); //TODO: Should Update UI When even TargetingSkillTimer is not activated
}

void AWeaponTargeting::InitWeapon(ACharacterPlayer* NewCharacter)
{
	Character = NewCharacter;
	if (Character)
	{
		CharacterAnimInstance = Character->GetArmMesh()->GetAnimInstance();
		//InitializeCamera(Character);
		LoadWeaponData_Upgrade();
	}
	InitializeUI();

	// Set up action bindings
	CharacterController = Cast<APlayerController>(Character->GetController());
	if (CharacterController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(CharacterController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}
	}

	SetMeshVisibility(false);
	SetAimSocketRelativeTransform();

	InitProjectileShells();

	if (bIsSkillWeapon)
	{
		InitProjectiles(FireData_L.ProjectileClass, 2);
		InitProjectiles(FireData_R.ProjectileClass, 2);
		InitProjectiles(FireData_Skill.ProjectileClass, 12);
	}
	else
	{
		InitProjectiles(FireData_L.ProjectileClass, 10);
		InitProjectiles(FireData_R.ProjectileClass, 10);
	}
}

void AWeaponTargeting::InitializeUI()
{
	Super::IsActorInitialized();

	if (TargetingSkillWidgetClass)
	{
		TargetingSkillWidget = CreateWidget<UTargetingSkillWidget>(GetWorld(), TargetingSkillWidgetClass);
		if (TargetingSkillWidget)
		{
			TargetingSkillWidget->InitializeUI(TargetingSkillCoolDown, TargetingSkillCoolDown, ElapsedTimeAfterTargetingStarted, MaxTargetingTime);
			TargetingSkillWidget->SetDetectionTimeUIVisible(false);
		}
	}
}

void AWeaponTargeting::LoadWeaponData_Upgrade()
{
	Super::LoadWeaponData_Upgrade();

	if (WeaponDataTable.IsNull() || WeaponRowName.IsNone()) return;
	LoadedWeaponTable = WeaponDataTable.LoadSynchronous();
	if (!LoadedWeaponTable) return;

	FWeaponData* WeaponData = LoadedWeaponTable->FindRow<FWeaponData>(WeaponRowName, TEXT("LoadWeaponData"));
	if (!WeaponData) return;
	if (!WeaponData->WeaponPDA.IsValid()) return;

	UAssetManager& AM = UAssetManager::Get();

	TSharedPtr<FStreamableHandle> H = AM.LoadPrimaryAsset(WeaponData->WeaponPDA);
	if (H.IsValid()) { H->WaitUntilComplete(); }

	auto* Obj = UAssetManager::Get().GetPrimaryAssetObject(WeaponData->WeaponPDA);
	UPlayerWeaponPrimaryDataAsset* Def = Cast<UPlayerWeaponPrimaryDataAsset>(Obj);
	if (!Def) return;

	TArray<FSoftObjectPath> Paths;
	auto Push = [&Paths](const FSoftObjectPath& P) {if (P.IsValid()) Paths.Add(P); };

	// <Sound>
	Push(Def->TargetSearchLoopSound.ToSoftObjectPath());
	Push(Def->TargetLockedSound.ToSoftObjectPath());

	if (Paths.Num() > 0)
	{
		auto& SM = UAssetManager::GetStreamableManager();
		TWeakObjectPtr<AWeapon> WeakThis(this);

		WeaponAssetsHandle = SM.RequestAsyncLoad(
			Paths,
			FStreamableDelegate::CreateWeakLambda(this, [this, WeakThis, Def]() {
				if (!WeakThis.IsValid()) return;

				TargetSearchLoopSound = Def->TargetSearchLoopSound.Get();
				TargetLockedSound = Def->TargetLockedSound.Get();

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
	// <Skill>
	bIsSkillWeapon = WeaponData->bIsSkillWeapon;
	bAllowNormalFireForSkillWeapon = WeaponData->bAllowNormalFireForSkillWeapon;

	// <Animation>
	RightHandSocketTransform_Targeting = WeaponData->RightHandSocketTransform_Targeting;
	RightHandSocketTransform_Targeting_Crouch = WeaponData->RightHandSocketTransform_Targeting_Crouch;

	// <Targeting(Homing)>
	MissileLaunchDelay = WeaponData->MissileLaunchDelay;
	MaxTargetNum = WeaponData->MaxTargetNum;
	MaxTargetDetectionRadius = WeaponData->MaxTargetDetectionRadius;
	MaxTargetDetectionAngle = WeaponData->MaxTargetDetectionAngle;
	MaxTargetDetectionTime = WeaponData->MaxTargetDetectionTime;
	TimeToReachMaxTargetDetectionRange = WeaponData->TimeToReachMaxTargetDetectionRange;
	TargetingGlobalTimeScale = WeaponData->TargetingGlobalTimeScale;
	TargetingGlobalTimeDilationSpeed_In = WeaponData->TargetingGlobalTimeDilationSpeed_In;
	TargetingGlobalTimeDilationSpeed_Out = WeaponData->TargetingGlobalTimeDilationSpeed_Out;
	TargetingSkillCoolDown = WeaponData->TargetingSkillCoolDown;
	MaxTargetingTime = WeaponData->MaxTargetingTime;
}

void AWeaponTargeting::SetInputActionBinding()
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
						[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
						{
							HandleSingleFire(bIsLeftInput, bSingleProjectile, NumPenetrable);
						},
						true, !bEnableMultiProjectile_L, MaxPenetrableObjectsNum_Left
					));
				}
				else if (LeftMouseAction == EWeaponAction::WeaponAction_BurstShot)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						LeftBurstShotAction,
						ETriggerEvent::Started,
						[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
						{
							HandleBurstFire(bIsLeftInput, bSingleProjectile, NumPenetrable);
						},
						true, !bEnableMultiProjectile_L, MaxPenetrableObjectsNum_Left
					));
				}
				else if (LeftMouseAction == EWeaponAction::WeaponAction_FullAutoShot)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						LeftFullAutoShotAction,
						ETriggerEvent::Triggered,
						[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
						{
							StartFullAutoShot(bIsLeftInput, bSingleProjectile, NumPenetrable);
						},
						true, !bEnableMultiProjectile_L, MaxPenetrableObjectsNum_Left
					));
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(LeftFullAutoShotAction, ETriggerEvent::Completed, this, &AWeaponTargeting::StopFullAutoShot));
				}
				else if (LeftMouseAction == EWeaponAction::WeaponAction_Charge)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(LeftChargeAction, ETriggerEvent::Triggered, this, &AWeaponTargeting::StartCharge));
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(LeftChargeAction, ETriggerEvent::Completed, this, &AWeaponTargeting::StopCharge));
				}
				else if (LeftMouseAction == EWeaponAction::WeaponAction_Hold)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(LeftHoldAction, ETriggerEvent::Triggered, this, &AWeaponTargeting::StartTargetDetection));
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						LeftHoldAction,
						ETriggerEvent::Completed,
						[this](const FInputActionValue& InputActionValue, FWeaponFireParams* InFireData)
						{
							StopTargetDetection(InFireData);
						},
						&FireData_L
					));
				}
				else if (LeftMouseAction == EWeaponAction::WeaponAction_Zoom)
				{
					// No Left Zoom
				}
				else if (LeftMouseAction == EWeaponAction::WeaponAction_SkillFire)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						LeftSingleShotAction,
						ETriggerEvent::Started,
						[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
						{
							HandleTargetingSkillFire(bIsLeftInput, bSingleProjectile, NumPenetrable);
						},
						true, !bEnableMultiProjectile_L, MaxPenetrableObjectsNum_Left
					));
				}

				// <RightMouseAction>
				if (RightMouseAction == EWeaponAction::WeaponAction_SingleShot)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						RightSingleShotAction,
						ETriggerEvent::Started,
						[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
						{
							HandleSingleFire(bIsLeftInput, bSingleProjectile, NumPenetrable);
						},
						false, !bEnableMultiProjectile_R, MaxPenetrableObjectsNum_Right
					));
				}
				else if (RightMouseAction == EWeaponAction::WeaponAction_BurstShot)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						RightBurstShotAction,
						ETriggerEvent::Started,
						[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
						{
							HandleBurstFire(bIsLeftInput, bSingleProjectile, NumPenetrable);
						},
						false, !bEnableMultiProjectile_R, MaxPenetrableObjectsNum_Right
					));
				}
				else if (RightMouseAction == EWeaponAction::WeaponAction_FullAutoShot)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						RightFullAutoShotAction,
						ETriggerEvent::Started,
						[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
						{
							StartFullAutoShot(bIsLeftInput, bSingleProjectile, NumPenetrable);
						},
						false, !bEnableMultiProjectile_R, MaxPenetrableObjectsNum_Right
					));
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightFullAutoShotAction, ETriggerEvent::Completed, this, &AWeaponTargeting::StopFullAutoShot));
				}
				else if (RightMouseAction == EWeaponAction::WeaponAction_Charge)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightChargeAction, ETriggerEvent::Triggered, this, &AWeaponTargeting::StartCharge));
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightChargeAction, ETriggerEvent::Completed, this, &AWeaponTargeting::StopCharge));
				}
				else if (RightMouseAction == EWeaponAction::WeaponAction_Hold)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightHoldAction, ETriggerEvent::Triggered, this, &AWeaponTargeting::StartTargetDetection));
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						RightHoldAction,
						ETriggerEvent::Completed,
						[this](const FInputActionValue& InputActionValue, FWeaponFireParams* InFireData)
						{
							StopTargetDetection(InFireData);
						},
						&FireData_R
					));
				}
				else if (RightMouseAction == EWeaponAction::WeaponAction_Zoom)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(RightZoomAction, ETriggerEvent::Started, this, &AWeaponTargeting::ZoomToggle));
				}
				else if (RightMouseAction == EWeaponAction::WeaponAction_SkillFire)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindActionValueLambda(
						RightSingleShotAction,
						ETriggerEvent::Started,
						[this](const FInputActionValue& InputActionValue, bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
						{
							HandleTargetingSkillFire(bIsLeftInput, bSingleProjectile, NumPenetrable);
						},
						false, !bEnableMultiProjectile_R, MaxPenetrableObjectsNum_Right
					));
				}

				// <Skill>
				if (SkillAction == EWeaponAction::WeaponAction_SkillToggle)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(SkillToggleAction, ETriggerEvent::Started, this, &AWeaponTargeting::HandleTargetDetectionSkill));
				}

				// <Reload>
				if (bActivePumpActionReload)
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AWeaponTargeting::HandlePumpActionReload));
				}
				else
				{
					InputActionBindingHandles.Add(&EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AWeaponTargeting::HandleReload));
				}
			}
		}
	}
}

bool AWeaponTargeting::AttachWeaponToPlayer(ACharacterPlayer* TargetCharacter)
{
	Character = TargetCharacter;
	if (Character == nullptr) { return false; }

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);


	if (WeaponSocketName.IsNone())
	{
		AttachToComponent(Character->GetArmMesh(), AttachmentRules, FName(TEXT("Gun")));
	}
	else
	{
		AttachToComponent(Character->GetArmMesh(), AttachmentRules, WeaponSocketName);
	}

	// Seunghwan - use unreal first person rendering
	WeaponMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	TArray<USceneComponent*> AttachedChildren;
	WeaponMesh->GetChildrenComponents(true, AttachedChildren);
	for (auto Child : AttachedChildren)
	{
		if (UPrimitiveComponent* PrimitiveChild = Cast<UPrimitiveComponent>(Child))
		{
			PrimitiveChild->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
		}
	}


	//---------------------------------------------
	//RightHandToAimSocketOffset = this->GetSocketLocation(FName(TEXT("Aim"))) - Character->GetMesh()->GetSocketLocation(FName("Gun"));
	RightHandToAimSocketOffset = WeaponMesh->GetSocketLocation(FName(TEXT("Aim"))) - Character->GetArmMesh()->GetBoneLocation(FName(TEXT("hand_r")));
	if (Character->GetWeaponSystemComponent() && Character->GetWeaponSystemComponent()->GetClass()->ImplementsInterface(UWeaponInterface::StaticClass()))
	{
		Character->GetWeaponSystemComponent()->SetRightHandToAimSocketOffset(RightHandToAimSocketOffset);
	}

	// Set AimSocketTransform
	SetAimSocketTransform();

	// Set Up Widget UI Class
	if (bIsSkillWeapon) { ActivateAimUIWidget(false); }
	else { ActivateAimUIWidget(true); }
	ActivateAmmoCounterWidget(true);
	ActivateTargetingSkillWidget(true);

	SetMeshVisibility(true);
	return true;
}

void AWeaponTargeting::FireSingleProjectile(FWeaponFireParams* FireData, int32 NumPenetrable, float AdditionalDamage, float AdditionalRecoilAmountPitch, float AdditionalRecoilAmountYaw, float AdditionalProjectileRadius, bool bIsHoming, AActor* HomingTarget)
{
	//UE_LOG(LogTemp, Warning, TEXT("FireSingleProjectile_Pool"));

	if (CurrentState == UnequippedState) return;
	if (!FireData) return;
	if (!Character) return;
	if (Character->GetController() == nullptr) return;

	const auto* Cam = Character->GetCameraComponent();
	if (!Cam) return;

	if (FireData->AmmoCost > 0)
	{
		if (FireData->bAllowFireWithInsufficientAmmo)
		{
			if (LeftAmmoInCurrentMag <= 0)
			{
				return;
			}
		}
		else
		{
			if (!HasAmmoInCurrentMag(FireData->AmmoCost))
			{
				return;
			}
		}
		ConsumeAmmo(FireData->AmmoCost, FireData->bAllowFireWithInsufficientAmmo);
	}

	FVector ProjectileStartLocation = Cam->GetComponentLocation();
	FVector ProjectileDirection = Cam->GetForwardVector();

	if (bIsZoomIn)
	{
		if (ZoomSpread.bEnableProjectileSpread)
		{
			ProjectileDirection = GetRandomSpreadVector(ProjectileDirection);
		}
		if (ZoomSpread.bEnableProjectileSpread || ZoomSpread.bEnableAimUISpread)
		{
			AddSpreadValue(&ZoomSpread);
		}
	}
	else
	{
		if (DefaultSpread.bEnableProjectileSpread)
		{
			ProjectileDirection = GetRandomSpreadVector(ProjectileDirection);
		}
		if (DefaultSpread.bEnableProjectileSpread || DefaultSpread.bEnableAimUISpread)
		{
			AddSpreadValue(&DefaultSpread);
		}
	}

	if (FireData->ProjectileClass != nullptr)
	{
		const FVector SpawnLocation = ProjectileStartLocation;
		FVector MuzzleLocation;
		if (WeaponMesh->DoesSocketExist(FName(TEXT("Muzzle"))))
		{
			MuzzleLocation = WeaponMesh->GetSocketLocation(FName(TEXT("Muzzle")));
		}
		else
		{
			MuzzleLocation = MuzzlePoint->GetComponentLocation();
		}

		const FRotator SpawnRotation = ProjectileDirection.Rotation();

		AProjectile* Projectile = GetProjectileFromPool(FireData->ProjectileClass);
		Projectile->InitProjectile(Character, this, AdditionalDamage, AdditionalProjectileRadius, NumPenetrable);
		SetUpAimUIDelegateBinding(Projectile); //TODO: 중복바인딩 방지
		if (bIsHoming)
		{
			Projectile->SetHomingTarget(bIsHoming, HomingTarget);
			Projectile->LaunchProjectile(MuzzleLocation, MuzzlePoint->GetComponentRotation());
		}
		else
		{
			Projectile->InitProjectileMovement(ProjectileStartLocation, ProjectileDirection, MuzzleLocation);
		}

		if (bWeaponAssetsReady)
		{
			//TODO: SpawnRotation 정상화
			SpawnMuzzleFireEffect(FireData->MuzzleFireEffect, MuzzleLocation, SpawnRotation);
		}
	}


	if (FireData->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireData->FireSound, Character->GetActorLocation());
	}

	StartFireAnimation(AM_Fire_Character, AM_Fire_Weapon);

	// <Overheat> //TODO: Delete
	if (bIsOverheatMode)
	{
		AddOverheatValue();
	}

	// <Recoil & CamShake>
	if (bIsZoomIn)
	{
		AddRecoilValue(&ZoomRecoil, AdditionalRecoilAmountPitch, AdditionalRecoilAmountYaw);
		ApplyCameraShake(ZoomCameraShakeClass);
	}
	else
	{
		AddRecoilValue(&FireData->Recoil, AdditionalRecoilAmountPitch, AdditionalRecoilAmountYaw);
		ApplyCameraShake(FireData->CamShake);
	}

	// <ArmRecoil Animation>
	if (bIsSkillWeapon) { AddSkillWeaponRecoil(&FireData->Armrecoil); }
	else { AddArmRecoil(&FireData->Armrecoil); }
}

void AWeaponTargeting::ActivateTargetingSkillWidget(bool bflag)
{
	if (bflag)
	{
		if (TargetingSkillWidget)
		{
			TargetingSkillWidget->AddToViewport();
		}
	}
	else
	{
		if (TargetingSkillWidget)
		{
			TargetingSkillWidget->RemoveFromViewport();
		}
	}
}

#pragma region FireMode/Targeting
void AWeaponTargeting::StartTargetDetection()
{
	if (CurrentState == IdleState)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Start Target Detection!!!"));

		ChangeState(TargetingState);
		PlayWeaponSound(TargetSearchLoopSound);
		UpdateTargetDetection(GetWorld()->GetDeltaSeconds());
	}
}
void AWeaponTargeting::UpdateTargetDetection(float DeltaTime)
{
	ElapsedTimeAfterTargetingStarted += DeltaTime;

	TArray<AActor*> NewOverlappedActors;

	CurrentTargetDetectionRadius = (FMath::Clamp(ElapsedTimeAfterTargetingStarted, 0.f, TimeToReachMaxTargetDetectionRange) / TimeToReachMaxTargetDetectionRange) * MaxTargetDetectionRadius;
	CurrentTargetDetectionAngle = (FMath::Clamp(ElapsedTimeAfterTargetingStarted, 0.f, TimeToReachMaxTargetDetectionRange) / TimeToReachMaxTargetDetectionRange) * MaxTargetDetectionAngle;

	SearchOverlappedActor(Character->GetActorLocation(), CurrentTargetDetectionRadius, NewOverlappedActors);


	//TODO: Targets�� ���� Update�� �ʿ���. �׾����� Targets���� ���ܽ��Ѿ���
	// TargetMarker�� ��쿡�� Visibility�� false�� �ٲ��ִ� ������ �����ϱ�

	for (TSet<AActor*>::TIterator It = Targets.CreateIterator(); It; ++It)
	{
		AActor* PreviousTarget = *It;
		if (IsValid(PreviousTarget) && CheckIfTargetIsBlockedByObstacle(PreviousTarget))
		{
			UTargetMarkerWidget** TargetMarkerPtr = MapTargetActorToWidget.Find(PreviousTarget);
			(*TargetMarkerPtr)->RemoveFromViewport();
			(*TargetMarkerPtr)->RemoveFromParent();
			TargetMarkerWidgets.Remove(*TargetMarkerPtr);

			It.RemoveCurrent();
			MapTargetActorToWidget.Remove(PreviousTarget);
		}
	}

	for (AActor* NewOverlappedActor : NewOverlappedActors)
	{
		if (Targets.Num() >= MaxTargetNum || ElapsedTimeAfterTargetingStarted > MaxTargetDetectionTime)
		{
			break;
		}
		if (!Targets.Contains(NewOverlappedActor))
		{
			if (IsInViewport(GetScreenPositionOfWorldLocation(NewOverlappedActor->GetActorLocation()).Get<0>(), 1.f, 1.f))
			{
				if (GetUnsignedAngleBetweenVectors(Character->GetActorForwardVector(), NewOverlappedActor->GetActorLocation() - Character->GetActorLocation(), FVector::ZAxisVector) < CurrentTargetDetectionAngle)
				{
					if (!CheckIfTargetIsBlockedByObstacle(NewOverlappedActor))
					{
						Targets.Add(NewOverlappedActor);
						UTargetMarkerWidget* NewTargetMarker = CreateTargetMarkerWidget(NewOverlappedActor);
						if (NewTargetMarker)
						{
							TargetMarkerWidgets.Add(NewTargetMarker);
							NewTargetMarker->AddToViewport();
							NewTargetMarker->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
							NewTargetMarker->SetVisibility(ESlateVisibility::Hidden);
						}

						if (TargetLockedSound)
						{
							UGameplayStatics::PlaySoundAtLocation(this, TargetLockedSound, Character->GetActorLocation());
						}
					}
				}
			}
		}
	}


	float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	//GetWorld()->GetTimerManager().SetTimer(TargetDetectionTimer, [this, DeltaSeconds]() {UpdateTargetDetection(DeltaSeconds); }, DeltaSeconds, false);
	TWeakObjectPtr WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(TargetDetectionTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis, DeltaSeconds]()
		{
			if (auto* HardThis = WeakThis.Get())
			{
				HardThis->UpdateTargetDetection(DeltaSeconds);
			}

		}), DeltaSeconds, false);


}
void AWeaponTargeting::StopTargetDetection(FWeaponFireParams* FireData)
{
	if (CurrentState == TargetingState)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Stop Target Detection!!!"));

		GetWorld()->GetTimerManager().ClearTimer(TargetDetectionTimer);

		ElapsedTimeAfterTargetingStarted = 0.f;
		CurrentTargetDetectionRadius = 0.f;
		CurrentTargetDetectionAngle = 0.f;

		ResetTargetMarkers();

		StopWeaponSound();

		TArray<AActor*> TargetsArray = Targets.Array();
		Targets.Empty();
		StartMissileLaunch(TargetsArray, FireData);
	}
}

bool AWeaponTargeting::SearchOverlappedActor(FVector CenterLocation, float SearchRadius, TArray<AActor*>& OverlappedActors)
{
	TArray<TEnumAsByte<EObjectTypeQuery>> traceObjectTypes;
	traceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	traceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_GameTraceChannel6));
	TArray<AActor*> ignoreActors;
	ignoreActors.Init(Character, 1);
	bool bIsAnyActorExist = UKismetSystemLibrary::SphereOverlapActors(GetWorld(), CenterLocation, SearchRadius, traceObjectTypes, nullptr, ignoreActors, OverlappedActors);

	return bIsAnyActorExist;
}
TTuple<FVector2D, bool> AWeaponTargeting::GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const
{
	FVector2D ScreenLocation = FVector2D::ZeroVector;
	bool bResult = UGameplayStatics::ProjectWorldToScreen(CharacterController, SearchLocation, ScreenLocation);

	return MakeTuple(ScreenLocation, bResult);
}
bool AWeaponTargeting::IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width, float ScreenRatio_Height) const
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
float AWeaponTargeting::GetUnsignedAngleBetweenVectors(const FVector& VectorA, const FVector& VectorB, const FVector& Axis)
{
	FVector NormalizedA = VectorA.GetSafeNormal();
	FVector NormalizedB = VectorB.GetSafeNormal();

	//�� ���� ������ ȸ���� ��Ÿ���� ���ʹϾ� ����
	FQuat Quat = FQuat::FindBetweenNormals(NormalizedA, NormalizedB);

	//ȸ�� �࿡ ���� ���⼺�� ����� ���� ���
	float Angle = Quat.GetAngle();  // �⺻������ ���� ������ ������ ��ȯ

	//���ʹϾ��� �࿡ �����Ͽ� ���� ����
	FVector RotationAxis = Quat.GetAxisZ();  // ���ʹϾ��� ȸ�� ���� ���� (���� X, Y, Z �� �ϳ��� ���� ����)

	float DotProduct = FVector::DotProduct(RotationAxis, Axis);

	// DotProduct�� ����� ������ �״��, ������ �ݴ� �������� ���� ��ȣ�� ����
	float SignedAngle = Angle * FMath::Sign(DotProduct);

	//������ �� ������ ��ȯ
	return FMath::RadiansToDegrees(Angle);
}
bool AWeaponTargeting::CheckIfTargetIsBlockedByObstacle(AActor* target)
{
	float Radius = 10.f;

	FVector StartPoint = Character->GetActorLocation();
	FVector EndPoint = target->GetActorLocation();

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));

	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;
	QueryParams.AddIgnoredActor(Character);

	bool bHit = GetWorld()->SweepMultiByObjectType(
		HitResults,
		StartPoint,
		EndPoint,
		FQuat::Identity,
		FCollisionObjectQueryParams(ObjectTypes),
		FCollisionShape::MakeSphere(Radius),
		QueryParams
	);
	return bHit;
}

UTargetMarkerWidget* AWeaponTargeting::CreateTargetMarkerWidget(AActor* TargetActor)
{
	if (TargetMarkerWidgetClass)
	{
		UTargetMarkerWidget* NewTargetMarkerWidget = CreateWidget<UTargetMarkerWidget>(GetWorld(), TargetMarkerWidgetClass);
		MapTargetActorToWidget.Add(TargetActor, NewTargetMarkerWidget);

		return NewTargetMarkerWidget;
	}
	return nullptr;
}
void AWeaponTargeting::UpdateTargetMarkers()
{
	for (AActor* Target : Targets)
	{
		FVector TargetLocation = Target->GetActorLocation();
		//FVector TargetOffset(0.f, 0.f, 50.f);

		UTargetMarkerWidget** TargetMarkerPtr = MapTargetActorToWidget.Find(Target);

		USkeletalMeshComponent* TargetSkeletalMesh = Target->GetComponentByClass<USkeletalMeshComponent>();
		if (TargetSkeletalMesh && TargetSkeletalMesh->DoesSocketExist(FName(TEXT("spine_03"))))
		{
			TargetLocation = TargetSkeletalMesh->GetBoneLocation(FName(TEXT("spine_03")));
		}

		FVector2D TargetScreenPosition = GetScreenPositionOfWorldLocation(TargetLocation).Get<0>();

		if (IsInViewport(TargetScreenPosition, 1.f, 1.f))
		{
			(*TargetMarkerPtr)->SetPositionInViewport(TargetScreenPosition);

			if ((*TargetMarkerPtr)->Visibility == ESlateVisibility::Hidden)
			{
				(*TargetMarkerPtr)->SetVisibility(ESlateVisibility::Visible);
			}
		}
		else
		{
			if ((*TargetMarkerPtr)->Visibility == ESlateVisibility::Visible)
			{
				(*TargetMarkerPtr)->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}
void AWeaponTargeting::ResetTargetMarkers()
{
	for (AActor* Target : Targets)
	{
		UTargetMarkerWidget** TargetMarkerPtr = MapTargetActorToWidget.Find(Target);

		(*TargetMarkerPtr)->RemoveFromViewport();
		(*TargetMarkerPtr)->RemoveFromParent();
	}
	TargetMarkerWidgets.Empty();
	MapTargetActorToWidget.Empty();
}

void AWeaponTargeting::StartMissileLaunch(TArray<AActor*> TargetActors, FWeaponFireParams* FireData)
{
	ConfirmedTargets = TargetActors;
	CurrentTargetIndex = 0;
	if (ConfirmedTargets.Num() == 0)
	{
		StopMissileLaunch();
	}
	else
	{
		ChangeState(FiringState);
		UpdateMissileLaunch(FireData);
	}
}
void AWeaponTargeting::UpdateMissileLaunch(FWeaponFireParams* FireData)
{
	//FireSingleProjectile(FireData, 0, 0.f, 0.f, 0.f, 0.f, true, ConfirmedTargets[CurrentTargetIndex]);
	FireSingleProjectile(FireData, 0, 0.f, 0.f, 0.f, 0.f, true, ConfirmedTargets[CurrentTargetIndex]);
	CurrentTargetIndex++;
	if (ConfirmedTargets.Num() <= CurrentTargetIndex)
	{
		StopMissileLaunch();
	}
	else
	{
		//GetWorld()->GetTimerManager().SetTimer(MissileLaunchTimer, [this, FireData] {UpdateMissileLaunch(FireData); }, MissileLaunchDelay, false);
		TWeakObjectPtr WeakThis = this;
		GetWorld()->GetTimerManager().SetTimer(MissileLaunchTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis, FireData]()
			{
				if (auto* HardThis = WeakThis.Get())
				{
					HardThis->UpdateMissileLaunch(FireData);
				}

			}), MissileLaunchDelay, false);
	}
}
void AWeaponTargeting::StopMissileLaunch()
{
	ConfirmedTargets.Empty();
	CurrentTargetIndex = 0;

	ReleaseControl();
	ChangeState(IdleState);
}
#pragma endregion

#pragma region Skill/Targeting
void AWeaponTargeting::HandleTargetDetectionSkill()
{
	if (CurrentState == IdleState && bCanUseTargetingSkill)
	{
		if (!TryTakeControl()) { return; };
		ChangeState(TargetingState);
		PlayWeaponSound(TargetSearchLoopSound);
		if (TargetingSkillWidget) { TargetingSkillWidget->SetDetectionTimeUIVisible(true); }
		UpdateTargetDetectionSkill(GetWorld()->GetDeltaSeconds());

		//suhyeon
		//UE_LOG(LogTemp, Warning, TEXT("RocketLauncherSkillActivated 델리게이트 호출됨"));
		OnRocketLauncherSkillActivated.Broadcast();

	}
	else if (CurrentState == TargetingState)
	{
		CancelTargetingSkill();
	}
}
void AWeaponTargeting::UpdateTargetDetectionSkill(float DeltaTime)
{
	ElapsedTimeAfterTargetingStarted += DeltaTime;
	TArray<AActor*> NewOverlappedActors;
	CurrentTargetDetectionRadius = (FMath::Clamp(ElapsedTimeAfterTargetingStarted, 0.f, TimeToReachMaxTargetDetectionRange) / TimeToReachMaxTargetDetectionRange) * MaxTargetDetectionRadius;
	CurrentTargetDetectionAngle = (FMath::Clamp(ElapsedTimeAfterTargetingStarted, 0.f, TimeToReachMaxTargetDetectionRange) / TimeToReachMaxTargetDetectionRange) * MaxTargetDetectionAngle;
	SearchOverlappedActor(Character->GetActorLocation(), CurrentTargetDetectionRadius, NewOverlappedActors);

	//TODO: Targets�� ���� Update�� �ʿ���. �׾����� Targets���� ���ܽ��Ѿ���
	// TargetMarker�� ��쿡�� Visibility�� false�� �ٲ��ִ� ������ �����ϱ�

	for (TSet<AActor*>::TIterator It = Targets.CreateIterator(); It; ++It)
	{
		AActor* PreviousTarget = *It;
		if (IsValid(PreviousTarget) && CheckIfTargetIsBlockedByObstacle(PreviousTarget))
		{
			UTargetMarkerWidget** TargetMarkerPtr = MapTargetActorToWidget.Find(PreviousTarget);
			(*TargetMarkerPtr)->RemoveFromViewport();
			(*TargetMarkerPtr)->RemoveFromParent();
			TargetMarkerWidgets.Remove(*TargetMarkerPtr);

			It.RemoveCurrent();
			MapTargetActorToWidget.Remove(PreviousTarget);
		}
	}

	for (AActor* NewOverlappedActor : NewOverlappedActors)
	{
		if (Targets.Num() >= MaxTargetNum || ElapsedTimeAfterTargetingStarted > MaxTargetDetectionTime)
		{
			break;
		}
		if (!Targets.Contains(NewOverlappedActor))
		{
			if (IsInViewport(GetScreenPositionOfWorldLocation(NewOverlappedActor->GetActorLocation()).Get<0>(), 1.f, 1.f))
			{
				if (GetUnsignedAngleBetweenVectors(Character->GetActorForwardVector(), NewOverlappedActor->GetActorLocation() - Character->GetActorLocation(), FVector::ZAxisVector) < CurrentTargetDetectionAngle)
				{
					if (!CheckIfTargetIsBlockedByObstacle(NewOverlappedActor))
					{
						Targets.Add(NewOverlappedActor);
						UTargetMarkerWidget* NewTargetMarker = Cast<UTargetMarkerWidget>(CreateTargetMarkerWidget(NewOverlappedActor));
						if (NewTargetMarker)
						{
							float TargetingReadyDuration = 0.3f; // default
							float TargetingSuccessDuration = 0.3f; // default
							//if (Character)
							//{
							//	UACUIMangerComponent* UIManager = Character->GetUIManager();
							//	if (UIManager)
							//	{
							//		UDataTable* UIDataTable = UIManager->GetUIDataTable();
							//		if (UIDataTable)
							//		{
							//			if (const FUIData* Row = UIDataTable->FindRow<FUIData>(TEXT("TargetingReady"), TEXT("")))
							//			{
							//				TargetingReadyDuration = Row->AnimDuration;
							//			}

							//			if (const FUIData* Row = UIDataTable->FindRow<FUIData>(TEXT("TargetingSuccess"), TEXT("")))
							//			{
							//				TargetingSuccessDuration = Row->AnimDuration;
							//			}
							//		}
							//	}
							//}
							NewTargetMarker->StartLockOnProcess(TargetingReadyDuration, TargetingSuccessDuration);

							MapTargetActorToWidget.Add(NewOverlappedActor, NewTargetMarker);
							TargetMarkerWidgets.Add(NewTargetMarker);
							NewTargetMarker->AddToViewport();
							NewTargetMarker->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
							NewTargetMarker->SetVisibility(ESlateVisibility::Hidden);
						}

						if (TargetLockedSound)
						{
							UGameplayStatics::PlaySoundAtLocation(this, TargetLockedSound, Character->GetActorLocation());
						}
					}
				}
			}
		}
	}

	if (TargetingSkillWidget)
	{
		TargetingSkillWidget->SetTargetingTimeUI(FMath::Clamp(ElapsedTimeAfterTargetingStarted, 0.f, MaxTargetingTime));
	}

	if (ElapsedTimeAfterTargetingStarted >= MaxTargetingTime)
	{
		CancelTargetingSkill();
	}
	else
	{
		float DeltaSeconds = GetWorld()->GetDeltaSeconds();
		//GetWorld()->GetTimerManager().SetTimer(TargetDetectionTimer, [this, DeltaSeconds]() {UpdateTargetDetectionSkill(DeltaSeconds); }, DeltaSeconds, false);
		TWeakObjectPtr WeakThis = this;
		GetWorld()->GetTimerManager().SetTimer(TargetDetectionTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis, DeltaSeconds]()
			{
				if (auto* HardThis = WeakThis.Get())
				{
					HardThis->UpdateTargetDetectionSkill(DeltaSeconds);
				}

			}), DeltaSeconds, false);
	}
}
void AWeaponTargeting::HandleTargetingSkillFire(bool bIsLeftInput, bool bSingleProjectile, int32 NumPenetrable)
{
	if (CurrentState == TargetingState)
	{
		GetWorld()->GetTimerManager().ClearTimer(TargetDetectionTimer);

		ElapsedTimeAfterTargetingStarted = 0.f;
		CurrentTargetDetectionRadius = 0.f;
		CurrentTargetDetectionAngle = 0.f;

		//ResetTargetMarkers();
		StopWeaponSound();

		if (TargetingSkillWidget) { TargetingSkillWidget->SetDetectionTimeUIVisible(false); }

		//TArray<AActor*> TargetsArray = Targets.Array();

		// 락온이 완료된 타겟만 저장할 새로운 배열 선언
		TArray<AActor*> LockedOnTargets;

		// 모든 타겟을 순회하면 락온된 타겟만 선별
		for (AActor* Target : Targets)
		{
			UTargetMarkerWidget** TargetMarkerPtr = MapTargetActorToWidget.Find(Target);
			if (TargetMarkerPtr && *TargetMarkerPtr)
			{
				UTargetMarkerWidget* TargetMarker = Cast<UTargetMarkerWidget>(*TargetMarkerPtr);

				if (TargetMarker && TargetMarker->bIsLockedOn)
				{
					LockedOnTargets.Add(Target);
				}
			}
		}
		ResetTargetMarkers();
		Targets.Empty();

		bCanUseTargetingSkill = false;
		bool bflag = true;

		TWeakObjectPtr WeakThis = this;
		GetWorld()->GetTimerManager().SetTimer(TargetingSkillTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis, bflag]()
			{
				if (auto* HardThis = WeakThis.Get())
				{
					HardThis->EnableTargetingSkill(bflag);
				}
			}), TargetingSkillCoolDown, false);

		if (LockedOnTargets.Num() == 0)
		{
			StartMissileLaunch(LockedOnTargets, &FireData_Skill);
			if (CurrentState == IdleState)
			{
				ChangeState(FiringState);
				StartSingleShot(bIsLeftInput, bSingleProjectile, NumPenetrable);
			}
		}
		else
		{
			StartMissileLaunch(LockedOnTargets, &FireData_Skill);
		}
	}
	else if (CurrentState == IdleState)
	{
		if (!bAllowNormalFireForSkillWeapon) { return; }
		ChangeState(FiringState);
		StartSingleShot(bIsLeftInput, bSingleProjectile, NumPenetrable);
	}
	else if (CurrentState == PumpActionReloadingState)
	{
		if (!bAllowNormalFireForSkillWeapon) { return; }
		BufferedFireRequest = FBufferedFireRequest(EWeaponAction::WeaponAction_SingleShot, bIsLeftInput, bSingleProjectile, NumPenetrable);
	}
	//suhyeon
	// 스킬 취소 시 델리게이트를 브로드캐스트하여 위젯에 알림
	OnRocketLauncherSkillOvered.Broadcast();
}
void AWeaponTargeting::CancelTargetingSkill()
{
	GetWorld()->GetTimerManager().ClearTimer(TargetDetectionTimer);

	ElapsedTimeAfterTargetingStarted = 0.f;
	CurrentTargetDetectionRadius = 0.f;
	CurrentTargetDetectionAngle = 0.f;

	ResetTargetMarkers();
	StopWeaponSound();

	Targets.Empty();

	ConfirmedTargets.Empty();
	CurrentTargetIndex = 0;

	if (TargetingSkillWidget) { TargetingSkillWidget->SetDetectionTimeUIVisible(false); }

	ReleaseControl();
	ChangeState(IdleState);

	//suhyeon
	// 스킬 취소 시 델리게이트를 브로드캐스트하여 위젯에 알림
	OnRocketLauncherSkillOvered.Broadcast();
}
void AWeaponTargeting::EnableTargetingSkill(bool bflag)
{
	bCanUseTargetingSkill = bflag;

	if (TargetingSkillWidget && IsValid(TargetingSkillWidget))
	{
		TargetingSkillWidget->SetTargetingSkillCoolDown(TargetingSkillCoolDown);
	}
}
void AWeaponTargeting::UpdateTargetingSkillUI()
{
	if (GetWorld()->GetTimerManager().IsTimerActive(TargetingSkillTimer))
	{
		float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(TargetingSkillTimer);
		ElapsedTime = FMath::Clamp(ElapsedTime, 0.f, TargetingSkillCoolDown);

		if (TargetingSkillWidget && IsValid(TargetingSkillWidget))
		{
			TargetingSkillWidget->SetTargetingSkillCoolDown(ElapsedTime);
		}
	}
}
bool AWeaponTargeting::TryTakeControl()
{
	if (!Character) { return false; }
	UWeaponSystemComponent* WSC = Character->GetWeaponSystemComponent();
	if (!WSC) { return false; }
	return WSC->TryTakeControl(this);
}
void AWeaponTargeting::ReleaseControl()
{
	if (!Character) { return; }
	UWeaponSystemComponent* WSC = Character->GetWeaponSystemComponent();
	if (!WSC) { return; }
	WSC->ReleaseControl();
}
void AWeaponTargeting::SetGlobalTimeDilation(float targettimescale)
{
	TargetGlobalTimeScale = targettimescale;
	bIsGlobalTimeScaleChanging = true;
}
void AWeaponTargeting::UpdateGlobalTimeDiation(float DeltaTime)
{
	if (bIsGlobalTimeScaleChanging)
	{
		float Current = GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation();
		float New;
		if (TargetGlobalTimeScale <= Current)
		{
			New = FMath::FInterpTo(Current, TargetGlobalTimeScale, DeltaTime, TargetingGlobalTimeDilationSpeed_In);
		}
		else
		{
			New = FMath::FInterpTo(Current, TargetGlobalTimeScale, DeltaTime, TargetingGlobalTimeDilationSpeed_Out);
		}

		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), New);

		if (FMath::IsNearlyEqual(New, TargetGlobalTimeScale, 0.1f))
		{
			//UE_LOG(LogTemp, Warning, TEXT("Global Time Scale is Stabilized!!!"));
			bIsGlobalTimeScaleChanging = false;
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), TargetGlobalTimeScale);
		}
	}
}
#pragma endregion