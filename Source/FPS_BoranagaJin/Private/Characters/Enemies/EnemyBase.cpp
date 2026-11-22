
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/HealthComponent.h"
#include "Characters/StaminaComponent.h"
#include "Characters/BloodTrailComponent.h"
#include "Characters/Enemies/EnemyStateMachineComponent.h"
#include "Characters/GameDamageType.h"

#include "SoundSystem/EnemyAudioComponent.h"
#include "SoundSystem/FootstepComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

//TODO: Custom Damage Component�� �����ؾ���
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
	FootstepComponent = CreateDefaultSubobject<UFootstepComponent>(TEXT("FootstepComponent"));
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

	if (IsValid(FootstepComponent))
	{
		FootstepComponent->SetFootstepMesh(GetMesh());
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
		StateMachineComponent->NotifyDamageReceived(AppliedDamage, DamageInstigator, DamageInfo.DamageType);
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

	// ���⼭ ���� Die ó�� ����
	// StopMovement, DisableCollision, DeathMontage, Ragdoll, SetLifeSpan ��
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

	// ���߿� ���⼭:
	// 1. Attack Montage ���
	// 2. AnimNotify���� ���� ������ ����
	// 3. ���� Trace / Melee Hit ó��

	if (AttackMontage)
	{
		PlayAnimMontage(AttackMontage);
	}

	// ������ �׽�Ʈ������ ��� ������
	// ���߿��� AnimNotify���� ApplyAttackDamage(Target)�� ȣ���ϴ� ������ ����
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

	// <Object Pooling ��� ���� ���>
	SetLifeSpan(DestroyDelayAfterDeath);

	//// <Obejct Pooling ����� ���>
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

	// Mesh�� ��� �ִϸ��̼��� ������� �ϹǷ� �⺻������ Query�� �����ϰų� NoCollision ó�� ����
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
