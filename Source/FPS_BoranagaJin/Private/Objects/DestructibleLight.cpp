#include "Objects/DestructibleLight.h"

#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SceneComponent.h"

ADestructibleLight::ADestructibleLight()
{
	PrimaryActorTick.bCanEverTick = false;

	LightRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LightRoot"));
	LightRoot->SetupAttachment(SceneRoot);

	PointLightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLightComponent"));
	PointLightComponent->SetupAttachment(LightRoot);
	PointLightComponent->SetMobility(EComponentMobility::Movable);

	SpotLightComponent = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLightComponent"));
	SpotLightComponent->SetupAttachment(LightRoot);
	SpotLightComponent->SetMobility(EComponentMobility::Movable);

	RectLightComponent = CreateDefaultSubobject<URectLightComponent>(TEXT("RectLightComponent"));
	RectLightComponent->SetupAttachment(LightRoot);
	RectLightComponent->SetMobility(EComponentMobility::Movable);
}

float ADestructibleLight::ReceiveDamage(const FDamageParams& DamageData)
{
	const float AppliedDamage = Super::ReceiveDamage(DamageData);

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

	InitializeEmissiveMaterials();

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

void ADestructibleLight::BreakObject(const FVector& HitLocation, const FVector& HitDirection)
{
	if (bDestroyed)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(MalfunctionFlickerTimerHandle);

	bIsMalfunctioning = false;
	bFlickerOutputEnabled = false;

	//SetLightEnabled(false);
	SetLightOutputEnabled(false);

	Super::BreakObject(HitLocation, HitDirection);
}

void ADestructibleLight::InitializeEmissiveMaterials()
{
	if (!bUseEmissiveMaterial)
	{
		return;
	}

	UMeshComponent* TargetMesh = GetEmissiveTargetMesh();

	if (!TargetMesh)
	{
		return;
	}

	for (FLightEmissiveMaterial& MaterialData : EmissiveMaterials)
	{
		if (MaterialData.MaterialIndex < 0)
		{
			continue;
		}

		if (MaterialData.MaterialIndex >= TargetMesh->GetNumMaterials())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT(
					"Invalid emissive material index. "
					"Actor=%s Index=%d"
				),
				*GetName(),
				MaterialData.MaterialIndex
			);

			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = TargetMesh->CreateAndSetMaterialInstanceDynamic(MaterialData.MaterialIndex);

		if (!DynamicMaterial)
		{
			continue;
		}

		MaterialData.DynamicMaterial = DynamicMaterial;
	}

	UpdateEmissiveMaterials();
}

void ADestructibleLight::UpdateEmissiveMaterials()
{
	const bool bEnableOutput = CanProduceLight();

	SetEmissiveOutputEnabled(bEnableOutput);
}

UMeshComponent* ADestructibleLight::GetEmissiveTargetMesh() const
{
	return IntactMesh;
}

bool ADestructibleLight::CanProduceLight() const
{
	return bLightEnabled && bHasExternalPower && !bDestroyed;
}

void ADestructibleLight::SetLightOutputEnabled(bool bEnabled)
{
	SetLightComponentOutputEnabled(bEnabled);
	SetEmissiveOutputEnabled(bEnabled);
}

void ADestructibleLight::SetLightComponentOutputEnabled(bool bEnabled)
{
	if (PointLightComponent)
	{
		const bool bEnable = bEnabled && LightType == EDestructibleLightType::Point;
		PointLightComponent->SetVisibility(bEnable, true);
	}

	if (SpotLightComponent)
	{
		const bool bEnable = bEnabled && LightType == EDestructibleLightType::Spot;
		SpotLightComponent->SetVisibility(bEnable, true);
	}

	if (RectLightComponent)
	{
		const bool bEnable = bEnabled && LightType == EDestructibleLightType::Rect;
		RectLightComponent->SetVisibility(bEnable, true);
	}
}

void ADestructibleLight::SetEmissiveOutputEnabled(bool bEnabled)
{
	UE_LOG(LogTemp, Error, TEXT("void ADestructibleLight::SetEmissiveOutputEnabled(bool bEnabled)"));


	if (!bUseEmissiveMaterial)
	{

		UE_LOG(LogTemp, Error, TEXT("bUseEmissiveMaterial == false!!!"));
		return;
	}

	for (FLightEmissiveMaterial& MaterialData : EmissiveMaterials)
	{
		if (!MaterialData.DynamicMaterial)
		{
			continue;
		}

		const float TargetIntensity = bEnabled ? MaterialData.EmissiveIntensity : 0.f;
		MaterialData.DynamicMaterial->SetScalarParameterValue(EmissiveIntensityParameterName, TargetIntensity);
	}
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

	////-------------------
	//if (!bLightEnabled)
	//{
	//	GetWorldTimerManager().ClearTimer(
	//		MalfunctionFlickerTimerHandle
	//	);
	//}
	//else if (
	//	bIsMalfunctioning &&
	//	bHasExternalPower
	//	)
	//{
	//	ScheduleNextFlicker();
	//}
	////-------------------



	UpdateActiveLightType();
}

void ADestructibleLight::SetLightIntensity(float NewIntensity)
{
	LightIntensity = FMath::Max(0.f, NewIntensity);
	ApplyCommonLightSettings();
}

void ADestructibleLight::SetLightColor(const FLinearColor& NewColor)
{
	LightColor = NewColor;
	ApplyCommonLightSettings();
}

bool ADestructibleLight::IsLightOn() const
{
	//return bLightEnabled && !bDestroyed;
	return CanProduceLight();
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

	case EDestructibleLightType::None:
	default:
		return nullptr;
	}
}

void ADestructibleLight::SetExternalPowerAvailable(bool bAvailable)
{
	if (bHasExternalPower == bAvailable)
	{
		return;
	}

	bHasExternalPower = bAvailable;

	if (!bHasExternalPower)
	{
		GetWorldTimerManager().ClearTimer(MalfunctionFlickerTimerHandle);
		SetLightOutputEnabled(false);
		return;
	}

	UpdateActiveLightType();

	if (bIsMalfunctioning && bLightEnabled && !bDestroyed)
	{
		ScheduleNextFlicker();
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

void ADestructibleLight::UpdateActiveLightType() //TODO: 수정
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

	//----------
	//SetLightOutputEnabled(CanProduceLight());
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

		LightComponent->SetIntensity(LightIntensity);
		LightComponent->SetLightColor(LightColor);
		LightComponent->SetCastShadows(bCastShadows);
	}

	if (PointLightComponent)
	{
		PointLightComponent->SetAttenuationRadius(AttenuationRadius);
	}

	if (SpotLightComponent)
	{
		SpotLightComponent->SetAttenuationRadius(AttenuationRadius);
	}

	if (RectLightComponent)
	{
		RectLightComponent->SetAttenuationRadius(AttenuationRadius);
	}
}

void ADestructibleLight::ApplyPointLightSettings()
{
	if (!PointLightComponent)
	{
		return;
	}

	PointLightComponent->SetSourceRadius(PointSourceRadius);
	PointLightComponent->SetSoftSourceRadius(PointSoftSourceRadius);
}

void ADestructibleLight::ApplySpotLightSettings()
{
	if (!SpotLightComponent)
	{
		return;
	}

	const float ValidOuterConeAngle = FMath::Max(InnerConeAngle, OuterConeAngle);

	SpotLightComponent->SetInnerConeAngle(InnerConeAngle);
	SpotLightComponent->SetOuterConeAngle(ValidOuterConeAngle);
}

void ADestructibleLight::ApplyRectLightSettings()
{
	if (!RectLightComponent)
	{
		return;
	}

	RectLightComponent->SetSourceWidth(RectSourceWidth);
	RectLightComponent->SetSourceHeight(RectSourceHeight);
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

	const float CurrentHealthRatio = CurrentHealth / MaxHealth;

	const bool bShouldMalfunction = CurrentHealthRatio <= MalfunctionHealthRatio;

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
	UE_LOG(LogTemp, Error, TEXT("void ADestructibleLight::StartMalfunctionFlicker()"));

	if (bIsMalfunctioning || bDestroyed)
	{
		return;
	}

	bIsMalfunctioning = true;

	if (bHasExternalPower && bLightEnabled)
	{
		ScheduleNextFlicker();
	}
}

void ADestructibleLight::StopMalfunctionFlicker()
{
	GetWorldTimerManager().ClearTimer(MalfunctionFlickerTimerHandle);

	bIsMalfunctioning = false;
	bFlickerOutputEnabled = true;

	if (!bDestroyed)
	{
		UpdateActiveLightType();
	}
}

void ADestructibleLight::UpdateMalfunctionFlicker()
{
	if (!bIsMalfunctioning || bDestroyed || !bHasExternalPower || !bLightEnabled)
	{
		SetLightOutputEnabled(false);
		return;
	}

	//ULightComponent* ActiveLight = GetActiveLightComponent();

	//if (!ActiveLight)
	//{
	//	return;
	//}

	//const bool bNewVisible = !ActiveLight->IsVisible();

	//ActiveLight->SetVisibility(bNewVisible, true);

	//float NextDelay = 0.f;

	//if (bNewVisible)
	//{
	//	NextDelay = FMath::FRandRange(
	//		MinFlickerOnDuration,
	//		FMath::Max(
	//			MinFlickerOnDuration,
	//			MaxFlickerOnDuration
	//		)
	//	);
	//}
	//else
	//{
	//	NextDelay = FMath::FRandRange(
	//		MinFlickerOffDuration,
	//		FMath::Max(
	//			MinFlickerOffDuration,
	//			MaxFlickerOffDuration
	//		)
	//	);
	//}

	//GetWorldTimerManager().SetTimer(
	//	MalfunctionFlickerTimerHandle,
	//	this,
	//	&ADestructibleLight::UpdateMalfunctionFlicker,
	//	FMath::Max(0.01f, NextDelay),
	//	false
	//);



	bFlickerOutputEnabled = !bFlickerOutputEnabled;

	SetLightOutputEnabled(bFlickerOutputEnabled);
	ScheduleNextFlicker();
}

void ADestructibleLight::ScheduleNextFlicker()
{
	if (!bIsMalfunctioning || bDestroyed || !bHasExternalPower || !bLightEnabled)
	{
		return;
	}

	const float ValidMinInterval = FMath::Max(0.01f, MinFlickerInterval);
	const float ValidMaxInterval = FMath::Max(ValidMinInterval, MaxFlickerInterval);

	const float NextFlickerDelay = FMath::FRandRange(ValidMinInterval, ValidMaxInterval);

	GetWorldTimerManager().SetTimer(
		MalfunctionFlickerTimerHandle,
		this,
		&ADestructibleLight::UpdateMalfunctionFlicker,
		NextFlickerDelay,
		false
	);
}