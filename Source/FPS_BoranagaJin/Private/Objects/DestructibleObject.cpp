#include "Objects/DestructibleObject.h"
#include "Objects/FireAreaComponent.h"

#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Components/AudioComponent.h"


ADestructibleObject::ADestructibleObject()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	IntactMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IntactMesh"));
	IntactMesh->SetupAttachment(SceneRoot);
	IntactMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	IntactMesh->SetSimulatePhysics(false);

	BrokenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BrokenMesh"));
	BrokenMesh->SetupAttachment(SceneRoot);
	BrokenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BrokenMesh->SetSimulatePhysics(false);
	BrokenMesh->SetVisibility(false);

	FireAreaComponent = CreateDefaultSubobject<UFireAreaComponent>(TEXT("FireAreaComponent"));
	FireAreaComponent->SetupAttachment(SceneRoot);
	FireAreaComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// <Save>
	if (!SaveID.IsValid())
	{
		SaveID = FGuid::NewGuid();
	}
}

#if WITH_EDITOR
void ADestructibleObject::PostEditImport()
{
	Super::PostEditImport();

	// 에디터에서 복사된 Actor가 기존 Actor와 같은 ID를 가지지 않도록 합니다.
	SaveID = FGuid::NewGuid();
}

void ADestructibleObject::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	if (DuplicateMode == EDuplicateMode::Normal)
	{
		SaveID = FGuid::NewGuid();
	}
}
#endif

void ADestructibleObject::OnAfterLoad()
{
	if (bBroken)
	{
		// 파괴 상태를 시각적으로 다시 적용합니다.
		HideBrokenMesh();

		SetActorEnableCollision(false);

		// TODO: 여기서 중요한 점은 로드 시 BreakObject()를 직접 호출하지 않는 것.
		// BreakObject()가 폭발, 사운드, 아이템 드롭 등을 발생시킨다면 로딩할 때 모든 효과가 다시 실행될 수 있음.
		//따라서 저장 복원 전용 함수가 필요합니다.
		// 예시: void ADestructibleObject::ApplyBrokenVisualState() // 효과나 데미지는 발생시키지 않고 상태만 복원

	}
	else
	{
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
	}
}

void ADestructibleObject::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (BrokenMesh)
	{
		BrokenMesh->SetVisibility(false);
		BrokenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BrokenMesh->SetSimulatePhysics(false);
	}
}

float ADestructibleObject::ReceiveDamage(const FDamageParams& DamageData)
{
	if (bDestroyed) return 0.f;
	if (DamageData.DamageAmount <= 0.f) return 0.f;

	CurrentHealth -= DamageData.DamageAmount;

	if (CurrentHealth <= 0.f)
	{
		BreakObject(DamageData.ImpactPoint, DamageData.HitDirection);
	}
	else
	{
		if (bCanIgniteFromDamage && IgnitionDamageThreshold > CurrentHealth)
		{
			if (!bBurning)
			{
				StartBurning();
			}
		}
	}


	return DamageData.DamageAmount;
}

bool ADestructibleObject::IsDead() const
{
	return bDestroyed;
}

void ADestructibleObject::Ignite(AActor* FireCauser)
{
	StartBurning(FireCauser);
}

void ADestructibleObject::BreakObject(const FVector& HitLocation, const FVector& HitDirection)
{
	if (bDestroyed) return;

	StopBurning();

	bDestroyed = true;

	if (IntactMesh)
	{
		IntactMesh->SetVisibility(false);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		IntactMesh->SetSimulatePhysics(false);
	}

	if (BrokenMesh)
	{
		BrokenMesh->SetWorldTransform(IntactMesh->GetComponentTransform());

		BrokenMesh->SetVisibility(true);
		BrokenMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BrokenMesh->SetSimulatePhysics(true);

		const FVector ImpulseDirection = HitDirection.GetSafeNormal();

		BrokenMesh->AddImpulseAtLocation(
			ImpulseDirection * BreakImpulse,
			HitLocation,
			NAME_None
		);
	}

	if (BreakEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			BreakEffect,
			HitLocation,
			HitDirection.Rotation()
		);
	}

	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			BreakSound,
			HitLocation
		);
	}

	if (bDestroyActorAfterBreak)
	{
		SetLifeSpan(BrokenMeshLifeTime);
	}
	else if (bHideBrokenMeshAfterDelay)
	{
		GetWorldTimerManager().SetTimer(
			HideBrokenMeshTimerHandle,
			this,
			&ADestructibleObject::HideBrokenMesh,
			BrokenMeshLifeTime,
			false
		);
	}
}

void ADestructibleObject::HideBrokenMesh()
{
	if (BrokenMesh)
	{
		BrokenMesh->SetVisibility(false);
		BrokenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BrokenMesh->SetSimulatePhysics(false);
	}
}

void ADestructibleObject::StartBurning(AActor* FireCauser)
{
	if (bDestroyed) return;
	if (!bCanBurn) return;
	if (bBurning) return;

	bBurning = true;
	BurnCauser = FireCauser;

	if (FireAreaComponent)
	{
		FireAreaComponent->ActivateFireArea(FireCauser);
	}

	if (FireEffect)
	{
		ActiveFireEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
			FireEffect,
			IntactMesh,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	if (FireStartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			FireStartSound,
			GetActorLocation()
		);
	}

	if (FireLoopSound)
	{
		ActiveFireLoopSound = UGameplayStatics::SpawnSoundAttached(
			FireLoopSound,
			GetRootComponent()
		);
	}

	GetWorldTimerManager().SetTimer(
		BurnDamageTimerHandle,
		this,
		&ADestructibleObject::ApplyBurnDamage,
		BurnDamageInterval,
		true
	);

	GetWorldTimerManager().SetTimer(
		BurnDurationTimerHandle,
		this,
		&ADestructibleObject::StopBurning,
		BurnDuration,
		false
	);
}

void ADestructibleObject::StopBurning()
{
	if (!bBurning) return;

	bBurning = false;
	BurnCauser = nullptr;

	GetWorldTimerManager().ClearTimer(BurnDamageTimerHandle);
	GetWorldTimerManager().ClearTimer(BurnDurationTimerHandle);

	if (FireAreaComponent)
	{
		FireAreaComponent->DeactivateFireArea();
	}

	if (ActiveFireEffect)
	{
		ActiveFireEffect->Deactivate();
		ActiveFireEffect = nullptr;
	}

	if (ActiveFireLoopSound)
	{
		ActiveFireLoopSound->Stop();
		ActiveFireLoopSound = nullptr;
	}
}

void ADestructibleObject::ApplyBurnDamage()
{
	if (bDestroyed)
	{
		StopBurning();
		return;
	}

	EGameDamageType::Fire;

	FDamageParams Damage;
	Damage.DamageAmount = BurnDamage;
	Damage.ImpactPoint = GetActorLocation();
	Damage.HitDirection = FVector::UpVector;
	Damage.DamageCauser = BurnCauser;

	ReceiveDamage(Damage);
}
