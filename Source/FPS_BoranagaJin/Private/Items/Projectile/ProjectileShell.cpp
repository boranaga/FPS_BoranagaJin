


#include "Items/Projectile/ProjectileShell.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AProjectileShell::AProjectileShell()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->SetCollisionObjectType(ECC_GameTraceChannel4);
	//CapsuleComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CapsuleComponent->SetCollisionProfileName(FName("ProjectileShell"));
	CapsuleComponent->SetSimulatePhysics(true);
	CapsuleComponent->InitCapsuleSize(4.f, 10.f);

	ProjectileShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileShell"));
	ProjectileShellMesh->SetupAttachment(RootComponent);
	ProjectileShellMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileShellMesh->SetCastShadow(false);

}

// Called when the game starts or when spawned
void AProjectileShell::BeginPlay()
{
	Super::BeginPlay();
	InitShell();
}

// Called every frame
void AProjectileShell::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectileShell::FellOutOfWorld(const UDamageType& dmgType)
{
	if (!ProjectileShellMesh) return;
	if (!CapsuleComponent) return;
	ProjectileShellMesh->SetVisibility(false);
	CapsuleComponent->SetSimulatePhysics(false);

	SetActorLocation(FVector::ZeroVector);
	SetActorRotation(FRotator::ZeroRotator);
}

void AProjectileShell::OutsideWorldBounds()
{
	if (!ProjectileShellMesh) return;
	if (!CapsuleComponent) return;
	ProjectileShellMesh->SetVisibility(false);
	CapsuleComponent->SetSimulatePhysics(false);

	SetActorLocation(FVector::ZeroVector);
	SetActorRotation(FRotator::ZeroRotator);
}

void AProjectileShell::InitShell()
{
	if (!ProjectileShellMesh) return;
	ProjectileShellMesh->SetVisibility(false);
}

void AProjectileShell::HideProjectileShell()
{
	if (!ProjectileShellMesh) return;
	ProjectileShellMesh->SetVisibility(false);
	//CapsuleComponent->SetSimulatePhysics(false);
}

void AProjectileShell::EjectShell(const FVector EjectLocation, const FRotator EjectRotation, const FVector EjectImpulse)
{
	if (GetWorld()->GetTimerManager().IsTimerActive(InvisibleTimer))
	{
		GetWorld()->GetTimerManager().ClearTimer(InvisibleTimer);
	}

	SetActorLocation(EjectLocation);
	SetActorRotation(EjectRotation);
	if (!ProjectileShellMesh) return;
	if (!CapsuleComponent) return;
	ProjectileShellMesh->SetVisibility(true);
	CapsuleComponent->SetSimulatePhysics(true);
	CapsuleComponent->AddImpulse(EjectImpulse, FName(), false);

	GetWorld()->GetTimerManager().SetTimer(InvisibleTimer, this, &AProjectileShell::HideProjectileShell, VisibleDuration, false);
}



