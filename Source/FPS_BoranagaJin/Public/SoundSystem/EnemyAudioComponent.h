#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/Enemies/EnemyStateType.h"
#include "SoundID.h"
#include "EnemyAudioEvent.h"
#include "EnemyAudioComponent.generated.h"

class AEnemyBase;
class UAudioComponent;
class UEnemyAudioDataAsset;

struct FEnemyAudioEventData;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UEnemyAudioComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UEnemyAudioComponent();
protected:
	virtual void BeginPlay() override;

#pragma region Event
public:
	bool PlayEvent(EEnemyAudioEvent AudioEvent);
	void StopVocal();
private:
	bool CanPlayEvent(EEnemyAudioEvent AudioEvent) const;
	bool PassesPlayProbability(const FEnemyAudioEventData& EventData) const;

	ESoundID SelectWeightedSound_1(EEnemyAudioEvent AudioEvent, const FEnemyAudioEventData& EventData) const;
	ESoundID SelectWeightedSound_2(EEnemyAudioEvent AudioEvent, const FEnemyAudioEventData& EventData) const;

	void UpdateEventCooldown(EEnemyAudioEvent AudioEvent, const FEnemyAudioEventData& EventData);
#pragma endregion

#pragma region State
public:
	void NotifyStateChanged(EEnemyStateType NewState);
private:
	void HandleStateAudio(EEnemyStateType NewState);
#pragma endregion

#pragma region AmbientVocal
private:
	void StartAmbientVocal(EEnemyAudioEvent AudioEvent);
	void StopAmbientVocal();
	void ScheduleNextAmbientVocal();
	void HandleAmbientVocalTimer();

private:
	EEnemyAudioEvent CurrentAmbientAudioEvent = EEnemyAudioEvent::None;

	FTimerHandle AmbientVocalTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Audio|Ambient", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float AmbientIntervalMin = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Audio|Ambient", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float AmbientIntervalMax = 7.f;
#pragma endregion

#pragma region Damage
public:
	void NotifyDamage(float DamageAmount);
	void NotifyAttack();
	void NotifyDeath();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Audio|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float HeavyDamageThreshold = 25.f;
#pragma endregion

#pragma region Vocal
private:
	bool CanInterruptCurrentVocal(const FEnemyAudioEventData& NewEventData) const;
	void PlayVocalSound(EEnemyAudioEvent AudioEvent, ESoundID SoundID, const FEnemyAudioEventData& EventData);
	void ResetCurrentVocalState();

	UFUNCTION()
	void HandleVocalFinished();

private:
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> VocalAudioComponent;

	int32 CurrentVocalPriority = INDEX_NONE;

	EEnemyAudioEvent CurrentVocalEvent = EEnemyAudioEvent::None;
#pragma endregion

#pragma region Breathing
public:
	void SetBreathingEnabled(bool bEnabled);
	void SetBreathingIntensity(float Intensity);

private:
	void UpdateBreathingFromState(EEnemyStateType NewState);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Audio|Breathing", meta = (AllowPrivateAccess = "true"))
	ESoundID BreathingSoundID = ESoundID::None;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BreathingAudioComponent;

	bool bBreathingEnabled = true;
#pragma endregion

#pragma region Data
private:
	const FEnemyAudioEventData* GetEventData(EEnemyAudioEvent AudioEvent) const;
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Audio", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyAudioDataAsset> AudioDataAsset;
#pragma endregion

#pragma region References
private:
	UPROPERTY()
	TObjectPtr<AEnemyBase> Enemy;
#pragma endregion

#pragma region RuntimeData
private:
	TMap<EEnemyAudioEvent, float> NextAllowedPlayTimes;
	TMap<EEnemyAudioEvent, ESoundID> LastPlayedSounds;
#pragma endregion
};