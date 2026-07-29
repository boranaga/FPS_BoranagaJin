#include "Objects/DestructibleLight.h"

#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SceneComponent.h"

ADestructibleLight::ADestructibleLight()
{
	PrimaryActorTick.bCanEverTick = false;

	LightRoot = CreateDefaultSubobject<USceneComponent>(
		TEXT("LightRoot")
	);
	LightRoot->SetupAttachment(SceneRoot);

	PointLightComponent =
		CreateDefaultSubobject<UPointLightComponent>(
			TEXT("PointLightComponent")
		);
	PointLightComponent->SetupAttachment(LightRoot);
	PointLightComponent->SetMobility(EComponentMobility::Movable);

	SpotLightComponent =
		CreateDefaultSubobject<USpotLightComponent>(
			TEXT("SpotLightComponent")
		);
	SpotLightComponent->SetupAttachment(LightRoot);
	SpotLightComponent->SetMobility(EComponentMobility::Movable);

	RectLightComponent =
		CreateDefaultSubobject<URectLightComponent>(
			TEXT("RectLightComponent")
		);
	RectLightComponent->SetupAttachment(LightRoot);
	RectLightComponent->SetMobility(EComponentMobility::Movable);
}

float ADestructibleLight::ReceiveDamage(const FDamageParams& DamageData)
{
	const float AppliedDamage =
		Super::ReceiveDamage(DamageData);

	/*
	 * 부모 ReceiveDamage에서 파괴되었을 수도 있으므로
	 * 파괴되지 않은 경우에만 고장 상태를 검사합니다.
	 */
	if (!bDestroyed)
	{
		EvaluateMalfunctionState();
	}

	return AppliedDamage;
}

void ADestructibleLight::BeginPlay()
{
	Super::BeginPlay();

	bLightEnabled = bStartsEnabled;

	UpdateLightSettings();
	EvaluateMalfunctionState();
}

void ADestructibleLight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	/*
	 * 에디터에서 설정을 변경했을 때 즉시 조명에 반영합니다.
	 *
	 * 게임 실행 전에는 bStartsEnabled를 기준으로 미리보기합니다.
	 */
	if (!HasActorBegunPlay())
	{
		bLightEnabled = bStartsEnabled;
	}

	UpdateLightSettings();
}

void ADestructibleLight::BreakObject(
	const FVector& HitLocation,
	const FVector& HitDirection
)
{
	if (bDestroyed)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(
		MalfunctionFlickerTimerHandle
	);

	bIsMalfunctioning = false;

	SetLightEnabled(false);

	Super::BreakObject(
		HitLocation,
		HitDirection
	);
}

void ADestructibleLight::TurnOnLight()
{
	SetLightEnabled(true);
}

void ADestructibleLight::TurnOffLight()
{
	SetLightEnabled(false);
}

void ADestructibleLight::ToggleLight()
{
	SetLightEnabled(!bLightEnabled);
}

void ADestructibleLight::SetLightEnabled(bool bEnabled)
{
	/*
	 * 파괴된 조명은 다시 켜지지 않도록 합니다.
	 */
	if (bDestroyed && bEnabled)
	{
		return;
	}

	bLightEnabled = bEnabled;

	UpdateActiveLightType();
}

void ADestructibleLight::SetLightIntensity(
	float NewIntensity
)
{
	LightIntensity = FMath::Max(0.f, NewIntensity);

	ApplyCommonLightSettings();
}

void ADestructibleLight::SetLightColor(
	const FLinearColor& NewColor
)
{
	LightColor = NewColor;

	ApplyCommonLightSettings();
}

bool ADestructibleLight::IsLightOn() const
{
	return bLightEnabled && !bDestroyed;
}

ULightComponent*
ADestructibleLight::GetActiveLightComponent() const
{
	switch (LightType)
	{
	case EDestructibleLightType::Point:
		return PointLightComponent;

	case EDestructibleLightType::Spot:
		return SpotLightComponent;

	case EDestructibleLightType::Rect:
		return RectLightComponent;

	default:
		return nullptr;
	}
}

void ADestructibleLight::UpdateLightSettings()
{
	ApplyCommonLightSettings();
	ApplyPointLightSettings();
	ApplySpotLightSettings();
	ApplyRectLightSettings();

	UpdateActiveLightType();
}

void ADestructibleLight::UpdateActiveLightType()
{
	const bool bCanActivateLight =
		bLightEnabled && !bDestroyed;

	if (PointLightComponent)
	{
		const bool bEnablePointLight =
			bCanActivateLight &&
			LightType == EDestructibleLightType::Point;

		PointLightComponent->SetVisibility(
			bEnablePointLight,
			true
		);
	}

	if (SpotLightComponent)
	{
		const bool bEnableSpotLight =
			bCanActivateLight &&
			LightType == EDestructibleLightType::Spot;

		SpotLightComponent->SetVisibility(
			bEnableSpotLight,
			true
		);
	}

	if (RectLightComponent)
	{
		const bool bEnableRectLight =
			bCanActivateLight &&
			LightType == EDestructibleLightType::Rect;

		RectLightComponent->SetVisibility(
			bEnableRectLight,
			true
		);
	}
}

void ADestructibleLight::ApplyCommonLightSettings()
{
	ULightComponent* LightComponents[] =
	{
		PointLightComponent,
		SpotLightComponent,
		RectLightComponent
	};

	for (ULightComponent* LightComponent : LightComponents)
	{
		if (!LightComponent)
		{
			continue;
		}

		LightComponent->SetIntensity(
			LightIntensity
		);

		LightComponent->SetLightColor(
			LightColor
		);

		LightComponent->SetCastShadows(
			bCastShadows
		);
	}

	if (PointLightComponent)
	{
		PointLightComponent->SetAttenuationRadius(
			AttenuationRadius
		);
	}

	if (SpotLightComponent)
	{
		SpotLightComponent->SetAttenuationRadius(
			AttenuationRadius
		);
	}

	if (RectLightComponent)
	{
		RectLightComponent->SetAttenuationRadius(
			AttenuationRadius
		);
	}
}

void ADestructibleLight::ApplyPointLightSettings()
{
	if (!PointLightComponent)
	{
		return;
	}

	PointLightComponent->SetSourceRadius(
		PointSourceRadius
	);

	PointLightComponent->SetSoftSourceRadius(
		PointSoftSourceRadius
	);
}

void ADestructibleLight::ApplySpotLightSettings()
{
	if (!SpotLightComponent)
	{
		return;
	}

	const float ValidOuterConeAngle =
		FMath::Max(InnerConeAngle, OuterConeAngle);

	SpotLightComponent->SetInnerConeAngle(
		InnerConeAngle
	);

	SpotLightComponent->SetOuterConeAngle(
		ValidOuterConeAngle
	);
}

void ADestructibleLight::ApplyRectLightSettings()
{
	if (!RectLightComponent)
	{
		return;
	}

	RectLightComponent->SetSourceWidth(
		RectSourceWidth
	);

	RectLightComponent->SetSourceHeight(
		RectSourceHeight
	);
}

void ADestructibleLight::EvaluateMalfunctionState()
{
	if (!bEnableMalfunctionFlicker)
	{
		StopMalfunctionFlicker();
		return;
	}

	if (bDestroyed || MaxHealth <= 0.f)
	{
		StopMalfunctionFlicker();
		return;
	}

	const float CurrentHealthRatio =
		CurrentHealth / MaxHealth;

	const bool bShouldMalfunction =
		CurrentHealthRatio <= MalfunctionHealthRatio;

	if (bShouldMalfunction && !bIsMalfunctioning)
	{
		StartMalfunctionFlicker();
	}
	else if (!bShouldMalfunction && bIsMalfunctioning)
	{
		/*
		 * 나중에 수리 기능으로 체력이 회복되는 경우를 고려합니다.
		 */
		StopMalfunctionFlicker();

		if (bStartsEnabled)
		{
			SetLightEnabled(true);
		}
	}
}

void ADestructibleLight::StartMalfunctionFlicker()
{
	if (bIsMalfunctioning || bDestroyed)
	{
		return;
	}

	bIsMalfunctioning = true;

	ScheduleNextFlicker();
}

void ADestructibleLight::StopMalfunctionFlicker()
{
	GetWorldTimerManager().ClearTimer(
		MalfunctionFlickerTimerHandle
	);

	bIsMalfunctioning = false;

	/*
	 * 파괴되지 않았다면 원래 논리 상태에 맞게 조명을 복원합니다.
	 */
	if (!bDestroyed)
	{
		UpdateActiveLightType();
	}
}

void ADestructibleLight::UpdateMalfunctionFlicker()
{
	//if (!bIsMalfunctioning || bDestroyed)
	//{
	//	return;
	//}

	//const float RandomValue = FMath::FRand();

	//const bool bShouldTurnOff =
	//	RandomValue <= FlickerOffProbability;

	///*
	// * SetLightEnabled()는 논리적인 사용자 설정까지 변경할 수 있으므로
	// * 고장 효과에서는 조명 컴포넌트의 표시 상태만 변경하는 것이
	// * 더 안전할 수 있습니다.
	// */
	//ULightComponent* ActiveLight =
	//	GetActiveLightComponent();

	//if (ActiveLight)
	//{
	//	ActiveLight->SetVisibility(
	//		!bShouldTurnOff,
	//		true
	//	);
	//}

	//ScheduleNextFlicker();

	//---------------------------------------------

	if (!bIsMalfunctioning || bDestroyed)
	{
		return;
	}

	ULightComponent* ActiveLight =
		GetActiveLightComponent();

	if (!ActiveLight)
	{
		return;
	}

	const bool bNewVisible =
		!ActiveLight->IsVisible();

	ActiveLight->SetVisibility(
		bNewVisible,
		true
	);

	float NextDelay = 0.f;

	if (bNewVisible)
	{
		NextDelay = FMath::FRandRange(
			MinFlickerOnDuration,
			FMath::Max(
				MinFlickerOnDuration,
				MaxFlickerOnDuration
			)
		);
	}
	else
	{
		NextDelay = FMath::FRandRange(
			MinFlickerOffDuration,
			FMath::Max(
				MinFlickerOffDuration,
				MaxFlickerOffDuration
			)
		);
	}

	GetWorldTimerManager().SetTimer(
		MalfunctionFlickerTimerHandle,
		this,
		&ADestructibleLight::UpdateMalfunctionFlicker,
		FMath::Max(0.01f, NextDelay),
		false
	);

}

void ADestructibleLight::ScheduleNextFlicker()
{
	if (!bIsMalfunctioning || bDestroyed)
	{
		return;
	}

	const float ValidMinInterval =
		FMath::Max(0.01f, MinFlickerInterval);

	const float ValidMaxInterval =
		FMath::Max(
			ValidMinInterval,
			MaxFlickerInterval
		);

	const float NextFlickerDelay =
		FMath::FRandRange(
			ValidMinInterval,
			ValidMaxInterval
		);

	GetWorldTimerManager().SetTimer(
		MalfunctionFlickerTimerHandle,
		this,
		&ADestructibleLight::UpdateMalfunctionFlicker,
		NextFlickerDelay,
		false
	);
}