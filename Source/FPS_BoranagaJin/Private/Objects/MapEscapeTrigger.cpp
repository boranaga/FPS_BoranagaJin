#include "Objects/MapEscapeTrigger.h"
#include "GameFlowSubsystem.h"

#include "Characters/Player/CharacterPlayer.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"

AMapEscapeTrigger::AMapEscapeTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);

	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
}

void AMapEscapeTrigger::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &AMapEscapeTrigger::HandleBeginOverlap);
}

void AMapEscapeTrigger::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//TODO: trigger를 밟은 객체가 플레이어인지 확인하는 로직을 조금 더 구체적으로 할 필요 있음

	if (!bEnabled || bTriggered) { return; }
	ACharacterPlayer* Player = Cast<ACharacterPlayer>(OtherActor);
	if (!IsValid(Player)) { return; }
	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance)) { return; }
	UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>();
	if (!IsValid(GameFlowSubsystem)) { return; }

	bTriggered = true;
	TriggerBox->SetGenerateOverlapEvents(false);

	GameFlowSubsystem->EndGame(EGameEndReason::Escaped);
}