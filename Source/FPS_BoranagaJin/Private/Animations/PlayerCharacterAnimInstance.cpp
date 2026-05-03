


#include "Animations/PlayerCharacterAnimInstance.h"

#include "Characters/Player/CharacterPlayer.h"

#include "Items/Weapons/WeaponSystemComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Items/WeaponState/WeaponBaseState.h"

#include "KismetAnimationLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"


void UPlayerCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerCharacter = Cast<ACharacterPlayer>(TryGetPawnOwner());

	if (PlayerCharacter)
	{
		// <Character Movement>
		MovementState = PlayerCharacter->GetPlayerMovementComponent()->GetMovementState();
		bIsRunning = PlayerCharacter->GetPlayerMovementComponent()->IsRunning();
		bIsCrouching = PlayerCharacter->GetPlayerMovementComponent()->IsCrouching();
		bIsDashing = PlayerCharacter->GetPlayerMovementComponent()->IsDashing();
		Velocity = PlayerCharacter->GetPlayerMovementComponent()->Velocity;
		MovementInputVector = PlayerCharacter->GetPlayerMovementComponent()->GetMovementInputVector();


		//-------------------------------------------
		// <weapon System>
		LoadAnimationData();

		SetAimPoint();
		CurrentWeaponStateType = EWeaponStateType::WeaponStateType_None;

		if (PlayerCharacter->GetWeaponSystemComponent())
		{
			//SetAimSocket();
		}

		SpringDamperPosBuffer.Init(MLSBufferSize);

	}
}

void UPlayerCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (PlayerCharacter)
	{
		// <Character Movement>
		MovementState = PlayerCharacter->GetPlayerMovementComponent()->GetMovementState();
		bIsRunning = PlayerCharacter->GetPlayerMovementComponent()->IsRunning();
		bIsCrouching = PlayerCharacter->GetPlayerMovementComponent()->IsCrouching();
		bIsDashing = PlayerCharacter->GetPlayerMovementComponent()->IsDashing();
		Velocity = PlayerCharacter->GetPlayerMovementComponent()->Velocity;
		MovementInputVector = PlayerCharacter->GetPlayerMovementComponent()->GetMovementInputVector();
	}
}

void UPlayerCharacterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (PlayerCharacter)
	{
		// <Weapon System>
		bIsZoomIn = PlayerCharacter->GetWeaponSystemComponent()->IsZoomIn();
		//ScreenCenterWorldLocation = Character->GetWeaponSystem()->GetScreenCenterWorldPosition();
		//TargetRightHandWorldLocation = Character->GetWeaponSystem()->GetTargetRightHandWorldLocation();
		SetTargetRightHandTransform();


		//TODO: Update 말고 트리거로 작동하게 해야함
		//SetAimSocket();
		//SetAimPoint();
		if (PlayerCharacter->GetWeaponSystemComponent())
		{
			//SetAimSocket();
			UpdateRightHandSocket(DeltaTime);
			UpdateSkillWeaponSocket(DeltaTime);
			SetAimPoint();
			UpdateWeapon();

			if (bUseDampingRatio)
			{
				//UpdateSpringDamper_Upgrade_2(DeltaTime);
				//UpdateSpringDamper_Upgrade(DeltaTime);

				if (bUseMLS)
				{
					UpdateSpringDamper_MLS(DeltaTime);
				}
				else
				{
					UpdateSpringDamper_Upgrade(DeltaTime);
				}
			}
			else
			{
				UpdateSpringDamper(DeltaTime);
			}
			//UpdateSpringDamper(DeltaTime);
			//UpdateSpringDamper_Upgrade(DeltaTime);


			UpdateArmRecoil(DeltaTime);
			ConvertRecoilValueFrame();

			UpdateSkillWeaponRecoil(DeltaTime);
			ConvertSkillWeaponRecoilValueFrame();

			LeftHandTransform = GetLeftHandTransform();
			TargetLeftHandSocketTransform = GetTargetLeftHandTransfrom();
		}

		// DefalutCameraRelativeTransform = Character->GetCamera()->GetRelativeTransform();
		// LogTransform(AimSocketRelativeTransform);
	}
}

void UPlayerCharacterAnimInstance::LoadAnimationData()
{
	AnimationData = AnimationDataTableHandle.GetRow<FPlayerAnimationData>("");
	if (AnimationData)
	{
		DamperScale = AnimationData->DamperScale;
		Stiffness = AnimationData->Stiffness;
		Damping = AnimationData->Damping;

		DampingRatio = AnimationData->DampingRatio;
		HalfLife = AnimationData->HalfLife;

		bUseDampingRatio = AnimationData->bUseDampingRatio;
		bUseMLS = AnimationData->bUseMLS;
		MLSBufferSize = AnimationData->MLSBufferSize;

		ActiveRotationInterpSpeed_SkillWeapon = AnimationData->ActiveRotationInterpSpeed_SkillWeapon;
		ActiveLocationInterpSpeed_SkillWeapon = AnimationData->ActiveLocationInterpSpeed_SkillWeapon;
		InctiveRotationInterpSpeed_SkillWeapon = AnimationData->InctiveRotationInterpSpeed_SkillWeapon;
		InctiveLocationInterpSpeed_SkillWeapon = AnimationData->InctiveLocationInterpSpeed_SkillWeapon;
	}
}

void UPlayerCharacterAnimInstance::UpdateWeapon()
{
	if (IsValid(PlayerCharacter->GetWeaponSystemComponent())
		&& PlayerCharacter->GetWeaponSystemComponent() != nullptr)
	{
		if (PlayerCharacter->GetWeaponSystemComponent()->GetWeaponNum() != 0
			&& IsValid(PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon())
			&& PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon() != nullptr)
		{
			bHasWeapon = true;

			AimSocketRelativeTransform = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetAimSocketRelativeTransform();

			//-------------------------------------------------------------
			CurrentWeaponStateType = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetCurrentState()->GetWeaponStateType();

			//if (CurrentWeaponStateType == EWeaponStateType::WeaponStateType_Firing)
			//{
			//	UE_LOG(LogTemp, Warning, TEXT("Firing State!!!!!!!!!!"));
			//}
			//else if (CurrentWeaponStateType == EWeaponStateType::WeaponStateType_Reloading)
			//{
			//	UE_LOG(LogTemp, Warning, TEXT("Reloading State!!!!!!!!!!"));
			//}
			//-------------------------------------------------------------

			if (CurrentWeapon != PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon())
			{
				// <RightHandSocket>
				RightHandSocketTransform = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetRightHandSocketTransform();
				RightHandSocketTransform_Crouch = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetRightHandSocketTransform_Crouch();
				//RightHandSocketTransform_Targeting = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetRightHandSocketTransform_Targeting();
				//RightHandSocketTransform_Targeting_Crouch = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetRightHandSocketTransform_Targeting_Crouch();

				// <Recoil>
				ArmRecoil = *PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetArmRecoilInfo(); //TODO: 이제 AddArmRecoil에서 struct 정보 가져오기 때문에 이 줄은 없어도 될듯
				ArmRecoil_Hand = *PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetArmRecoilInfo_Hand();
				ArmRecoil_UpperArm = *PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetArmRecoilInfo_UpperArm();
				ArmRecoil_LowerArm = *PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetArmRecoilInfo_LowerArm();
			}

			if (CurrentSkillWeapon != PlayerCharacter->GetWeaponSystemComponent()->GetCurrentSkillWeapon())
			{
				// <SkillWeaponSocket>
				SkillWeaponSocketTransform_Active = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentSkillWeapon()->GetSkillWeaponSocketTransform_Active();
				SkillWeaponSocketTransform_Inactive = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentSkillWeapon()->GetSkillWeaponSocketTransform_Inactive();
			}
		}


		//if (CurrentWeapon != Character->GetWeaponSystem()->GetCurrentWeapon())
		//{
		//	CurrentWeapon = Character->GetWeaponSystem()->GetCurrentWeapon();

		//	//SetAimSocket();
		//	UE_LOG(LogTemp, Warning, TEXT("Weapon Updated!!!"));
		//}

		//if (Character->GetWeaponSystem()->IsZoomIn())
		//{
		//	SetAimSocket();
		//}
	}
}

void UPlayerCharacterAnimInstance::SetAimSocket()
{
	//TODO: Logic 손봐야함
	//if (IsValid(Character->GetWeaponSystem())
	//	&& Character->GetWeaponSystem() != nullptr)
	//{
	//	if (Character->GetWeaponSystem()->GetWeaponNum() != 0
	//		&& IsValid(Character->GetWeaponSystem()->GetCurrentWeapon())
	//		&& Character->GetWeaponSystem()->GetCurrentWeapon() != nullptr)
	//	{
	//		FTransform AimSocketTransform = Character->GetWeaponSystem()->GetCurrentWeapon()->GetSocketTransform(FName(TEXT("Aim")));
	//		FTransform RightHandTransform = Character->GetMesh()->GetSocketTransform(FName(TEXT("ik_hand_gun")));

	//		AimSocketRelativeTransform = AimSocketTransform.GetRelativeTransform(RightHandTransform);
	//	}
	//}
	//else
	//{
	//	FTransform AimSocketTransform = Character->GetMesh()->GetSocketTransform(FName(TEXT("ik_hand_gun")));
	//	FTransform RightHandTransform = Character->GetMesh()->GetSocketTransform(FName(TEXT("ik_hand_gun")));

	//	AimSocketRelativeTransform = AimSocketTransform.GetRelativeTransform(RightHandTransform);
	//}

	if (IsValid(PlayerCharacter->GetWeaponSystemComponent())
		&& PlayerCharacter->GetWeaponSystemComponent() != nullptr)
	{
		if (PlayerCharacter->GetWeaponSystemComponent()->GetWeaponNum() != 0
			&& IsValid(PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon())
			&& PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon() != nullptr)
		{
			FTransform AimSocketTransform = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetWeaponMesh()->GetSocketTransform(FName(TEXT("Aim")));
			FTransform IKHandGunTransform = PlayerCharacter->GetArmMesh()->GetSocketTransform(FName(TEXT("ik_hand_gun")));

			AimSocketRelativeTransform = AimSocketTransform.GetRelativeTransform(IKHandGunTransform);
			//AimSocketRelativeTransform = IKHandGunTransform.GetRelativeTransform(AimSocketTransform);
		}
	}
	else
	{
		FTransform AimSocketTransform = PlayerCharacter->GetArmMesh()->GetSocketTransform(FName(TEXT("ik_hand_gun")));
		FTransform IKHandGunTransform = PlayerCharacter->GetArmMesh()->GetSocketTransform(FName(TEXT("ik_hand_gun")));

		//AimSocketRelativeTransform = AimSocketTransform.GetRelativeTransform(IKHandGunTransform);
		AimSocketRelativeTransform = IKHandGunTransform.GetRelativeTransform(AimSocketTransform);
	}
}

void UPlayerCharacterAnimInstance::SetAimPoint()
{
	FTransform CameraTransform = PlayerCharacter->GetCameraComponent()->GetComponentTransform();
	//FTransform HandRootTransform = Character->GetMesh()->GetSocketTransform(FName(TEXT("ik_hand_root")));
	//FTransform HandRootTransform = Character->GetMesh()->GetBoneTransform(FName(TEXT("ik_hand_root")), ERelativeTransformSpace::RTS_World);
	FTransform HandRootTransform = PlayerCharacter->GetArmMesh()->GetComponentTransform();

	FTransform CameraRelativeTransform = CameraTransform.GetRelativeTransform(HandRootTransform);
	//FTransform CameraRelativeTransform = HandRootTransform.GetRelativeTransform(CameraTransform);

	float CamOffset = 20.f; //TODO: 임시로 설정했음. 설정가능한 변수로 바꾸기

	AimPointLocation = CameraRelativeTransform.GetLocation() + CameraRelativeTransform.GetRotation().GetForwardVector() * CamOffset;
	AimPointRotation = CameraRelativeTransform.GetRotation().Rotator();
}

void UPlayerCharacterAnimInstance::SetTargetRightHandTransform()
{
	if (IsValid(PlayerCharacter->GetWeaponSystemComponent())
		&& PlayerCharacter->GetWeaponSystemComponent() != nullptr)
	{
		if (PlayerCharacter->GetWeaponSystemComponent()->GetWeaponNum() != 0
			&& IsValid(PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon())
			&& PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon() != nullptr)
		{
			//FTransform RootTransform = Character->GetMesh()->GetBoneTransform(FName(TEXT("ik_hand_root")), ERelativeTransformSpace::RTS_World);
			FTransform RootTransform = PlayerCharacter->GetArmMesh()->GetComponentTransform();

			FTransform CameraTransform = PlayerCharacter->GetCameraComponent()->GetComponentTransform();
			FTransform WeaponAimSocketTransform = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon()->GetWeaponMesh()->GetSocketTransform(FName(TEXT("Aim")));
			FTransform RightHandTransform = PlayerCharacter->GetArmMesh()->GetBoneTransform(FName(TEXT("hand_r")));

			FTransform CameraRelativeTransform = CameraTransform.GetRelativeTransform(RootTransform);
			FTransform WeaponAimSocketRelativeTransform = WeaponAimSocketTransform.GetRelativeTransform(RootTransform);
			FTransform RightHandRelativeTransform = RightHandTransform.GetRelativeTransform(RootTransform);

			float CamOffset = 10.f; //TODO: 임시로 설정했음. 설정가능한 변수로 바꾸기

			AimPointRelativeLocation = CameraRelativeTransform.GetLocation() + CameraRelativeTransform.GetRotation().GetForwardVector() * CamOffset;
			AimPointRelativeRotation = CameraRelativeTransform.GetRotation().Rotator();

			HandTargetRelativeLocation = AimPointRelativeLocation + (RightHandRelativeTransform.GetLocation() - WeaponAimSocketRelativeTransform.GetLocation());
			HandTargetRelativeRotation = AimPointRelativeRotation + (RightHandRelativeTransform.GetRotation().Rotator() - WeaponAimSocketRelativeTransform.GetRotation().Rotator());
		}
	}
}


void UPlayerCharacterAnimInstance::LogTransform(const FTransform& Transform, const FString& TransformName)
{
	FVector Location = Transform.GetLocation();
	FRotator Rotation = Transform.Rotator();
	FVector Scale = Transform.GetScale3D();

	UE_LOG(LogTemp, Log, TEXT("%s: Location: X=%.2f, Y=%.2f, Z=%.2f | Rotation: Pitch=%.2f, Yaw=%.2f, Roll=%.2f | Scale: X=%.2f, Y=%.2f, Z=%.2f"),
		*TransformName,
		Location.X, Location.Y, Location.Z,
		Rotation.Pitch, Rotation.Yaw, Rotation.Roll,
		Scale.X, Scale.Y, Scale.Z);
}

FTransform UPlayerCharacterAnimInstance::GetLeftHandTransform()
{
	return PlayerCharacter->GetArmMesh()->GetSocketTransform(FName("hand_l"));
}

FTransform UPlayerCharacterAnimInstance::GetTargetLeftHandTransfrom() //TODO: 수정
{
	if (PlayerCharacter)
	{
		CurrentWeapon = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentWeapon();
		CurrentSkillWeapon = PlayerCharacter->GetWeaponSystemComponent()->GetCurrentSkillWeapon();
		if (CurrentWeapon)
		{
			return CurrentWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHand"));
		}
	}

	return FTransform();
}

#pragma region RightHandSocket
void UPlayerCharacterAnimInstance::UpdateRightHandSocket(float DeltaTime)
{
	if (CurrentWeapon && CurrentWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Waiting)
	{
		if (bIsCrouching)
		{
			CurrentRightHandSocketTransform.SetRotation((FMath::RInterpTo(CurrentRightHandSocketTransform.GetRotation().Rotator(), RightHandSocketTransform_Targeting_Crouch.GetRotation().Rotator(), DeltaTime, 10.f)).Quaternion());
			CurrentRightHandSocketTransform.SetTranslation(FMath::VInterpTo(CurrentRightHandSocketTransform.GetTranslation(), RightHandSocketTransform_Targeting_Crouch.GetTranslation(), DeltaTime, 10.f));
		}
		else
		{
			CurrentRightHandSocketTransform.SetRotation((FMath::RInterpTo(CurrentRightHandSocketTransform.GetRotation().Rotator(), RightHandSocketTransform_Targeting.GetRotation().Rotator(), DeltaTime, 10.f)).Quaternion());
			CurrentRightHandSocketTransform.SetTranslation(FMath::VInterpTo(CurrentRightHandSocketTransform.GetTranslation(), RightHandSocketTransform_Targeting.GetTranslation(), DeltaTime, 10.f));
		}
	}
	else
	{
		if (bIsCrouching)
		{
			CurrentRightHandSocketTransform.SetRotation((FMath::RInterpTo(CurrentRightHandSocketTransform.GetRotation().Rotator(), RightHandSocketTransform_Crouch.GetRotation().Rotator(), DeltaTime, 10.f)).Quaternion());
			CurrentRightHandSocketTransform.SetTranslation(FMath::VInterpTo(CurrentRightHandSocketTransform.GetTranslation(), RightHandSocketTransform_Crouch.GetTranslation(), DeltaTime, 10.f));
		}
		else
		{
			CurrentRightHandSocketTransform.SetRotation((FMath::RInterpTo(CurrentRightHandSocketTransform.GetRotation().Rotator(), RightHandSocketTransform.GetRotation().Rotator(), DeltaTime, 10.f)).Quaternion());
			CurrentRightHandSocketTransform.SetTranslation(FMath::VInterpTo(CurrentRightHandSocketTransform.GetTranslation(), RightHandSocketTransform.GetTranslation(), DeltaTime, 10.f));
		}
	}
}
#pragma endregion

#pragma region ArmRecoil
void UPlayerCharacterAnimInstance::AddArmRecoil(FArmRecoilParams* armrecoil, float AdditionalRecoilAmountX, float AdditionalRecoilAmountY, float AdditionalRecoilAmountZ)
{
	if (armrecoil)
	{
		bIsArmRecoiling = true;
		ArmRecoil = *armrecoil;
		//UE_LOG(LogTemp, Warning, TEXT("Arm Recoil Added!!!"));

		FRotator RandRecoil_Rot;
		FVector RandRecoil_Vec;

		RandRecoil_Rot.Roll = FMath::RandRange((ArmRecoil.Recoil_Rot.Roll + AdditionalRecoilAmountX) * ArmRecoil.RecoilRangeMin_Rot.Roll, (ArmRecoil.Recoil_Rot.Roll + AdditionalRecoilAmountX) * ArmRecoil.RecoilRangeMax_Rot.Roll);
		RandRecoil_Rot.Pitch = FMath::RandRange((ArmRecoil.Recoil_Rot.Pitch + AdditionalRecoilAmountY) * ArmRecoil.RecoilRangeMin_Rot.Pitch, (ArmRecoil.Recoil_Rot.Pitch + AdditionalRecoilAmountY) * ArmRecoil.RecoilRangeMax_Rot.Pitch);
		RandRecoil_Rot.Yaw = FMath::RandRange((ArmRecoil.Recoil_Rot.Yaw + AdditionalRecoilAmountZ) * ArmRecoil.RecoilRangeMin_Rot.Yaw, (ArmRecoil.Recoil_Rot.Yaw + AdditionalRecoilAmountZ) * ArmRecoil.RecoilRangeMax_Rot.Yaw);

		RandRecoil_Vec.X = FMath::RandRange((ArmRecoil.Recoil_Vec.X + AdditionalRecoilAmountX) * ArmRecoil.RecoilRangeMin_Vec.X, (ArmRecoil.Recoil_Vec.X + AdditionalRecoilAmountX) * ArmRecoil.RecoilRangeMax_Vec.X);
		RandRecoil_Vec.Y = FMath::RandRange((ArmRecoil.Recoil_Vec.Y + AdditionalRecoilAmountY) * ArmRecoil.RecoilRangeMin_Vec.Y, (ArmRecoil.Recoil_Vec.Y + AdditionalRecoilAmountY) * ArmRecoil.RecoilRangeMax_Vec.Y);
		RandRecoil_Vec.Z = FMath::RandRange((ArmRecoil.Recoil_Vec.Z + AdditionalRecoilAmountZ) * ArmRecoil.RecoilRangeMin_Vec.Z, (ArmRecoil.Recoil_Vec.Z + AdditionalRecoilAmountZ) * ArmRecoil.RecoilRangeMax_Vec.Z);

		TotalTargetRecoil_Rot += RandRecoil_Rot;
		TotalTargetRecoil_Vec += RandRecoil_Vec;
	}
}

void UPlayerCharacterAnimInstance::ApplyArmRecoil(float DeltaTime)
{
	if (&ArmRecoil)
	{
		FRotator InterpRecoilTarget_Rot = FMath::RInterpTo(CurrentRecoil_Rot, TotalTargetRecoil_Rot, DeltaTime, ArmRecoil.RecoilSpeed);
		CurrentRecoil_Rot = InterpRecoilTarget_Rot;

		FVector InterpRecoilTarget_Vec = FMath::VInterpTo(CurrentRecoil_Vec, TotalTargetRecoil_Vec, DeltaTime, ArmRecoil.RecoilSpeed);
		CurrentRecoil_Vec = InterpRecoilTarget_Vec;
	}
}

void UPlayerCharacterAnimInstance::RecoverArmRecoil(float DeltaTime)
{
	FRotator InterpRecoilRecoverTarget_Rot = FMath::RInterpTo(TotalTargetRecoil_Rot, FRotator::ZeroRotator, DeltaTime, ArmRecoil.RecoilRecoverSpeed);
	TotalTargetRecoil_Rot = InterpRecoilRecoverTarget_Rot;

	FVector InterpRecoilRecoverTarget_Vec = FMath::VInterpTo(TotalTargetRecoil_Vec, FVector::ZeroVector, DeltaTime, ArmRecoil.RecoilRecoverSpeed);
	TotalTargetRecoil_Vec = InterpRecoilRecoverTarget_Vec;

	if (CurrentRecoil_Rot.IsNearlyZero()
		&& CurrentRecoil_Vec.IsNearlyZero())
	{
		//UE_LOG(LogTemp, Warning, TEXT("Arm Recoil has been perfectly Recovered!!!"));

		TotalTargetRecoil_Rot = FRotator::ZeroRotator;
		CurrentRecoil_Rot = FRotator::ZeroRotator;

		TotalTargetRecoil_Vec = FVector::ZeroVector;
		CurrentRecoil_Vec = FVector::ZeroVector;

		bIsArmRecoiling = false;
	}
}

void UPlayerCharacterAnimInstance::UpdateArmRecoil(float DeltaTime)
{
	if (bIsArmRecoiling)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Updating Arm Recoil"));

		ApplyArmRecoil(DeltaTime);
		RecoverArmRecoil(DeltaTime);
	}
}

void UPlayerCharacterAnimInstance::ConvertRecoilValueFrame()
{
	ConvertedCurrentRecoil_Rot = (RightHandSocketSpringDamperTransform.GetRotation() * CurrentRecoil_Rot.Quaternion()).Rotator();
	ConvertedCurrentRecoil_Vec = RightHandSocketSpringDamperTransform.GetRotation().RotateVector(CurrentRecoil_Vec) + RightHandSocketSpringDamperTransform.GetTranslation();
}
#pragma endregion

#pragma region SkillWeaponRecoil
void UPlayerCharacterAnimInstance::UpdateSkillWeaponSocket(float DeltaTime)
{
	//UE_LOG(LogTemp, Error, TEXT("No SkillWeapon!!!"));
	if (!CurrentSkillWeapon) { return; }

	if (CurrentSkillWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Targeting || CurrentSkillWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Firing)
	{
		CurrentSkillWeaponSocketTransform.SetRotation((FMath::RInterpTo(CurrentSkillWeaponSocketTransform.GetRotation().Rotator(), SkillWeaponSocketTransform_Active.GetRotation().Rotator(), DeltaTime, ActiveRotationInterpSpeed_SkillWeapon)).Quaternion());
		CurrentSkillWeaponSocketTransform.SetTranslation(FMath::VInterpTo(CurrentSkillWeaponSocketTransform.GetTranslation(), SkillWeaponSocketTransform_Active.GetTranslation(), DeltaTime, ActiveLocationInterpSpeed_SkillWeapon));
	}
	else
	{
		CurrentSkillWeaponSocketTransform.SetRotation((FMath::RInterpTo(CurrentSkillWeaponSocketTransform.GetRotation().Rotator(), SkillWeaponSocketTransform_Inactive.GetRotation().Rotator(), DeltaTime, InctiveRotationInterpSpeed_SkillWeapon)).Quaternion());
		CurrentSkillWeaponSocketTransform.SetTranslation(FMath::VInterpTo(CurrentSkillWeaponSocketTransform.GetTranslation(), SkillWeaponSocketTransform_Inactive.GetTranslation(), DeltaTime, InctiveLocationInterpSpeed_SkillWeapon));
	}
}
void UPlayerCharacterAnimInstance::AddSkillWeaponRecoil(FArmRecoilParams* armrecoil, float AdditionalRecoilAmountX, float AdditionalRecoilAmountY, float AdditionalRecoilAmountZ)
{
	if (armrecoil)
	{
		bIsSkillWeaponRecoiling = true;
		SkillWeaponRecoil = *armrecoil;
		//UE_LOG(LogTemp, Warning, TEXT("Arm Recoil Added!!!"));

		FRotator RandRecoil_Rot;
		FVector RandRecoil_Vec;

		RandRecoil_Rot.Roll = FMath::RandRange((ArmRecoil.Recoil_Rot.Roll + AdditionalRecoilAmountX) * ArmRecoil.RecoilRangeMin_Rot.Roll, (ArmRecoil.Recoil_Rot.Roll + AdditionalRecoilAmountX) * ArmRecoil.RecoilRangeMax_Rot.Roll);
		RandRecoil_Rot.Pitch = FMath::RandRange((ArmRecoil.Recoil_Rot.Pitch + AdditionalRecoilAmountY) * ArmRecoil.RecoilRangeMin_Rot.Pitch, (ArmRecoil.Recoil_Rot.Pitch + AdditionalRecoilAmountY) * ArmRecoil.RecoilRangeMax_Rot.Pitch);
		RandRecoil_Rot.Yaw = FMath::RandRange((ArmRecoil.Recoil_Rot.Yaw + AdditionalRecoilAmountZ) * ArmRecoil.RecoilRangeMin_Rot.Yaw, (ArmRecoil.Recoil_Rot.Yaw + AdditionalRecoilAmountZ) * ArmRecoil.RecoilRangeMax_Rot.Yaw);

		RandRecoil_Vec.X = FMath::RandRange((ArmRecoil.Recoil_Vec.X + AdditionalRecoilAmountX) * ArmRecoil.RecoilRangeMin_Vec.X, (ArmRecoil.Recoil_Vec.X + AdditionalRecoilAmountX) * ArmRecoil.RecoilRangeMax_Vec.X);
		RandRecoil_Vec.Y = FMath::RandRange((ArmRecoil.Recoil_Vec.Y + AdditionalRecoilAmountY) * ArmRecoil.RecoilRangeMin_Vec.Y, (ArmRecoil.Recoil_Vec.Y + AdditionalRecoilAmountY) * ArmRecoil.RecoilRangeMax_Vec.Y);
		RandRecoil_Vec.Z = FMath::RandRange((ArmRecoil.Recoil_Vec.Z + AdditionalRecoilAmountZ) * ArmRecoil.RecoilRangeMin_Vec.Z, (ArmRecoil.Recoil_Vec.Z + AdditionalRecoilAmountZ) * ArmRecoil.RecoilRangeMax_Vec.Z);

		TotalTargetSkillWeaponRecoil_Rot += RandRecoil_Rot;
		TotalTargetSkillWeaponRecoil_Vec += RandRecoil_Vec;
	}
}
void UPlayerCharacterAnimInstance::ApplySkillWeaponRecoil(float DeltaTime)
{
	if (&SkillWeaponRecoil)
	{
		FRotator InterpRecoilTarget_Rot = FMath::RInterpTo(CurrentSkillWeaponRecoil_Rot, TotalTargetSkillWeaponRecoil_Rot, DeltaTime, SkillWeaponRecoil.RecoilSpeed);
		CurrentSkillWeaponRecoil_Rot = InterpRecoilTarget_Rot;

		FVector InterpRecoilTarget_Vec = FMath::VInterpTo(CurrentSkillWeaponRecoil_Vec, TotalTargetSkillWeaponRecoil_Vec, DeltaTime, SkillWeaponRecoil.RecoilSpeed);
		CurrentSkillWeaponRecoil_Vec = InterpRecoilTarget_Vec;
	}
}
void UPlayerCharacterAnimInstance::RecoverSkillWeaponRecoil(float DeltaTime)
{
	FRotator InterpRecoilRecoverTarget_Rot = FMath::RInterpTo(TotalTargetSkillWeaponRecoil_Rot, FRotator::ZeroRotator, DeltaTime, SkillWeaponRecoil.RecoilRecoverSpeed);
	TotalTargetSkillWeaponRecoil_Rot = InterpRecoilRecoverTarget_Rot;

	FVector InterpRecoilRecoverTarget_Vec = FMath::VInterpTo(TotalTargetSkillWeaponRecoil_Vec, FVector::ZeroVector, DeltaTime, SkillWeaponRecoil.RecoilRecoverSpeed);
	TotalTargetSkillWeaponRecoil_Vec = InterpRecoilRecoverTarget_Vec;

	if (CurrentSkillWeaponRecoil_Rot.IsNearlyZero()
		&& CurrentSkillWeaponRecoil_Vec.IsNearlyZero())
	{
		//UE_LOG(LogTemp, Warning, TEXT("SkillWeapon Recoil has been perfectly Recovered!!!"));

		TotalTargetSkillWeaponRecoil_Rot = FRotator::ZeroRotator;
		CurrentSkillWeaponRecoil_Rot = FRotator::ZeroRotator;

		TotalTargetSkillWeaponRecoil_Vec = FVector::ZeroVector;
		CurrentSkillWeaponRecoil_Vec = FVector::ZeroVector;

		bIsSkillWeaponRecoiling = false;
	}
}
void UPlayerCharacterAnimInstance::UpdateSkillWeaponRecoil(float DeltaTime)
{
	if (bIsSkillWeaponRecoiling)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Updating Arm Recoil"));
		ApplySkillWeaponRecoil(DeltaTime);
		RecoverSkillWeaponRecoil(DeltaTime);
	}
}
void UPlayerCharacterAnimInstance::ConvertSkillWeaponRecoilValueFrame()
{
	//TODO: 스피링 댐퍼 적용
	ConvertedSkillWeaponCurrentRecoil_Rot = (CurrentSkillWeaponSocketTransform.GetRotation() * CurrentSkillWeaponRecoil_Rot.Quaternion()).Rotator();
	ConvertedSkillWeaponCurrentRecoil_Vec = CurrentSkillWeaponSocketTransform.GetRotation().RotateVector(CurrentSkillWeaponRecoil_Vec) + CurrentSkillWeaponSocketTransform.GetTranslation();

	if (CurrentSkillWeapon)
	{
		CurrentSkillWeapon->GetWeaponMesh()->SetRelativeRotation(ConvertedSkillWeaponCurrentRecoil_Rot);
		CurrentSkillWeapon->GetWeaponMesh()->SetRelativeLocation(ConvertedSkillWeaponCurrentRecoil_Vec);
	}
}
#pragma endregion

#pragma region Spring Damper
float UPlayerCharacterAnimInstance::fast_negexp(float x)
{
	return 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
}
void UPlayerCharacterAnimInstance::SpringDamper(FVector CurrPos, FVector CurrVel, FVector GoalPos, FVector GoalVel, FVector& OutPos, FVector& OutVel, FVector stiffness, FVector damping, float DeltaTime, float eps)
{
	FVector x = CurrPos;
	FVector v = CurrVel;

	FVector g = GoalPos;
	FVector q = GoalVel;
	FVector s = stiffness;
	FVector d = damping;
	FVector c = g + (d * q) / (s + eps);
	FVector y = d / 2.0f;

	// <X>
	if (fabs(s.X - (d.X * d.X) / 4.0f) < eps) // Critically Damped
	{
		float j0 = x.X - c.X;
		float j1 = v.X + j0 * y.X;

		float eydt = fast_negexp(y.X * DeltaTime);

		x.X = j0 * eydt + DeltaTime * j1 * eydt + c.X;
		v.X = -y.X * j0 * eydt - y.X * DeltaTime * j1 * eydt + j1 * eydt;
	}
	else if (s.X - (d.X * d.X) / 4.0f > 0.0) // Under Damped
	{
		float w = sqrtf(s.X - (d.X * d.X) / 4.0f);
		float j = sqrtf(FMath::Square(v.X + y.X * (x.X - c.X)) / (w * w + eps) + FMath::Square(x.X - c.X));
		float p = FMath::Atan((v.X + (x.X - c.X) * y.X) / (-(x.X - c.X) * w + eps));

		j = (x.X - c.X) > 0.0f ? j : -j;

		float eydt = fast_negexp(y.X * DeltaTime);

		x.X = j * eydt * cosf(w * DeltaTime + p) + c.X;
		v.X = -y.X * j * eydt * cosf(w * DeltaTime + p) - w * j * eydt * sinf(w * DeltaTime + p);
	}
	else if (s.X - (d.X * d.X) / 4.0f < 0.0) // Over Damped
	{
		float y0 = (d.X + sqrtf(d.X * d.X - 4 * s.X)) / 2.0f;
		float y1 = (d.X - sqrtf(d.X * d.X - 4 * s.X)) / 2.0f;

		float j1 = (c.X * y0 - x.X * y0 - v.X) / (y1 - y0);
		float j0 = x.X - j1 - c.X;

		float ey0dt = fast_negexp(y0 * DeltaTime);
		float ey1dt = fast_negexp(y1 * DeltaTime);

		x.X = j0 * ey0dt + j1 * ey1dt + c.X;
		v.X = -y0 * j0 * ey0dt - y1 * j1 * ey1dt;
	}

	// <Y>
	if (fabs(s.Y - (d.Y * d.Y) / 4.0f) < eps) // Critically Damped
	{
		float j0 = x.Y - c.Y;
		float j1 = v.Y + j0 * y.Y;

		float eydt = fast_negexp(y.Y * DeltaTime);

		x.Y = j0 * eydt + DeltaTime * j1 * eydt + c.Y;
		v.Y = -y.Y * j0 * eydt - y.Y * DeltaTime * j1 * eydt + j1 * eydt;
	}
	else if (s.Y - (d.Y * d.Y) / 4.0f > 0.0) // Under Damped
	{
		float w = sqrtf(s.Y - (d.Y * d.Y) / 4.0f);
		float j = sqrtf(FMath::Square(v.Y + y.Y * (x.Y - c.Y)) / (w * w + eps) + FMath::Square(x.Y - c.Y));
		float p = FMath::Atan((v.Y + (x.Y - c.Y) * y.Y) / (-(x.Y - c.Y) * w + eps));

		j = (x.Y - c.Y) > 0.0f ? j : -j;

		float eydt = fast_negexp(y.Y * DeltaTime);

		x.Y = j * eydt * cosf(w * DeltaTime + p) + c.Y;
		v.Y = -y.Y * j * eydt * cosf(w * DeltaTime + p) - w * j * eydt * sinf(w * DeltaTime + p);
	}
	else if (s.Y - (d.Y * d.Y) / 4.0f < 0.0) // Over Damped
	{
		float y0 = (d.Y + sqrtf(d.Y * d.Y - 4 * s.Y)) / 2.0f;
		float y1 = (d.Y - sqrtf(d.Y * d.Y - 4 * s.Y)) / 2.0f;

		float j1 = (c.Y * y0 - x.Y * y0 - v.Y) / (y1 - y0);
		float j0 = x.Y - j1 - c.Y;

		float ey0dt = fast_negexp(y0 * DeltaTime);
		float ey1dt = fast_negexp(y1 * DeltaTime);

		x.Y = j0 * ey0dt + j1 * ey1dt + c.Y;
		v.Y = -y0 * j0 * ey0dt - y1 * j1 * ey1dt;
	}

	// <Z>
	if (fabs(s.Z - (d.Z * d.Z) / 4.0f) < eps) // Critically Damped
	{
		float j0 = x.Z - c.Z;
		float j1 = v.Z + j0 * y.Z;

		float eydt = fast_negexp(y.Z * DeltaTime);

		x.Z = j0 * eydt + DeltaTime * j1 * eydt + c.Z;
		v.Z = -y.Z * j0 * eydt - y.Z * DeltaTime * j1 * eydt + j1 * eydt;
	}
	else if (s.Z - (d.Z * d.Z) / 4.0f > 0.0) // Under Damped
	{
		float w = sqrtf(s.Z - (d.Z * d.Z) / 4.0f);
		float j = sqrtf(FMath::Square(v.Z + y.Z * (x.Z - c.Z)) / (w * w + eps) + FMath::Square(x.Z - c.Z));
		float p = FMath::Atan((v.Z + (x.Z - c.Z) * y.Z) / (-(x.Z - c.Z) * w + eps));

		j = (x.Z - c.Z) > 0.0f ? j : -j;

		float eydt = fast_negexp(y.Z * DeltaTime);

		x.Z = j * eydt * cosf(w * DeltaTime + p) + c.Z;
		v.Z = -y.Z * j * eydt * cosf(w * DeltaTime + p) - w * j * eydt * sinf(w * DeltaTime + p);
	}
	else if (s.Z - (d.Z * d.Z) / 4.0f < 0.0) // Over Damped
	{
		float y0 = (d.Z + sqrtf(d.Z * d.Z - 4 * s.Z)) / 2.0f;
		float y1 = (d.Z - sqrtf(d.Z * d.Z - 4 * s.Z)) / 2.0f;

		float j1 = (c.Z * y0 - x.Z * y0 - v.Z) / (y1 - y0);
		float j0 = x.Z - j1 - c.Z;

		float ey0dt = fast_negexp(y0 * DeltaTime);
		float ey1dt = fast_negexp(y1 * DeltaTime);

		x.Z = j0 * ey0dt + j1 * ey1dt + c.Z;
		v.Z = -y0 * j0 * ey0dt - y1 * j1 * ey1dt;
	}

	OutPos = x;
	OutVel = v;
}
void UPlayerCharacterAnimInstance::SpringDamper_f(float CurrPos, float CurrVel, float GoalPos, float GoalVel, double& OutPos, double& OutVel, float damping_ratio, float halflife, float DeltaTime, float eps)
{
	//// <Old Version>

	//float x = CurrPos;
	//float v = CurrVel;
	//float dt = DeltaTime;

	//float g = GoalPos;
	//float q = GoalVel;
	//float d = halflife_to_damping(halflife);
	//float s = damping_ratio_to_stiffness(damping_ratio, d);
	//float c = g + (d * q) / (s + eps);
	//float y = d / 2.0f;

	//if (fabs(s - (d * d) / 4.0f) < eps) // Critically Damped
	//{
	//	float j0 = x - c;
	//	float j1 = v + j0 * y;

	//	float eydt = fast_negexp(y * dt);

	//	x = j0 * eydt + dt * j1 * eydt + c;
	//	v = -y * j0 * eydt - y * dt * j1 * eydt + j1 * eydt;
	//}
	//else if (s - (d * d) / 4.0f > 0.0) // Under Damped
	//{
	//	float w = FMath::Sqrt(s - (d * d) / 4.0f);
	//	float j = FMath::Sqrt(FMath::Square(v + y * (x - c)) / (w * w + eps) + FMath::Square(x - c));
	//	float p = FMath::Atan((v + (x - c) * y) / (-(x - c) * w + eps)); //TODO: Atan or Atan2 ???

	//	j = (x - c) > 0.0f ? j : -j;

	//	float eydt = fast_negexp(y * dt);

	//	x = j * eydt * FMath::Cos(w * dt + p) + c;
	//	v = -y * j * eydt * FMath::Cos(w * dt + p) - w * j * eydt * FMath::Sin(w * dt + p);
	//}
	//else if (s - (d * d) / 4.0f < 0.0) // Over Damped
	//{
	//	float y0 = (d + FMath::Sqrt(d * d - 4 * s)) / 2.0f;
	//	float y1 = (d - FMath::Sqrt(d * d - 4 * s)) / 2.0f;
	//	float j1 = (c * y0 - x * y0 - v) / (y1 - y0);
	//	float j0 = x - j1 - c;

	//	float ey0dt = fast_negexp(y0 * dt);
	//	float ey1dt = fast_negexp(y1 * dt);

	//	x = j0 * ey0dt + j1 * ey1dt + c;
	//	v = -y0 * j0 * ey0dt - y1 * j1 * ey1dt;
	//}

	//OutPos = x;
	//OutVel = v;

	//--------------------------
	// <New Version> //fast negexp 대신 exp
	float x = CurrPos;
	float v = CurrVel;
	float dt = DeltaTime;

	float g = GoalPos;
	float q = GoalVel;
	float d = halflife_to_damping(halflife);
	float s = damping_ratio_to_stiffness(damping_ratio, d);
	float c = g + (d * q) / (s + eps);
	float y = d / 2.0f;

	if (fabs(s - (d * d) / 4.0f) < eps) // Critically Damped
	{
		float j0 = x - c;
		float j1 = v + j0 * y;

		float eydt = FMath::Exp(-(y * dt));

		x = j0 * eydt + dt * j1 * eydt + c;
		v = -y * j0 * eydt - y * dt * j1 * eydt + j1 * eydt;
	}
	else if (s - (d * d) / 4.0f > 0.0) // Under Damped
	{
		float w = FMath::Sqrt(s - (d * d) / 4.0f);
		float j = FMath::Sqrt(FMath::Square(v + y * (x - c)) / (w * w + eps) + FMath::Square(x - c));
		float p = FMath::Atan((v + (x - c) * y) / (-(x - c) * w + eps)); //TODO: Atan or Atan2 ???

		j = (x - c) > 0.0f ? j : -j;

		float eydt = FMath::Exp(-(y * dt));

		x = j * eydt * FMath::Cos(w * dt + p) + c;
		v = -y * j * eydt * FMath::Cos(w * dt + p) - w * j * eydt * FMath::Sin(w * dt + p);
	}
	else if (s - (d * d) / 4.0f < 0.0) // Over Damped
	{
		float y0 = (d + FMath::Sqrt(d * d - 4 * s)) / 2.0f;
		float y1 = (d - FMath::Sqrt(d * d - 4 * s)) / 2.0f;
		float j1 = (c * y0 - x * y0 - v) / (y1 - y0);
		float j0 = x - j1 - c;

		float ey0dt = FMath::Exp(-(y0 * dt));
		float ey1dt = FMath::Exp(-(y1 * dt));

		x = j0 * ey0dt + j1 * ey1dt + c;
		v = -y0 * j0 * ey0dt - y1 * j1 * ey1dt;
	}

	OutPos = x;
	OutVel = v;

}
void UPlayerCharacterAnimInstance::UpdateSpringDamper(float DeltaTime)
{
	FTransform MeshToWorldTransform = PlayerCharacter->GetArmMesh()->GetComponentTransform();
	FTransform ActorToWorldTransform = PlayerCharacter->GetTransform();

	FVector CurrPos = ActorToWorldTransform.InverseTransformPosition(CurrentComponentPos); //Actor
	FVector CurrVel = ActorToWorldTransform.InverseTransformVector(CurrentComponentVel); //Actor

	FVector GoalPos = ActorToWorldTransform.InverseTransformPosition(MeshToWorldTransform.TransformPosition(CurrentRightHandSocketTransform.GetLocation())); //Actor
	FVector GoalVel = ActorToWorldTransform.InverseTransformVector(PlayerCharacter->GetVelocity()); //Actor
	//FVector GoalVel = ActorToWorldTransform.InverseTransformVector(Velocity); //Actor

	FVector OutPos; //Actor
	FVector OutVel; //Actor

	SpringDamper(CurrPos, CurrVel, GoalPos, GoalVel, OutPos, OutVel, Stiffness, Damping, DeltaTime); //Local 기준 계산

	//UE_LOG(LogTemp, Warning, TEXT("GoalVel: %s"), *GoalVel.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("GoalPos: %s"), *GoalPos.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("CurrVel: %s"), *CurrVel.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("CurrPos: %s"), *CurrPos.ToString());

	//UE_LOG(LogTemp, Warning, TEXT("Velocity: %s"), *Velocity.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("Player->GetVelocity(): %s"), *SuraPlayer->GetVelocity().ToString());

	FVector ConvertedPos = (GoalPos + (OutPos - GoalPos) * DamperScale);
	ConvertedPos.X = GoalPos.X;

	ConvertedPos = MeshToWorldTransform.InverseTransformPosition(ActorToWorldTransform.TransformPosition(ConvertedPos));

	CurrentComponentPos = ActorToWorldTransform.TransformPosition(OutPos); //World
	CurrentComponentVel = ActorToWorldTransform.TransformVector(OutVel); //World

	FVector DirectionVec = ConvertedPos - (CurrentRightHandSocketTransform.GetLocation() + FVector(0.f, -20.f, 0.f));
	FQuat RotByZ = FQuat(FVector::ZAxisVector, -FMath::Atan(DirectionVec.X / DirectionVec.Y));
	FQuat RotByX = FQuat(FVector::XAxisVector, FMath::Atan(DirectionVec.Z / FMath::Sqrt(DirectionVec.X * DirectionVec.X + DirectionVec.Y * DirectionVec.Y)));

	//Tilt
	FQuat RotByY = FQuat(FVector::YAxisVector, (CurrentRightHandSocketTransform.GetLocation().X - ConvertedPos.X) * (0.3f)); //Test


	RightHandSocketSpringDamperTransform.SetLocation(ConvertedPos);
	//RightHandSocketSpringDamperTransform.SetRotation(RotByX * RotByZ * CurrentRightHandSocketTransform.GetRotation());
	RightHandSocketSpringDamperTransform.SetRotation(RotByY * RotByX * RotByZ * CurrentRightHandSocketTransform.GetRotation()); //Test

}
void UPlayerCharacterAnimInstance::UpdateSpringDamper_Upgrade(float DeltaTime)
{
	FTransform MeshToWorldTransform = PlayerCharacter->GetArmMesh()->GetComponentTransform();
	FTransform ActorToWorldTransform = PlayerCharacter->GetTransform();

	FVector CurrPos = ActorToWorldTransform.InverseTransformPosition(CurrentComponentPos); //Actor
	FVector CurrVel = ActorToWorldTransform.InverseTransformVector(CurrentComponentVel); //Actor

	FVector GoalPos = ActorToWorldTransform.InverseTransformPosition(MeshToWorldTransform.TransformPosition(CurrentRightHandSocketTransform.GetLocation())); //Actor
	FVector GoalVel = ActorToWorldTransform.InverseTransformVector(PlayerCharacter->GetVelocity()); //Actor
	//FVector GoalVel = ActorToWorldTransform.InverseTransformVector(Velocity); //Actor

	FVector OutPos; //Actor
	FVector OutVel; //Actor

	//SpringDamper(CurrPos, CurrVel, GoalPos, GoalVel, OutPos, OutVel, Stiffness, Damping, DeltaTime); //Local 기준 계산
	SpringDamper_f(CurrPos.X, CurrVel.X, GoalPos.X, GoalVel.X, OutPos.X, OutVel.X, DampingRatio.X, HalfLife.X, DeltaTime);
	SpringDamper_f(CurrPos.Y, CurrVel.Y, GoalPos.Y, GoalVel.Y, OutPos.Y, OutVel.Y, DampingRatio.Y, HalfLife.Y, DeltaTime);
	SpringDamper_f(CurrPos.Z, CurrVel.Z, GoalPos.Z, GoalVel.Z, OutPos.Z, OutVel.Z, DampingRatio.Z, HalfLife.Z, DeltaTime);

	//UE_LOG(LogTemp, Error, TEXT("OutPos Z: %f"), OutPos.Z);


	FVector ConvertedPos = (GoalPos + (OutPos - GoalPos) * DamperScale);
	ConvertedPos.X = GoalPos.X;

	ConvertedPos = MeshToWorldTransform.InverseTransformPosition(ActorToWorldTransform.TransformPosition(ConvertedPos));

	CurrentComponentPos = ActorToWorldTransform.TransformPosition(OutPos); //World
	CurrentComponentVel = ActorToWorldTransform.TransformVector(OutVel); //World

	FVector DirectionVec = ConvertedPos - (CurrentRightHandSocketTransform.GetLocation() + FVector(0.f, -20.f, 0.f));
	FQuat RotByZ = FQuat(FVector::ZAxisVector, -FMath::Atan(DirectionVec.X / DirectionVec.Y));
	FQuat RotByX = FQuat(FVector::XAxisVector, FMath::Atan(DirectionVec.Z / FMath::Sqrt(DirectionVec.X * DirectionVec.X + DirectionVec.Y * DirectionVec.Y)));

	//Tilt
	FQuat RotByY = FQuat(FVector::YAxisVector, (CurrentRightHandSocketTransform.GetLocation().X - ConvertedPos.X) * (0.3f)); //Test


	RightHandSocketSpringDamperTransform.SetLocation(ConvertedPos);
	//RightHandSocketSpringDamperTransform.SetRotation(RotByX * RotByZ * CurrentRightHandSocketTransform.GetRotation());
	RightHandSocketSpringDamperTransform.SetRotation(RotByY * RotByX * RotByZ * CurrentRightHandSocketTransform.GetRotation()); //Test
}
void UPlayerCharacterAnimInstance::UpdateSpringDamper_MLS(float DeltaTime)
{
	FTransform MeshToWorldTransform = PlayerCharacter->GetArmMesh()->GetComponentTransform();
	FTransform ActorToWorldTransform = PlayerCharacter->GetTransform();

	FVector CurrPos = ActorToWorldTransform.InverseTransformPosition(CurrentComponentPos); //Actor
	FVector CurrVel = ActorToWorldTransform.InverseTransformVector(CurrentComponentVel); //Actor

	FVector GoalPos = ActorToWorldTransform.InverseTransformPosition(MeshToWorldTransform.TransformPosition(CurrentRightHandSocketTransform.GetLocation())); //Actor
	FVector GoalVel = ActorToWorldTransform.InverseTransformVector(PlayerCharacter->GetVelocity()); //Actor


	FVector OutPos; //Actor
	FVector OutVel; //Actor

	//Local 기준 계산
	SpringDamper_f(CurrPos.X, CurrVel.X, GoalPos.X, GoalVel.X, OutPos.X, OutVel.X, DampingRatio.X, HalfLife.X, DeltaTime);
	SpringDamper_f(CurrPos.Y, CurrVel.Y, GoalPos.Y, GoalVel.Y, OutPos.Y, OutVel.Y, DampingRatio.Y, HalfLife.Y, DeltaTime);
	SpringDamper_f(CurrPos.Z, CurrVel.Z, GoalPos.Z, GoalVel.Z, OutPos.Z, OutVel.Z, DampingRatio.Z, HalfLife.Z, DeltaTime);

	//UE_LOG(LogTemp, Error, TEXT("OutPos Z: %f"), OutPos.Z);

	//SpringDamperPosBuffer.Push(OutPos.Z, false);
	SpringDamperPosBuffer.Push(OutPos.Z, true);
	float MLS_Z = SpringDamperPosBuffer.GetApproximation();
	//SpringDamperPosBuffer.Push(MLS_Z, true);


	//if (Player->GetVelocity().Z != 0)
	//{
	//	UE_LOG(LogTemp, Error, TEXT("OutPos Z: %f"), OutPos.Z);
	//	UE_LOG(LogTemp, Error, TEXT("MLS_Z: %f"), MLS_Z);
	//}

	OutPos.Z = MLS_Z;


	//UE_LOG(LogTemp, Error, TEXT("MLS_Z: %f"), MLS_Z);

	FVector ConvertedPos = (GoalPos + (OutPos - GoalPos) * DamperScale);
	ConvertedPos.X = GoalPos.X;

	ConvertedPos = MeshToWorldTransform.InverseTransformPosition(ActorToWorldTransform.TransformPosition(ConvertedPos));

	CurrentComponentPos = ActorToWorldTransform.TransformPosition(OutPos); //World
	CurrentComponentVel = ActorToWorldTransform.TransformVector(OutVel); //World

	FVector DirectionVec = ConvertedPos - (CurrentRightHandSocketTransform.GetLocation() + FVector(0.f, -20.f, 0.f));
	FQuat RotByZ = FQuat(FVector::ZAxisVector, -FMath::Atan(DirectionVec.X / DirectionVec.Y));
	FQuat RotByX = FQuat(FVector::XAxisVector, FMath::Atan(DirectionVec.Z / FMath::Sqrt(DirectionVec.X * DirectionVec.X + DirectionVec.Y * DirectionVec.Y)));

	//Tilt
	FQuat RotByY = FQuat(FVector::YAxisVector, (CurrentRightHandSocketTransform.GetLocation().X - ConvertedPos.X) * (0.3f)); //Test


	RightHandSocketSpringDamperTransform.SetLocation(ConvertedPos);
	//RightHandSocketSpringDamperTransform.SetRotation(RotByX * RotByZ * CurrentRightHandSocketTransform.GetRotation());
	RightHandSocketSpringDamperTransform.SetRotation(RotByY * RotByX * RotByZ * CurrentRightHandSocketTransform.GetRotation()); //Test
}
#pragma endregion

#pragma region Tilt
void UPlayerCharacterAnimInstance::UpdateRightHandTilt(float DeltaTime)
{
	//FTransform MeshToWorldTransform = Player->GetArmMesh()->GetComponentTransform();
	//FTransform ActorToWorldTransform = Player->GetTransform();

	//FVector CurrPos = ActorToWorldTransform.InverseTransformPosition(Player->GetActorLocation()); //Actor
	//FVector GoalPos = ActorToWorldTransform.InverseTransformPosition(MeshToWorldTransform.TransformPosition(RightHandSocketSpringDamperTransform.GetLocation())); //Actor


	//ConvertedPos = MeshToWorldTransform.InverseTransformPosition(ActorToWorldTransform.TransformPosition(ConvertedPos));


	//FVector DirectionVec = ConvertedPos - (CurrentRightHandSocketTransform.GetLocation() + FVector(0.f, -20.f, 0.f));
	//FQuat RotByZ = FQuat(FVector::ZAxisVector, -FMath::Atan(DirectionVec.X / DirectionVec.Y));
	//FQuat RotByX = FQuat(FVector::XAxisVector, FMath::Atan(DirectionVec.Z / FMath::Sqrt(DirectionVec.X * DirectionVec.X + DirectionVec.Y * DirectionVec.Y)));

	//RightHandSocketSpringDamperTransform.SetLocation(ConvertedPos);
	//RightHandSocketSpringDamperTransform.SetRotation(RotByX * RotByZ * CurrentRightHandSocketTransform.GetRotation());
}
#pragma endregion