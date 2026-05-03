


#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Player/CharacterPlayer.h"
//#include "ActorComponents/DamageComponent/ACEnemyDamageSystem.h"
//#include "ActorComponents/UISystem/ACKillLogManager.h"
//#include "Widgets/Enemies/EnemyHealthBarWidget.h"
//#include "Structures/Enemies/EnemyAttributesData.h"
//#include "Characters/Enemies/AI/EnemyBaseAIController.h"

#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "BrainComponent.h"
#include "NiagaraComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "MotionWarpingComponent.h"


#define ECC_ENEMY ECC_GameTraceChannel5

AEnemyBase::AEnemyBase()
{
	// Damage system comp
	//DamageSystemComp = CreateDefaultSubobject<UACEnemyDamageSystem>(TEXT("DamageSystemComponent"));
	//AddOwnedComponent(DamageSystemComp);

	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));

	if (HealthBarWidget)
	{
		HealthBarWidget->SetupAttachment(RootComponent);
		HealthBarWidget->SetWidgetSpace(EWidgetSpace::World);
		HealthBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 1.f));

		static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClass{ TEXT("/Game/UI/Enemies/WBP_EnemyHealthBar") };

		if (WidgetClass.Succeeded())
		{
			HealthBarWidget->SetWidgetClass((WidgetClass.Class));
		}
	}

	bUseControllerRotationYaw = true; // for controller controlled rotation
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->GravityScale = 2.f;

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	NiagaraComponent->SetupAttachment(RootComponent);

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));

	EnemyType = "Base";

	GetCapsuleComponent()->SetCollisionProfileName("EnemyPawnOverlap");

	HitColorTimeline = CreateDefaultSubobject<UTimelineComponent>("HitColorTimeline");

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>("DissolveColorTimeline");

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel8, ECollisionResponse::ECR_Block); // <JaeHyeong>

}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	InitializeEnemy();

	// Initialize hit color timeline component.
	if (HitColorCurve)
	{
		OnHitColorTimelineFloat.BindUFunction(this, FName("UpdateHitColor"));
		HitColorTimeline->AddInterpFloat(HitColorCurve, OnHitColorTimelineFloat);
	}

	if (DissolveColorCurve)
	{
		OnDissolveColorTimelineFloat.BindUFunction(this, FName("UpdateDissolveColor"));
		DissolveTimeline->AddInterpFloat(DissolveColorCurve, OnDissolveColorTimelineFloat);
	}

	BindKillLogOnDeath();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel8, ECollisionResponse::ECR_Block); // <JaeHyeong>
	GetCapsuleComponent()->GetBodyInstance()->bLockXRotation = true;
	GetCapsuleComponent()->GetBodyInstance()->bLockYRotation = true;
	GetCapsuleComponent()->GetBodyInstance()->bLockZRotation = false;
}

void AEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//if (!isInitialized) return;

	//if (UEnemyHealthBarWidget* const Widget = Cast<UEnemyHealthBarWidget>(HealthBarWidget->GetUserWidgetObject()))
	//{
	//	float Distance = FVector::Dist(GetPlayerController()->PlayerCameraManager->GetCameraLocation(), GetActorLocation());

	//	//int32 ViewportX, ViewportY;
	//	//GetPlayerController()->GetViewportSize(ViewportX, ViewportY);

	//	Distance = FMath::Clamp(Distance, 100, 1000);

	//	FVector2D NewSize = GetHealthBarWidgetSize() * 500 / Distance; // * (ViewportY / 2) / FMath::Tan(FMath::DegreesToRadians(GetPlayerController()->PlayerCameraManager->GetFOVAngle() / 2))

	//	Widget->ResizeHealthBar(NewSize);
	//}

	///*FHitResult Hit;

	//FCollisionQueryParams CollisionQueryParams;
	//CollisionQueryParams.AddIgnoredActor(this);

	//GetWorld()->LineTraceSingleByChannel(Hit, EnemyLocation, CameraLocation, ECollisionChannel::ECC_Visibility, CollisionQueryParams);

	//DrawDebugLine(GetWorld(), EnemyLocation, CameraLocation, Hit.bBlockingHit ? FColor::Blue : FColor::Red, false, 5.0f, 0, 10.0f);

	//if (Hit.bBlockingHit)
	//	HealthBarWidget->SetHiddenInGame(true);
	//else
	//	HealthBarWidget->SetHiddenInGame(false);*/
}

void AEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

	Super::EndPlay(EndPlayReason);
}

void AEnemyBase::OnDamagedTriggered()
{
	//if (GetWorldTimerManager().IsTimerActive(HideHealthBarHandle))
	//	GetWorldTimerManager().ClearTimer(HideHealthBarHandle);

	//GetWorldTimerManager().SetTimer(
	//	HideHealthBarHandle,
	//	FTimerDelegate::CreateWeakLambda(this, [this]() { HealthBarWidget->SetHiddenInGame(true); }),
	//	1.f,
	//	false
	//);

	//UpdateHealthBarValue();
	//HealthBarWidget->SetHiddenInGame(false);

	//if (!HealthBarWidget->bHiddenInGame)
	//	Cast<UEnemyHealthBarWidget>(HealthBarWidget->GetUserWidgetObject())->PlayFadeAnimation();

	//// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("%s"), *(HitAnimation->GetFName()).ToString()));

	//if (!HitAnimations.IsEmpty() && GetDamageSystemComp()->GetIsInterruptible())
	//{
	//	OnHitMontageEnded.BindUObject(this, &AEnemyBase::OnHitEnded);

	//	UAnimInstance* const EnemyAnimInstance = GetMesh()->GetAnimInstance();
	//	EnemyAnimInstance->Montage_Play(GetRandomAnimationMontage(HitAnimations));

	//	GetMesh()->GetAnimInstance()->Montage_SetBlendingOutDelegate(OnHitMontageEnded); // montage interrupted
	//	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(OnHitMontageEnded); // montage ended
	//}
}

void AEnemyBase::OnHitEnded(UAnimMontage* AnimMontage, bool bInterrupted)
{
	//ACharacterPlayer* Player = Cast<ACharacterPlayer>(GetPlayerController()->GetPawn());

	//if (Player && AIController->GetCurrentState() != EEnemyStates::Pursue && AIController->GetCurrentState() != EEnemyStates::Attacking && AIController->GetCurrentState() != EEnemyStates::Climbing)
	//	GetAIController()->SetStateToChaseOrPursue(Player);
}

void AEnemyBase::OnDeathTriggered()
{
	//UpdateHealthBarValue();

	//DissolveTimeline->PlayFromStart();

	//float DeathAnimDuration = 3.f;

	//if (!DeathAnimations.IsEmpty())
	//{
	//	UAnimMontage* DeathAnimation = GetRandomAnimationMontage(DeathAnimations);

	//	PlayAnimMontage(DeathAnimation);
	//	DeathAnimDuration = DeathAnimation->GetPlayLength();
	//}

	//if (AIController->GetCurrentState() == EEnemyStates::Climbing)
	//{
	//	// GetCharacterMovement()->StopMovementImmediately();
	//	// GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	//	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	//}

	//if (AIController->GetCurrentState() == EEnemyStates::Pursue || AIController->GetCurrentState() == EEnemyStates::Attacking)
	//{
	//	AIController->EndPursueState();
	//	// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Puruse Token Returned"));
	//}

	//AIController->ClearFocus(EAIFocusPriority::Gameplay);
	//AIController->GetBrainComponent()->StopLogic("Death");

	//// Disable all collisions on capsule
	//// GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//// GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	//GetCapsuleComponent()->SetCollisionObjectType(ECC_GameTraceChannel1); // to disable collision with Projectile object

	//// GetCapsuleComponent()->SetSimulatePhysics(false);

	//// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("%s"), GetCapsuleComponent()->IsSimulatingPhysics() ? TEXT("true") : TEXT("false")));

	//// Ragdoll physics
	///*GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	//GetMesh()->SetSimulatePhysics(true);
	//GetMesh()->SetCollisionObjectType(ECC_GameTraceChannel1);*/ // to disable collision with Projectile object

	////objectpoolDisableEnemy
	//FTimerHandle DeathHandle;



	//GetWorldTimerManager().SetTimer(
	//	DeathHandle,
	//	FTimerDelegate::CreateWeakLambda(this, [this]()
	//		{
	//			/*GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	//			GetMesh()->Stop();*/

	//			/*if (EnemyWeapon)
	//				EnemyWeapon->Destroy();*/

	//			Destroy();
	//		}),
	//	DeathAnimDuration,
	//	false
	//);
}

void AEnemyBase::UpdateHealthBarValue()
{
	//const float Health = GetDamageSystemComp()->GetHealth();
	//const float MaxHealth = GetDamageSystemComp()->GetMaxHealth();

	//if (UEnemyHealthBarWidget* const Widget = Cast<UEnemyHealthBarWidget>(HealthBarWidget->GetUserWidgetObject()))
	//	Widget->SetHealthBarPercent(Health / MaxHealth);
}

UAnimMontage* AEnemyBase::GetRandomAnimationMontage(TArray<UAnimMontage*> AnimMontages)
{
	int selection = FMath::RandRange(0, AnimMontages.Num() - 1);

	return AnimMontages[selection];
}

void AEnemyBase::UpdateHitColor(float Alpha)
{

	GetMesh()->SetScalarParameterValueOnMaterials("HitColorAlpha", Alpha);
}

void AEnemyBase::UpdateDissolveColor(float Alpha)
{
	GetMesh()->SetScalarParameterValueOnMaterials("DissolveAlpha", Alpha);
}

void AEnemyBase::LungeToTarget(float LungeForce = 1000.f)
{
	//if (!GetAIController())
	//	return;

	//GetAIController()->StopMovement();

	//if (!GetAIController()->GetAttackTarget())
	//	return;

	//FVector TargetLocation = GetAIController()->GetAttackTarget()->GetActorLocation();

	//if (!GetCharacterMovement())
	//	return;

	//float OriginalMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	//GetCharacterMovement()->MaxWalkSpeed = LungeForce;

	//FTimerHandle LungeSpeedResetHandle;
	//GetWorldTimerManager().SetTimer(
	//	LungeSpeedResetHandle,
	//	FTimerDelegate::CreateWeakLambda(this, [this, OriginalMaxWalkSpeed]() { if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed; }),
	//	0.5f,
	//	false
	//);

	//UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetAIController(), TargetLocation);

	///*FVector MyLocation = GetActorLocation();
	//FVector Direction = (TargetLocation - MyLocation).GetSafeNormal();
	//float Distance = FVector::Dist(MyLocation, TargetLocation);

	//UE_LOG(LogTemp, Error, TEXT("Distance: %f"), Distance);

	//// float LungeDistance = FVector::Dist(TargetLocation, MyLocation);
	//// FVector NewLocation = MyLocation + Direction * LungeDistance;

	//// or use interpolation?
	//LaunchCharacter(Direction * Distance, true, true);*/
}

void AEnemyBase::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	FVector ActorLocation = GetActorLocation();
	FRotator ActorRotation = GetActorRotation();

	OutLocation = ActorLocation + FVector(0.0f, 0.0f, 100.0f); // Adjust Z offset as needed

	OutRotation = FRotator(0.0f, ActorRotation.Yaw, 0.0f);
}

bool AEnemyBase::TakeDamage(const FDamageParams& DamageData, AActor* DamageCauser)
{
	//if (GetDamageSystemComp()->GetHealth() > 0)
	//{
	//	HitColorTimeline->PlayFromStart();
	//}

	//return GetDamageSystemComp()->TakeDamage(DamageData, DamageCauser);
	//-------------
	return false;
}

UAnimMontage* AEnemyBase::ChooseRandomAttackMontage()
{
	if (!AttackAnimations.IsEmpty())
		return GetRandomAnimationMontage(AttackAnimations);

	return nullptr;
}

void AEnemyBase::Attack(ACharacterPlayer* Player)
{
	/*if (!AttackAnimations.IsEmpty())
	{
		UAnimInstance* const EnemyAnimInstance = GetMesh()->GetAnimInstance();
		EnemyAnimInstance->Montage_Play(GetRandomAnimationMontage(AttackAnimations));
	}*/
}

//void AEnemyBase::SetMovementSpeed(EEnemySpeed Speed)
//{
//	switch (Speed)
//	{
//	case EEnemySpeed::Idle:
//		GetCharacterMovement()->MaxWalkSpeed = 0.f;
//		break;
//	case EEnemySpeed::Walk:
//		GetCharacterMovement()->MaxWalkSpeed = 250.f;
//		break;
//	case EEnemySpeed::Jog:
//		GetCharacterMovement()->MaxWalkSpeed = 400.f;
//		break;
//	case EEnemySpeed::Sprint:
//		GetCharacterMovement()->MaxWalkSpeed = 800.f;
//		break;
//	default:
//		GetCharacterMovement()->MaxWalkSpeed = 300.f;
//		break;
//	}
//}
//
//void AEnemyBase::JumpWall(const FVector& Destination)
//{
//	GetCharacterMovement()->GravityScale = 1.f;
//
//	FVector LaunchVelocity = FVector::ZeroVector;
//	FVector FinalDestination = FVector(Destination.X, Destination.Y, Destination.Z + 250.f);
//
//	UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, LaunchVelocity, GetActorLocation(), FinalDestination);
//
//	LaunchCharacter(LaunchVelocity, true, true);
//
//	FTimerHandle GravityScaleHandle;
//
//	GetWorldTimerManager().SetTimer(
//		GravityScaleHandle,
//		FTimerDelegate::CreateWeakLambda(this, [this]() { GetCharacterMovement()->GravityScale = 2.f; }),
//		2.f,
//		false
//	);
//}

void AEnemyBase::Climb(const FVector& Destination)
{
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	FVector ToVector = Destination - GetActorLocation();
	AddMovementInput(FVector(ToVector.X, ToVector.Y, ToVector.Z + 100.f), 5.0f);

	FOnMontageEnded OnClimbMontageEnded;

	OnClimbMontageEnded.BindUObject(this, &AEnemyBase::OnClimbEnded);

	if (ClimbAnimation)
	{
		UAnimInstance* const EnemyAnimInstance = GetMesh()->GetAnimInstance();
		EnemyAnimInstance->Montage_Play(ClimbAnimation);
	}

	GetMesh()->GetAnimInstance()->Montage_SetBlendingOutDelegate(OnClimbMontageEnded); // montage interrupted
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(OnClimbMontageEnded); // montage ended
}

void AEnemyBase::OnClimbEnded(UAnimMontage* AnimMontage, bool bInterrupted)
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void AEnemyBase::InitializeEnemy()
{
	//if (isInitialized)
	//{
	//	GetDamageSystemComp()->SetHealth(GetDamageSystemComp()->GetMaxHealth());
	//}
	//else
	//{
	//	UpdateHealthBarValue();

	//	HealthBarWidget->SetHiddenInGame(true);

	//	if (UEnemyHealthBarWidget* const Widget = Cast<UEnemyHealthBarWidget>(HealthBarWidget->GetUserWidgetObject()))
	//	{
	//		HealthBarWidgetSize = Widget->GetHealthBarSize();
	//	}

	//	if (DamageSystemComp)
	//	{
	//		GetDamageSystemComp()->OnDamaged.AddUObject(this, &AEnemyBase::OnDamagedTriggered);
	//		GetDamageSystemComp()->OnDeath.AddUObject(this, &AEnemyBase::OnDeathTriggered);
	//	}

	//	// GetCapsuleComponent()->SetVisibility(true);
	//	// GetCapsuleComponent()->SetHiddenInGame(false);

	//	if (const auto EnemyAttributesData = EnemyAttributesDT.DataTable->FindRow<FEnemyAttributesData>(EnemyType, ""))
	//	{
	//		GetDamageSystemComp()->SetMaxHealth(EnemyAttributesData->MaxHealth);
	//		GetDamageSystemComp()->SetHealth(EnemyAttributesData->MaxHealth);
	//		GetDamageSystemComp()->SetImpulsePower(EnemyAttributesData->partsKnockback_Weak,
	//			EnemyAttributesData->partsKnockback_Normal, EnemyAttributesData->partsKnockback_Hard);
	//		GetDamageSystemComp()->SetPartsHealth(EnemyAttributesData->HeadHealth, EnemyAttributesData->BodyHealth, EnemyAttributesData->RArmHealth,
	//			EnemyAttributesData->LArmHealth, EnemyAttributesData->RLegHealth, EnemyAttributesData->LLegHealth);

	//		GetCharacterMovement()->MaxWalkSpeed = EnemyAttributesData->MaxWalkSpeed;

	//		AttackDamageAmount = EnemyAttributesData->AttackDamageAmount;
	//		AttackRate = EnemyAttributesData->AttackRate;
	//		MeleeAttackRange = EnemyAttributesData->MeleeAttackRange;
	//		MeleeAttackSphereRadius = EnemyAttributesData->MeleeAttackSphereRadius;

	//		MaxWalkSpeed = EnemyAttributesData->MaxWalkSpeed;
	//		MinWalkSpeedVariation = EnemyAttributesData->MinWalkSpeedVariation;
	//		MaxWalkSpeedVariation = EnemyAttributesData->MaxWalkSpeedVariation;
	//		MinAttackRateVariation = EnemyAttributesData->MinAttackRateVariation;
	//		MaxAttackRateVariation = EnemyAttributesData->MaxAttackRateVariation;

	//		HitAnimations = EnemyAttributesData->HitAnimations;
	//		DeathAnimations = EnemyAttributesData->DeathAnimations;
	//		AttackAnimations = EnemyAttributesData->AttackAnimations;
	//		ClimbAnimation = EnemyAttributesData->ClimbAnimation;
	//		ThrowAnimation = EnemyAttributesData->ThrowAnimation;
	//		FallingAnimation = EnemyAttributesData->FallingAnimation;
	//	}

	//	PlayerController = GetWorld()->GetFirstPlayerController();

	//	bIsLevelSequenceSpawned = false;

	//	isInitialized = true;
	//}
}

void AEnemyBase::BindKillLogOnDeath()
{
	//// AddSkull 중복 호출 방지
	//if (bIsDeathEventBound)
	//{
	//	return;
	//}

	//if (UACDamageSystem* DamageSystem = FindComponentByClass<UACDamageSystem>())
	//{
	//	DamageSystem->OnDeath.AddLambda([this]()
	//		{
	//			// KillLog 호출 로직
	//			// 3. 플레이어 가져오기
	//			APlayerController* PC = GetPlayerController();
	//			if (!PC) return;

	//			APawnPlayer* Player = Cast<APawnPlayer>(PC->GetPawn());
	//			if (!Player) return;

	//			// 4. UIManager → KillLogManager 가져와서 호출
	//			if (UACUIMangerComponent* UIManager = Player->FindComponentByClass<UACUIMangerComponent>())
	//			{
	//				if (UACKillLogManager* KLM = UIManager->GetKillLogManager())
	//				{
	//					KLM->AddKillLog(Player->GetName(), this->GetName());
	//				}
	//			}

	//			if (DeathSound)
	//			{
	//				UGameplayStatics::PlaySound2D(GetWorld(), DeathSound);
	//			}
	//		});

	//	bIsDeathEventBound = true;
	//}
}

void AEnemyBase::TurnOffAIController(bool bIsLSSpawned)
{
	//GetAIController()->EndPursueState();
	//GetAIController()->GetBrainComponent()->StopLogic("Turn Off AIC");

	//if (bIsLSSpawned)
	//{
	//	bIsLevelSequenceSpawned = true;
	//	GetDamageSystemComp()->OnDeath.RemoveAll(this);
	//}
}

void AEnemyBase::TurnOnAIController()
{
	//if (bIsLevelSequenceSpawned && GetDamageSystemComp()->GetIsDead())
	//{
	//	OnDeathTriggered();
	//	return;
	//}

	//bIsLevelSequenceSpawned = false;
	//GetDamageSystemComp()->OnDeath.AddUObject(this, &AEnemyBase::OnDeathTriggered);
	//UAnimInstance* const EnemyAnimInstance = GetMesh()->GetAnimInstance();
	//EnemyAnimInstance->StopAllMontages(true); // restart anim instance

	//GetAIController()->GetBrainComponent()->RestartLogic();
}

void AEnemyBase::SetUpAIController(AEnemyBaseAIController* const NewAIController)
{
	AIController = NewAIController;
}