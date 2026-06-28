
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyStateMachineComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

//TODO: Custom Damage Component로 변경해야함
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AEnemyBaseAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	StateMachineComponent = CreateDefaultSubobject<UEnemyStateMachineComponent>(
		TEXT("StateMachineComponent")
	);

	CurrentHealth = MaxHealth;
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
}

float AEnemyBase::ReceiveDamage(const FDamageParams& DamageInfo)
{
	if (IsDead())
	{
		return 0.f;
	}

	const float ActualDamage = DamageInfo.DamageAmount;

	TakeEnemyDamage(ActualDamage);

	if (!IsDead())
	{
		AActor* InstigatorActor = nullptr;

		if (DamageInfo.InstigatorController)
		{
			InstigatorActor = DamageInfo.InstigatorController->GetPawn();
		}

		if (!InstigatorActor)
		{
			InstigatorActor = DamageInfo.DamageCauser;
		}

		if (StateMachineComponent && InstigatorActor)
		{
			StateMachineComponent->SetTarget(InstigatorActor);
			StateMachineComponent->SetState(EEnemyStateType::Chase);
		}
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Enemy Damage: Amount=%f, Type=%d"),
		ActualDamage,
		static_cast<int32>(DamageInfo.DamageType)
	);

	return ActualDamage;
}

bool AEnemyBase::IsDead() const
{
	return CurrentHealth <= 0.f;
}

void AEnemyBase::TakeEnemyDamage(float DamageAmount)
{
	if (IsDead()) return;

	CurrentHealth -= DamageAmount;

	if (CurrentHealth <= 0.f)
	{
		CurrentHealth = 0.f;
		Die();
	}
}

void AEnemyBase::AttackTarget(AActor* Target)
{
	if (!Target) return;

	UE_LOG(LogTemp, Warning, TEXT("Enemy Attack Target: %s"), *Target->GetName());

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

	// TODO: Custom Component로 변경해야함
	UGameplayStatics::ApplyDamage(
		Target,
		AttackDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);

	UE_LOG(LogTemp, Warning, TEXT("Enemy Apply Damage: %f"), AttackDamage);
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
