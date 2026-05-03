

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AmmoCounterWidget.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API UAmmoCounterWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* AmmoCount;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* TotalAmmo;

	void UpdateAmmoCount(int32 NewAmmoCount);
	void UpdateTotalAmmo(int32 NewTotalAmmo);
};