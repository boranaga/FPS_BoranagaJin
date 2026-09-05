
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/HealthComponent.h"
#include "Characters/StaminaComponent.h"
#include "Characters/BloodTrailComponent.h"
#include "Characters/Enemies/EnemyStateMachineComponent.h"
#include "Characters/GameDamageType.h"

#include "SoundSystem/EnemyAudioComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

//TODO: Custom Damage Component로 변경해야함
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AEnemyBaseAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));
	BloodTrailComponent = CreateDefaultSubobject<UBloodTrailComponent>(TEXT("BloodTrailComponent"));
	EnemyAudioComponent = CreateDefaultSubobject<UEnemyAudioComponent>(TEXT("AudioComponent"));
	StateMachineComponent = CreateDefaultSubobject<UEnemyStateMachineComponent>(TEXT("StateMachineComponent"));

	Tags.AddUnique(TEXT("Enemy"));
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();


	UE_LOG(LogTemp, Warning, TEXT("Enemy BeginPlay: %s"), *GetName());

	AAIController* AIController = Cast<AAIController>(GetController());

	if (IsValid(AIController))
	{
		UE_LOG(LogTemp, Warning, TEXT("AIController Valid: %s"), *AIController->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AIController INVALID: %s"), *GetName());
	}

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddUObject(this, &AEnemyBase::OnDeath);
	}
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	#if ENABLE_DRAW_DEBUG

		if (StateMachineComponent)
		{
			StateMachineComponent->DrawDebug();
		}

	#endif
}

float AEnemyBase::GetCurrentHealth() const
{
	return HealthComponent ? HealthComponent->GetCurrentHealth() : 0.f;
}

float AEnemyBase::GetMaxHealth() const
{
	return HealthComponent ? HealthComponent->GetMaxHealth() : 0.f;
}

float AEnemyBase::ReceiveDamage(const FDamageParams& DamageInfo)
{
	if (!HealthComponent || HealthComponent->IsDead()) { return 0.f; }
	float ActualDamage = DamageInfo.DamageAmount;
	if (DamageInfo.bIsCritical)
	{
		ActualDamage *= 2.f; //TODO: Set Critical Damage as Variable
	}
	const float AppliedDamage = HealthComponent->ApplyDamage(ActualDamage);

	//------------------------------------------------

	//// <Old>
	//if (!HealthComponent->IsDead())
	//{
	//	AActor* InstigatorActor = nullptr;

	//	if (DamageInfo.InstigatorController)
	//	{
	//		InstigatorActor = DamageInfo.InstigatorController->GetPawn();
	//	}

	//	if (!InstigatorActor)
	//	{
	//		InstigatorActor = DamageInfo.DamageCauser;
	//	}

	//	OnDamagedBy(InstigatorActor);
	//}

	// <New>

	AActor* DamageInstigator = nullptr;

	if (DamageInfo.InstigatorController)
	{
		DamageInstigator = DamageInfo.InstigatorController->GetPawn();
	}

	if (!DamageInstigator)
	{
		DamageInstigator = DamageInfo.DamageCauser;
	}

	//if (BloodTrailComponent)
	//{
	//	BloodTrailComponent->NotifyDamageReceived(AppliedDamage);
	//}

	if (!HealthComponent->IsDead() && StateMachineComponent)
	{
		StateMachineComponent->NotifyDamageReceived(AppliedDamage, DamageInstigator);
	}

	//------------------------------------------------
	return AppliedDamage;
}

bool AEnemyBase::IsDead() const
{
	return HealthComponent ? HealthComponent->IsDead() : true;
}

void AEnemyBase::OnDeath()
{
	if (StateMachineComponent)
	{
		StateMachineComponent->SetState(EEnemyStateType::Dead);
	}

	// 여기서 기존 Die 처리 연결
	// StopMovement, DisableCollision, DeathMontage, Ragdoll, SetLifeSpan 등
}

void AEnemyBase::OnDamagedBy(AActor* DamageInstigatorActor)
{
	if (!DamageInstigatorActor) return;
	if (!StateMachineComponent) return;

	StateMachineComponent->SetTarget(DamageInstigatorActor);
	StateMachineComponent->SetState(EEnemyStateType::Chase);
}

//void AEnemyBase::TakeEnemyDamage(float DamageAmount)
//{
//	if (IsDead()) return;
//
//	//CurrentHealth -= DamageAmount;
//
//	if (CurrentHealth <= 0.f)
//	{
//		CurrentHealth = 0.f;
//		Die();
//	}
//}

void AEnemyBase::AttackTarget(AActor* Target)
{
	if (!Target) return;
	//UE_LOG(LogTemp, Warning, TEXT("Enemy Attack Target: %s"), *Target->GetName());

	// 나중에 여기서:
	// 1. Attack Montage 재생
	// 2. AnimNotify에서 실제 데미지 적용
	// 3. 무기 Trace / Melee Hit 처리

	if (AttackMontage)
	{
		PlayAnimMontage(AttackMontage);
	}

	// 지금은 테스트용으로 즉시 데미지
	// 나중에는 AnimNotify에서 ApplyAttackDamage(Target)을 호출하는 구조가 좋음
	ApplyAttackDamage(Target);
}

void AEnemyBase::ApplyAttackDamage(AActor* Target)
{
	if (!Target) return;

	FDamageParams Damage;
	Damage.DamageAmount = AttackDamage;
	Damage.DamageType = EGameDamageType::Melee;
	Damage.bCanForceDamage = false;
	//Damage.HitBoneName = BoneName;
	//Damage.ImpulseDirection = ImpulseDirection;
	//Damage.SurfaceType = SurfaceType;
	//Damage.ImpactPoint = ImpactPoint;
	Damage.DamageCauser = this;

	if (Target->GetClass()->ImplementsInterface(UDamageableInterface::StaticClass()))
	{
		Cast<IDamageableInterface>(Target)->ReceiveDamage(Damage);
	}
}

void AEnemyBase::Die()
{
	UEnemyStateMachineComponent* StateMachine = GetStateMachineComponent();
	if (StateMachine)
	{
		StateMachine->SetState(EEnemyStateType::Dead);
	}

	AEnemyBaseAIController* EnemyAIController = Cast<AEnemyBaseAIController>(GetController());
	if (EnemyAIController)
	{
		EnemyAIController->StopAIMovement();
		EnemyAIController->DisablePerception();
	}

	GetCharacterMovement()->DisableMovement();

	DisableEnemyCollision();

	if (bUseRagdollOnDeath)
	{
		StartRagdoll();
	}
	else if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}

	//-------------------------------------

	// <Object Pooling 사용 안할 경우>
	SetLifeSpan(DestroyDelayAfterDeath);

	//// <Obejct Pooling 사용할 경우>
	//GetWorld()->GetTimerManager().SetTimer(
	//	DeathTimerHandle,
	//	this,
	//	&AEnemyCharacter::ReturnToPool,
	//	DestroyDelayAfterDeath,
	//	false
	//);

}

void AEnemyBase::DisableEnemyCollision()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Mesh는 사망 애니메이션을 보여줘야 하므로 기본적으로 Query만 유지하거나 NoCollision 처리 가능
	if (!bUseRagdollOnDeath)
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AEnemyBase::StartRagdoll()
{
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
}
