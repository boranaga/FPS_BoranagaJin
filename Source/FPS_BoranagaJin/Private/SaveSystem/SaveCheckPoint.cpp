#include "SaveSystem/SaveCheckpoint.h"

#include "Characters/Player/CharacterPlayer.h"
#include "SaveSystem/SaveGameSubsystem.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCheckpoint, Log, All);

ACheckpoint::ACheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 120.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);

	RespawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(SceneRoot);
	RespawnPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

	CheckpointMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckpointMesh"));
	CheckpointMesh->SetupAttachment(SceneRoot);
	CheckpointMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACheckpoint::BeginPlay()
{
	Super::BeginPlay();

	ValidateCheckpointID();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ACheckpoint::HandleCheckpointOverlap);
	}

	/*
	 * 저장된 마지막 체크포인트가 현재 Actor와 같다면,
	 * 로드 직후 활성화된 시각 상태를 적용합니다.
	 */
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>())
		{
			const FCheckpointSaveData& SavedCheckpoint = SaveSubsystem->GetCurrentCheckpointData();
			if (SavedCheckpoint.IsValid() && SavedCheckpoint.CheckpointID == CheckpointID)
			{
				bActivated = true;
				ApplyActivatedVisualState();
			}
		}
	}
}

void ACheckpoint::HandleCheckpointOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ACharacterPlayer* Player = Cast<ACharacterPlayer>(OtherActor);
	if (!IsValid(Player)) { return; }
	if (bActivated && bActivateOnlyOnce && !bAllowRepeatedSave) { return; }
	ActivateCheckpoint(Player);
}

void ACheckpoint::ActivateCheckpoint(ACharacterPlayer* Player)
{
	if (!IsValid(Player)) { return; }

	if (CheckpointID.IsNone())
	{
		UE_LOG(
			LogCheckpoint,
			Error,
			TEXT(
				"Checkpoint activation failed: "
				"CheckpointID is None. Actor=%s"
			),
			*GetName()
		);

		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance)) { return; }

	USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>();

	if (!IsValid(SaveSubsystem))
	{
		UE_LOG(
			LogCheckpoint,
			Error,
			TEXT(
				"Checkpoint activation failed: "
				"SaveGameSubsystem was not found."
			)
		);

		return;
	}

	const bool bWasAlreadyActivated = bActivated;

	SaveSubsystem->SetCurrentCheckpoint(CheckpointID, GetRespawnTransform());

	bActivated = true;

	if (!bWasAlreadyActivated)
	{
		PlayActivationEffects();
		ApplyActivatedVisualState();
	}
	if (bAutoSaveOnActivation)
	{
		const bool bSaveStarted = SaveSubsystem->SaveGameAsync();
		if (!bSaveStarted)
		{
			UE_LOG(
				LogCheckpoint,
				Warning,
				TEXT(
					"Checkpoint was activated, "
					"but asynchronous save did not start. "
					"CheckpointID=%s"
				),
				*CheckpointID.ToString()
			);
		}
	}

	if (bActivateOnlyOnce && !bAllowRepeatedSave && TriggerBox)
	{
		TriggerBox->SetGenerateOverlapEvents(false);
	}

	UE_LOG(
		LogCheckpoint,
		Log,
		TEXT(
			"Checkpoint activated. "
			"ID=%s, RespawnLocation=%s"
		),
		*CheckpointID.ToString(),
		*GetRespawnTransform()
		.GetLocation()
		.ToString()
	);
}

FTransform ACheckpoint::GetRespawnTransform() const
{
	if (IsValid(RespawnPoint))
	{
		return RespawnPoint->GetComponentTransform();
	}
	return GetActorTransform();
}

void ACheckpoint::PlayActivationEffects()
{
	const FVector EffectLocation = GetActorLocation() + EffectLocationOffset;
	if (ActivationEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ActivationEffect, EffectLocation, GetActorRotation());
	}
	if (ActivationSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ActivationSound, EffectLocation);
	}
}

void ACheckpoint::ApplyActivatedVisualState()
{
	if (!CheckpointMesh) { return; }
	if (ActivatedMaterial)
	{
		CheckpointMesh->SetMaterial(0, ActivatedMaterial);
	}
}

void ACheckpoint::ValidateCheckpointID()
{
	if (!CheckpointID.IsNone()) { return; }

	UE_LOG(
		LogCheckpoint,
		Error,
		TEXT(
			"Checkpoint has no ID. "
			"Actor=%s, Level=%s"
		),
		*GetName(),
		GetWorld()
		? *GetWorld()->GetMapName()
		: TEXT("InvalidWorld")
	);
}

#if WITH_EDITOR

void ACheckpoint::PostEditImport()
{
	Super::PostEditImport();

	CheckpointID = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
}

void ACheckpoint::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	if (DuplicateMode == EDuplicateMode::Normal)
	{
		CheckpointID = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}
}

void ACheckpoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ValidateCheckpointID();
}

#endif