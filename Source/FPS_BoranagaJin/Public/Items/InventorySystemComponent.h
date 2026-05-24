

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemName.h"
#include "InventorySystemComponent.generated.h"

class ACharacterPlayer;
class AItem;
class AItemPickUP;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UInventorySystemComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UInventorySystemComponent();
protected:
	virtual void BeginPlay() override;
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	void InitPlayerReference();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "WeaponSystem|PlayerReference", Meta = (AllowPrivateAccess = "true"))
	ACharacterPlayer* PlayerOwner;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "WeaponSystem|PlayerReference", Meta = (AllowPrivateAccess = "true"))
	class APlayerController* PlayerController;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* InventorySystemMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LeftMouseButtonAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RightMouseButtonAction;

#pragma region SearchItem
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponSystem")
	float SearchItemRadius = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponSystem")
	float SearchItemViewportRatio_Width = 0.7;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponSystem")
	float SearchItemViewportRatio_Height = 0.7;
	//UPROPERTY(VisibleAnywhere)
	//AItemPickUP* OverlappedItem = nullptr;
protected:
	bool SearchItem();
	//TTuple<FVector2D, bool> GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const;
	//bool IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width = 0.0f, float ScreenRatio_Height = 0.0f) const;
#pragma endregion
#pragma region Inventory
protected:
	//TMap<EItemName, AItem*> ItemInventoryMap;
	//TArray<EItemName> ItemInventoryKeys;


#pragma endregion
};


//TODO: Inventory 자체는 Map으로 구성해서, Item Name으로 검색하도록 구현하기
//근데, array 처럼 마우스 휠로 순회 가능하게도 해야하는데, 이 부분에 대해서 고려해야함

//TODO: InventorySystem 과 WeaponSystem의 Search object 기능을 통합해야함.