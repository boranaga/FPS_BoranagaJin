#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "HealthWidget.generated.h"


class UProgressBar;

UCLASS()
class FPS_BORANAGAJIN_API UHealthWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual EUIType GetUIType() const { return EUIType::UIType_Stamina; }
public:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar = nullptr;
protected:
	float MaxHealth = 100.f;
	float CurrHealth = 100.f;
public:
	void InitHealthBar(float maxhealth = 100.f);
	void SetHealthBarPercent(float const maxhealth, float const currhealth);
};
