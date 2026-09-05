#include "SoundSystem/EnemyAudioComponent.h"
#include "SoundSystem/EnemyAudioDataAsset.h"
#include "SoundSystem/GameAudioSubsystem.h"

#include "Characters/Enemies/EnemyBase.h"

#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"


UEnemyAudioComponent::UEnemyAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	Enemy = Cast<AEnemyBase>(GetOwner());
}

bool UEnemyAudioComponent::CanPlayEvent(EEnemyAudioEvent AudioEvent) const
{
	if (!GetWorld()) { return false; }
	if (AudioEvent == EEnemyAudioEvent::None)
	{
		return false;
	}

	const float* NextAllowedTime = NextAllowedPlayTimes.Find(AudioEvent);

	if (!NextAllowedTime)
	{
		return true;
	}

	return GetWorld()->GetTimeSeconds() >= *NextAllowedTime;
}

bool UEnemyAudioComponent::PassesPlayProbability(const FEnemyAudioEventData& EventData) const
{
	if (EventData.PlayProbability >= 1.f)
	{
		return true;
	}

	if (EventData.PlayProbability <= 0.f)
	{
		return false;
	}

	return FMath::FRand() <= EventData.PlayProbability;
}

ESoundID UEnemyAudioComponent::SelectWeightedSound_1(EEnemyAudioEvent AudioEvent, const FEnemyAudioEventData& EventData) const
{
	if (EventData.Variations.IsEmpty())
	{
		return ESoundID::None;
	}

	const ESoundID* LastPlayedSound = LastPlayedSounds.Find(AudioEvent);

	float TotalWeight = 0.f;

	for (const FEnemySoundVariation& Variation : EventData.Variations)
	{
		if (Variation.SoundID == ESoundID::None)
		{
			continue;
		}

		if (Variation.Weight <= 0.f)
		{
			continue;
		}

		if (EventData.Variations.Num() > 1 && LastPlayedSound && Variation.SoundID == *LastPlayedSound)
		{
			continue;
		}

		TotalWeight += Variation.Weight;
	}

	/*
	 * 마지막 Sound 제외로 인해 후보가 전부 사라졌다면
	 * Repeat Prevention을 무시하고 다시 계산한다.
	 */
	// ??? 

	if (TotalWeight <= 0.f)
	{
		for (const FEnemySoundVariation& Variation : EventData.Variations)
		{
			if (Variation.SoundID == ESoundID::None)
			{
				continue;
			}

			if (Variation.Weight <= 0.f)
			{
				continue;
			}

			TotalWeight += Variation.Weight;
		}
	}

	if (TotalWeight <= 0.f)
	{
		return ESoundID::None;
	}

	float RandomValue = FMath::FRandRange(0.f, TotalWeight);

	for (const FEnemySoundVariation& Variation : EventData.Variations)
	{
		if (Variation.SoundID == ESoundID::None ||
			Variation.Weight <= 0.f)
		{
			continue;
		}

		const bool bShouldSkipRepeatedSound =
			EventData.Variations.Num() > 1 &&
			LastPlayedSound &&
			Variation.SoundID == *LastPlayedSound;

		if (bShouldSkipRepeatedSound)
		{
			continue;
		}

		RandomValue -= Variation.Weight;

		if (RandomValue <= 0.f)
		{
			return Variation.SoundID;
		}
	}

	/*
	 * Repeat Prevention 때문에 선택이 실패했다면
	 * 정상적인 첫 번째 후보 반환.
	 */
	for (const FEnemySoundVariation& Variation : EventData.Variations)
	{
		if (Variation.SoundID != ESoundID::None &&
			Variation.Weight > 0.f)
		{
			return Variation.SoundID;
		}
	}

	return ESoundID::None;
}


ESoundID UEnemyAudioComponent::SelectWeightedSound_2(
	EEnemyAudioEvent AudioEvent,
	const FEnemyAudioEventData& EventData) const
{
	if (EventData.Variations.IsEmpty())
	{
		return ESoundID::None;
	}

	const ESoundID* LastPlayedSound = LastPlayedSounds.Find(AudioEvent);

	TArray<const FEnemySoundVariation*> Candidates;
	float TotalWeight = 0.f;

	for (const FEnemySoundVariation& Variation : EventData.Variations)
	{
		if (Variation.SoundID == ESoundID::None ||
			Variation.Weight <= 0.f)
		{
			continue;
		}

		if (LastPlayedSound &&
			EventData.Variations.Num() > 1 &&
			Variation.SoundID == *LastPlayedSound)
		{
			continue;
		}

		Candidates.Add(&Variation);
		TotalWeight += Variation.Weight;
	}

	if (Candidates.IsEmpty())
	{
		for (const FEnemySoundVariation& Variation : EventData.Variations)
		{
			if (Variation.SoundID == ESoundID::None ||
				Variation.Weight <= 0.f)
			{
				continue;
			}

			Candidates.Add(&Variation);
			TotalWeight += Variation.Weight;
		}
	}

	if (Candidates.IsEmpty() || TotalWeight <= 0.f)
	{
		return ESoundID::None;
	}

	float RandomWeight = FMath::FRandRange(0.f, TotalWeight);

	for (const FEnemySoundVariation* Candidate : Candidates)
	{
		if (!Candidate)
		{
			continue;
		}

		RandomWeight -= Candidate->Weight;

		if (RandomWeight <= 0.f)
		{
			return Candidate->SoundID;
		}
	}

	return Candidates.Last()->SoundID;
}

void UEnemyAudioComponent::UpdateEventCooldown(EEnemyAudioEvent AudioEvent,const FEnemyAudioEventData& EventData)
{
	if (!GetWorld())
	{
		return;
	}

	const float MinCooldown =
		FMath::Min(EventData.CooldownMin, EventData.CooldownMax);

	const float MaxCooldown =
		FMath::Max(EventData.CooldownMin, EventData.CooldownMax);

	const float Cooldown =
		FMath::FRandRange(MinCooldown, MaxCooldown);

	NextAllowedPlayTimes.FindOrAdd(AudioEvent) =
		GetWorld()->GetTimeSeconds() + Cooldown;
}

bool UEnemyAudioComponent::CanInterruptCurrentVocal(
	const FEnemyAudioEventData& NewEventData) const
{
	if (!IsValid(VocalAudioComponent))
	{
		return true;
	}

	if (!VocalAudioComponent->IsPlaying())
	{
		return true;
	}

	if (!NewEventData.bInterruptLowerPriority)
	{
		return false;
	}

	return NewEventData.Priority > CurrentVocalPriority;
}


void UEnemyAudioComponent::PlayVocalSound(EEnemyAudioEvent AudioEvent, ESoundID SoundID, const FEnemyAudioEventData& EventData)
{
	if (!IsValid(Enemy))
	{
		return;
	}

	UGameInstance* GameInstance = Enemy->GetGameInstance();

	if (!IsValid(GameInstance))
	{
		return;
	}

	UGameAudioSubsystem* AudioSubsystem =
		GameInstance->GetSubsystem<UGameAudioSubsystem>();

	if (!IsValid(AudioSubsystem))
	{
		return;
	}

	if (IsValid(VocalAudioComponent))
	{
		VocalAudioComponent->OnAudioFinished.RemoveDynamic(
			this,
			&UEnemyAudioComponent::HandleVocalFinished
		);

		if (VocalAudioComponent->IsPlaying())
		{
			VocalAudioComponent->Stop();
		}
	}

	VocalAudioComponent =
		AudioSubsystem->PlaySoundAttached(
			SoundID,
			Enemy->GetRootComponent()
		);

	if (!IsValid(VocalAudioComponent))
	{
		ResetCurrentVocalState();
		return;
	}

	CurrentVocalEvent = AudioEvent;
	CurrentVocalPriority = EventData.Priority;

	VocalAudioComponent->OnAudioFinished.AddDynamic(
		this,
		&UEnemyAudioComponent::HandleVocalFinished
	);
}

void UEnemyAudioComponent::HandleVocalFinished()
{
	if (IsValid(VocalAudioComponent))
	{
		VocalAudioComponent->OnAudioFinished.RemoveDynamic(
			this,
			&UEnemyAudioComponent::HandleVocalFinished
		);
	}

	ResetCurrentVocalState();
}


void UEnemyAudioComponent::ResetCurrentVocalState()
{
	VocalAudioComponent = nullptr;
	CurrentVocalEvent = EEnemyAudioEvent::None;
	CurrentVocalPriority = INDEX_NONE;
}


void UEnemyAudioComponent::StopVocal()
{
	if (IsValid(VocalAudioComponent))
	{
		VocalAudioComponent->OnAudioFinished.RemoveDynamic(
			this,
			&UEnemyAudioComponent::HandleVocalFinished
		);

		VocalAudioComponent->Stop();
	}

	ResetCurrentVocalState();
}


bool UEnemyAudioComponent::PlayEvent(EEnemyAudioEvent AudioEvent)
{
	if (AudioEvent == EEnemyAudioEvent::None) { return false; }
	if (!CanPlayEvent(AudioEvent)) { return false; }

	const FEnemyAudioEventData* EventData = GetEventData(AudioEvent);
	if (!EventData) { return false; }

	if (!PassesPlayProbability(*EventData))
	{
		return false;
	}

	if (!CanInterruptCurrentVocal(*EventData))
	{
		return false;
	}

	const ESoundID SelectedSound = SelectWeightedSound_2(AudioEvent, *EventData);

	if (SelectedSound == ESoundID::None)
	{
		return false;
	}

	PlayVocalSound(
		AudioEvent,
		SelectedSound,
		*EventData
	);

	LastPlayedSounds.FindOrAdd(AudioEvent) =
		SelectedSound;

	UpdateEventCooldown(
		AudioEvent,
		*EventData
	);

	return true;
}

void UEnemyAudioComponent::StartAmbientVocal(EEnemyAudioEvent AudioEvent)
{
	if (!GetWorld()) { return; }
	StopAmbientVocal();
	CurrentAmbientAudioEvent = AudioEvent;
	ScheduleNextAmbientVocal();
}

void UEnemyAudioComponent::StopAmbientVocal()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AmbientVocalTimerHandle);
	}

	CurrentAmbientAudioEvent = EEnemyAudioEvent::None;
}

void UEnemyAudioComponent::ScheduleNextAmbientVocal()
{
	if (!GetWorld()) { return; }
	if (CurrentAmbientAudioEvent == EEnemyAudioEvent::None)
	{
		return;
	}

	//---------------------
	// <TODO: Interval 설정>

	//const FEnemyAudioEventData* EventData =
	//	GetEventData(CurrentAmbientAudioEvent);

	//const float Interval =
	//	FMath::FRandRange(
	//		EventData->AmbientIntervalMin,
	//		EventData->AmbientIntervalMax
	//	);

	//----------------------

	const float MinInterval =
		FMath::Min(AmbientIntervalMin, AmbientIntervalMax);

	const float MaxInterval =
		FMath::Max(AmbientIntervalMin, AmbientIntervalMax);

	const float Interval =
		FMath::FRandRange(MinInterval, MaxInterval);

	GetWorld()->GetTimerManager().SetTimer(
		AmbientVocalTimerHandle,
		this,
		&UEnemyAudioComponent::HandleAmbientVocalTimer,
		Interval,
		false
	);
}

void UEnemyAudioComponent::HandleAmbientVocalTimer()
{
	if (CurrentAmbientAudioEvent == EEnemyAudioEvent::None)
	{
		return;
	}

	PlayEvent(CurrentAmbientAudioEvent);

	ScheduleNextAmbientVocal();
}

void UEnemyAudioComponent::NotifyStateChanged(EEnemyStateType NewState)
{
	HandleStateAudio(NewState);
	UpdateBreathingFromState(NewState);
}

void UEnemyAudioComponent::HandleStateAudio(EEnemyStateType NewState)
{
	switch (NewState)
	{
	case EEnemyStateType::Idle:
		StartAmbientVocal(EEnemyAudioEvent::IdleVocal);
		break;

	case EEnemyStateType::Patrol:
		StartAmbientVocal(EEnemyAudioEvent::PatrolVocal);
		break;

	case EEnemyStateType::Investigate:
		StopAmbientVocal();
		PlayEvent(EEnemyAudioEvent::Investigate);
		break;

	case EEnemyStateType::TrackBlood:
		StopAmbientVocal();
		PlayEvent(EEnemyAudioEvent::Investigate);
		break;

	case EEnemyStateType::Chase:
		PlayEvent(EEnemyAudioEvent::ChaseStart);
		StartAmbientVocal(EEnemyAudioEvent::ChaseVocal);
		break;

	case EEnemyStateType::Attack:
		/*
		 * Attack 사운드는 Animation Notify에서 처리하는 것을 권장.
		 */
		StopAmbientVocal();
		break;

	case EEnemyStateType::Flee:
		StopAmbientVocal();
		PlayEvent(EEnemyAudioEvent::Flee);
		break;

	case EEnemyStateType::Hide:
		StopAmbientVocal();
		PlayEvent(EEnemyAudioEvent::Hide);
		break;

	case EEnemyStateType::Recover:
		StopAmbientVocal();
		PlayEvent(EEnemyAudioEvent::Recover);
		break;

	case EEnemyStateType::Dead:
		StopAmbientVocal();
		break;
	}
}

void UEnemyAudioComponent::NotifyDamage(float DamageAmount)
{
	if (DamageAmount <= 0.f)
	{
		return;
	}

	if (DamageAmount >= HeavyDamageThreshold)
	{
		PlayEvent(EEnemyAudioEvent::PainHeavy);
	}
	else
	{
		PlayEvent(EEnemyAudioEvent::PainLight);
	}
}

void UEnemyAudioComponent::NotifyAttack()
{
	PlayEvent(EEnemyAudioEvent::Attack);
}

void UEnemyAudioComponent::NotifyDeath()
{
	StopAmbientVocal();

	StopVocal();

	PlayEvent(EEnemyAudioEvent::Death);

	SetBreathingEnabled(false);
}

void UEnemyAudioComponent::SetBreathingEnabled(bool bEnabled)
{
	bBreathingEnabled = bEnabled;

	if (!bEnabled)
	{
		if (IsValid(BreathingAudioComponent))
		{
			BreathingAudioComponent->Stop();
			BreathingAudioComponent = nullptr;
		}

		return;
	}

	if (BreathingSoundID == ESoundID::None ||
		!IsValid(Enemy))
	{
		return;
	}

	if (IsValid(BreathingAudioComponent))
	{
		return;
	}

	UGameInstance* GameInstance =
		Enemy->GetGameInstance();

	if (!IsValid(GameInstance))
	{
		return;
	}

	UGameAudioSubsystem* AudioSubsystem =
		GameInstance->GetSubsystem<UGameAudioSubsystem>();

	if (!IsValid(AudioSubsystem))
	{
		return;
	}

	BreathingAudioComponent =
		AudioSubsystem->PlaySoundAttached(
			BreathingSoundID,
			Enemy->GetRootComponent()
		);
}

void UEnemyAudioComponent::SetBreathingIntensity(float Intensity)
{
	if (!IsValid(BreathingAudioComponent))
	{
		return;
	}

	BreathingAudioComponent->SetFloatParameter(
		TEXT("Intensity"),
		FMath::Clamp(Intensity, 0.f, 1.f)
	);
}

void UEnemyAudioComponent::UpdateBreathingFromState(EEnemyStateType NewState)
{
	if (NewState == EEnemyStateType::Dead)
	{
		SetBreathingEnabled(false);
		return;
	}

	SetBreathingEnabled(true);

	switch (NewState)
	{
	case EEnemyStateType::Idle:
		SetBreathingIntensity(0.2f);
		break;

	case EEnemyStateType::Patrol:
		SetBreathingIntensity(0.3f);
		break;

	case EEnemyStateType::Investigate:
	case EEnemyStateType::TrackBlood:
		SetBreathingIntensity(0.45f);
		break;

	case EEnemyStateType::Chase:
		SetBreathingIntensity(0.9f);
		break;

	case EEnemyStateType::Attack:
		SetBreathingIntensity(1.f);
		break;

	case EEnemyStateType::Flee:
		SetBreathingIntensity(1.f);
		break;

	case EEnemyStateType::Hide:
		SetBreathingIntensity(0.55f);
		break;

	case EEnemyStateType::Recover:
		SetBreathingIntensity(0.75f);
		break;

	default:
		SetBreathingIntensity(0.3f);
		break;
	}
}

const FEnemyAudioEventData* UEnemyAudioComponent::GetEventData(EEnemyAudioEvent AudioEvent) const
{
	if (!IsValid(AudioDataAsset))
	{
		return nullptr;
	}

	return AudioDataAsset->FindEventData(AudioEvent);
}