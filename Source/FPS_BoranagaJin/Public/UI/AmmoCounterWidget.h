

#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "AmmoCounterWidget.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API UAmmoCounterWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual EUIType GetUIType() const override { return EUIType::AmmoCounter; }
public:
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* AmmoCount;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* TotalAmmo;

	void UpdateAmmoCount(int32 NewAmmoCount);
	void UpdateTotalAmmo(int32 NewTotalAmmo);
};