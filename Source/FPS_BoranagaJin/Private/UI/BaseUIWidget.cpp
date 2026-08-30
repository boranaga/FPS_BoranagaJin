
#include "UI/BaseUIWidget.h"

#include "Engine/GameInstance.h"
#include "SoundSystem/GameAudioSubsystem.h"

void UBaseUIWidget::SetUIType(EUIType NewUIType)
{
	UIType = NewUIType;
}

void UBaseUIWidget::PlayUISound(ESoundID SoundID) const
{
	if (SoundID == ESoundID::None)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();

	if (!IsValid(GameInstance))
	{
		return;
	}

	UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>();

	if (!IsValid(AudioSubsystem))
	{
		return;
	}

	AudioSubsystem->PlaySound2D(SoundID);
}