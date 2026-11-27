#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "ItemName.h"
#include "InventorySlot.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FInventorySlot
{
    GENERATED_BODY()

public:
    // One runtime actor represents the entire homogeneous stack.
    UPROPERTY(Transient)
    TObjectPtr<AItem> Item = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EItemName ItemName = EItemName::ItemName_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FName ItemID = NAME_None;

    // Mirror of AItem's base stack quantity. Never edit either independently.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ItemNum = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
    int32 MaxStackCount = 99;

    void ClearSlot(); // Detach only; caller owns pool/drop policy. Preserve capacity.
    bool IsEmpty() const;
    int32 GetStackItemNum() const;
    int32 Num() const; // UI quantity (throwable launcher exposes its ammo via GetItemNum).
    int32 Count() const;
    bool IsSameItem(const EItemName& InItemName) const;
    static bool CanRepresentStack(const AItem* InItem);
    bool CanStack(const EItemName& InItemName, int32 AddCount = 1) const;
    bool CanStackWith(const AItem* InItem, int32 AddCount = 1) const;
    bool AddItem(AItem* InItem, int32 AddCount = 1);
    bool AddStack(int32 AddCount = 1);
    bool RemoveItems(int32 RemoveCount = 1);
    void SynchronizeItemNum();
    // Index 0 retained for source compatibility. There are no other actor indices.
    AItem* GetItem(int32 ItemIdx = 0) const;
};
