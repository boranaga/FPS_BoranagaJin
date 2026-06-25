
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyStateMachineComponent.h"

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
}