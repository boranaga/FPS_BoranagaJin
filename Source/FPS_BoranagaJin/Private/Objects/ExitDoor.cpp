#include "Objects/ExitDoor.h"

#include "Characters/Player/CharacterPlayer.h"
#include "Items/InventoryName.h"
#include "Items/InventorySlot.h"
#include "Items/InventorySystemComponent.h"
#include "Items/Item.h"
#include "Objects/DestructibleDoor.h"
#include "Objects/MapEscapeTrigger.h"
#include "UI/ExitDoorWidget.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	// 1번 탈출구부터 3개씩 차례로 밀려서 사용한다. (N번 탈출구 = N-1번째부터 3개)
	// 아이템 BP의 ItemID 와 같은 이름이어야 한다.
	const TCHAR* const GExitItemOrder[] =
	{
		TEXT("KeyCard"),
		TEXT("Battery"),
		TEXT("Fuse"),
		TEXT("Chip")
	};

	// 인벤토리 클래스는 수정하지 않고, 공개된 슬롯 배열만 읽고 쓴다.
	int32 CountItems(const ACharacterPlayer* Player, FName ItemID)
	{
		const UInventorySystemComponent* Inventory = IsValid(Player) ? Player->GetInventorySystemComponent() : nullptr;
		const TArray<FInventorySlot>* Slots = Inventory ? Inventory->GetInventoryConst(EInventoryName::Item) : nullptr;
		if (!Slots || ItemID.IsNone()) { return 0; }

		int32 Count = 0;
		for (const FInventorySlot& Slot : *Slots)
		{
			if (!Slot.IsEmpty() && Slot.ItemID == ItemID)
			{
				Count += Slot.Num();
			}
		}
		return Count;
	}

	bool ConsumeOneItem(ACharacterPlayer* Player, FName ItemID)
	{
		UInventorySystemComponent* Inventory = IsValid(Player) ? Player->GetInventorySystemComponent() : nullptr;
		TArray<FInventorySlot>* Slots = Inventory ? Inventory->GetInventory(EInventoryName::Item) : nullptr;
		if (!Slots || ItemID.IsNone()) { return false; }

		for (FInventorySlot& Slot : *Slots)
		{
			if (Slot.IsEmpty() || Slot.ItemID != ItemID) { continue; }

			// INDEX_NONE = 마지막 아이템. 슬롯이 비면 FInventorySlot이 스스로 초기화한다.
			AItem* RemovedItem = Slot.RemoveItem(INDEX_NONE);
			if (IsValid(RemovedItem))
			{
				RemovedItem->DeactivateItem();
			}

			// 인벤토리 UI 갱신
			Player->OnInventoryUpdatedDelegate.Broadcast(*Slots);
			return true;
		}
		return false;
	}

	const int32 GExitRequiredItemCount = 3;

	// 다른 HUD보다 위, 일시정지/게임오버 메뉴보다는 아래
	const int32 GExitWidgetZOrder = 5;

	template<typename TActor>
	TActor* FindNearestActor(const AActor* Origin, float Radius)
	{
		TActor* Nearest = nullptr;
		float NearestDistSq = FMath::Square(Radius);

		for (TActorIterator<TActor> It(Origin->GetWorld()); It; ++It)
		{
			const float DistSq = FVector::DistSquared(It->GetActorLocation(), Origin->GetActorLocation());
			if (DistSq <= NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = *It;
			}
		}
		return Nearest;
	}
}

AExitDoor::AExitDoor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(SceneRoot);

	DoorVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorVolume"));
	DoorVolume->SetupAttachment(SceneRoot);
	DoorVolume->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	DoorVolume->SetBoxExtent(FVector(25.f, 100.f, 110.f));
	// 잠겨 있는 동안 상호작용 트레이스(Visibility)만 막는다. 이동이나 총알은 막지 않는다.
	DoorVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DoorVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	DoorVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	DoorVolume->SetGenerateOverlapEvents(false);

	WidgetClass = UExitDoorWidget::StaticClass();
	AutoLinkDoorClasses.Add(ADestructibleDoor::StaticClass());

	ApplyExitPreset();
}

void AExitDoor::BeginPlay()
{
	Super::BeginPlay();

	if (RequiredSlots.IsEmpty())
	{
		ApplyExitPreset();
	}

	for (FExitRequirementSlot& Slot : RequiredSlots)
	{
		Slot.bInserted = false;
	}
	bUnlocked = false;

	if (bAutoLinkNearby)
	{
		if (!LinkedDoor) { LinkedDoor = FindNearestDoorActor(); }
		if (!LinkedEscapeTrigger) { LinkedEscapeTrigger = FindNearestActor<AMapEscapeTrigger>(this, AutoLinkRadius); }
	}

	SetLinkedActorsLocked(true);

	UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: BeginPlay (Exit %d, LinkedDoor=%s, LinkedEscapeTrigger=%s, ShowDistance=%.0f)"),
		*GetName(), ExitNumber, *GetNameSafe(LinkedDoor), *GetNameSafe(LinkedEscapeTrigger), ShowDistance);

	GetWorldTimerManager().SetTimer(ProximityTimerHandle, this, &AExitDoor::UpdateProximity, ProximityCheckInterval, true);
}

void AExitDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ProximityTimerHandle);

	SetWidgetShown(false);
	if (ExitWidget)
	{
		ExitWidget->RemoveFromParent();
		ExitWidget = nullptr;
	}
	NearbyPlayer = nullptr;

	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AExitDoor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AExitDoor, ExitNumber))
	{
		ApplyExitPreset();
	}
}
#endif

void AExitDoor::ApplyExitPreset()
{
	const int32 ItemKindCount = UE_ARRAY_COUNT(GExitItemOrder);
	const int32 StartIndex = FMath::Clamp(ExitNumber, 1, ItemKindCount) - 1;

	RequiredSlots.Reset();
	for (int32 Offset = 0; Offset < GExitRequiredItemCount; ++Offset)
	{
		FExitRequirementSlot Slot;
		Slot.ItemID = FName(GExitItemOrder[(StartIndex + Offset) % ItemKindCount]);
		Slot.DisplayName = GetDefaultItemDisplayName(Slot.ItemID);
		RequiredSlots.Add(Slot);
	}
}

int32 AExitDoor::GetInsertedCount() const
{
	int32 Count = 0;
	for (const FExitRequirementSlot& Slot : RequiredSlots)
	{
		if (Slot.bInserted) { ++Count; }
	}
	return Count;
}

FText AExitDoor::GetDefaultItemDisplayName(FName ItemID)
{
	if (ItemID == FName(TEXT("KeyCard"))) { return FText::FromString(TEXT("키카드")); }
	if (ItemID == FName(TEXT("Battery"))) { return FText::FromString(TEXT("배터리")); }
	if (ItemID == FName(TEXT("Fuse"))) { return FText::FromString(TEXT("퓨즈")); }
	if (ItemID == FName(TEXT("Chip"))) { return FText::FromString(TEXT("칩")); }
	return FText::FromName(ItemID);
}

void AExitDoor::SetLinkedActorsLocked(bool bLocked)
{
	// 잠긴 동안에는 DoorVolume이 문 대신 상호작용 트레이스를 받는다.
	DoorVolume->SetCollisionEnabled(bLocked ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	if (LinkedEscapeTrigger)
	{
		// 트리거의 충돌을 꺼서 겹침 이벤트가 생기지 않게 한다.
		LinkedEscapeTrigger->SetActorEnableCollision(!bLocked);
	}

	if (ADestructibleDoor* DestructibleDoor = Cast<ADestructibleDoor>(LinkedDoor))
	{
		if (bLocked)
		{
			DestructibleDoor->CloseDoor();
		}
		else
		{
			DestructibleDoor->OpenDoor();
		}
	}
	else if (LinkedDoor)
	{
		SetTriggeredDoorLocked(LinkedDoor, bLocked);
	}
}

void AExitDoor::SetTriggeredDoorLocked(AActor* DoorActor, bool bLocked)
{
	if (bLocked)
	{
		// 문 블루프린트의 감지 박스를 꺼서, 플레이어가 다가가도 스스로 열리지 않게 한다.
		LockedTriggers.Reset();
		LockedTriggerCollision.Reset();

		TInlineComponentArray<UShapeComponent*> Shapes(DoorActor);
		for (UShapeComponent* Shape : Shapes)
		{
			if (Shape->GetCollisionEnabled() == ECollisionEnabled::NoCollision) { continue; }

			LockedTriggers.Add(Shape);
			LockedTriggerCollision.Add(Shape->GetCollisionEnabled());
			Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: locked door %s (%d trigger(s) disabled)"), *GetName(), *DoorActor->GetName(), LockedTriggers.Num());
		return;
	}

	// 문 블루프린트의 타임라인(열리는 애니메이션)을 직접 재생한다.
	TInlineComponentArray<UTimelineComponent*> Timelines(DoorActor);
	for (UTimelineComponent* Timeline : Timelines)
	{
		Timeline->Play();
	}

	// 계속 열어 둘 거라면 감지 박스는 꺼진 채로 둔다. (켜면 멀어질 때 다시 닫힌다)
	// 타임라인을 못 찾았을 때는 원래 동작이라도 하도록 되살린다.
	if (!bKeepDoorOpenAfterUnlock || Timelines.IsEmpty())
	{
		for (int32 Index = 0; Index < LockedTriggers.Num(); ++Index)
		{
			if (UShapeComponent* Shape = LockedTriggers[Index].Get())
			{
				Shape->SetCollisionEnabled(LockedTriggerCollision[Index]);
			}
		}
	}
	LockedTriggers.Reset();
	LockedTriggerCollision.Reset();

	UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: opened door %s (%d timeline(s) played)"), *GetName(), *DoorActor->GetName(), Timelines.Num());
}

AActor* AExitDoor::FindNearestDoorActor() const
{
	AActor* Nearest = nullptr;
	float NearestDistSq = FMath::Square(AutoLinkRadius);

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (Candidate == this) { continue; }

		const bool bIsDoor = AutoLinkDoorClasses.ContainsByPredicate([Candidate](const TSubclassOf<AActor>& DoorClass)
		{
			return DoorClass && Candidate->IsA(DoorClass);
		});
		if (!bIsDoor) { continue; }

		const float DistSq = FVector::DistSquared(Candidate->GetActorLocation(), GetActorLocation());
		if (DistSq <= NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Candidate;
		}
	}
	return Nearest;
}

float AExitDoor::GetDistanceToDoor(const FVector& WorldLocation) const
{
	// DoorVolume(회전된 박스) 표면까지의 거리. 박스 안에 있으면 0.
	const FTransform& BoxTransform = DoorVolume->GetComponentTransform();
	const FVector Extent = DoorVolume->GetUnscaledBoxExtent();

	const FVector Local = BoxTransform.InverseTransformPosition(WorldLocation);
	const FVector ClosestLocal(
		FMath::Clamp(Local.X, -Extent.X, Extent.X),
		FMath::Clamp(Local.Y, -Extent.Y, Extent.Y),
		FMath::Clamp(Local.Z, -Extent.Z, Extent.Z));

	return FVector::Dist(BoxTransform.TransformPosition(ClosestLocal), WorldLocation);
}

void AExitDoor::UpdateProximity()
{
	// DoorVolume이 덮지 못한 부분으로 문을 열었을 때를 대비해, 잠긴 동안에는 다시 닫는다.
	if (!bUnlocked)
	{
		ADestructibleDoor* DestructibleDoor = Cast<ADestructibleDoor>(LinkedDoor);
		if (DestructibleDoor && DestructibleDoor->IsDoorOpen())
		{
			DestructibleDoor->CloseDoor();
		}
	}

	ACharacterPlayer* Player = Cast<ACharacterPlayer>(UGameplayStatics::GetPlayerPawn(this, 0));

	bool bInRange = false;
	if (IsValid(Player) && Player->IsLocallyControlled())
	{
		// 캡슐 중심이 아니라 캡슐 표면 기준으로 잰다.
		const float Distance = GetDistanceToDoor(Player->GetActorLocation()) - Player->GetSimpleCollisionRadius();
		bInRange = Distance <= ShowDistance;
	}

	NearbyPlayer = bInRange ? Player : nullptr;

	if (bInRange != bWidgetShown)
	{
		SetWidgetShown(bInRange);
	}
	else if (bWidgetShown)
	{
		// 가까이 있는 동안 인벤토리 변화를 UI에 반영한다.
		RefreshWidget();
	}
}

void AExitDoor::SetWidgetShown(bool bShown)
{
	if (bShown && !ExitWidget && WidgetClass && IsValid(NearbyPlayer))
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(NearbyPlayer->GetController()))
		{
			ExitWidget = CreateWidget<UExitDoorWidget>(PlayerController, WidgetClass);
			if (ExitWidget)
			{
				ExitWidget->OnSlotClicked.AddUObject(this, &AExitDoor::HandleSlotClicked);
				ExitWidget->AddToViewport(GExitWidgetZOrder);
			}
		}
	}

	bWidgetShown = bShown;

	if (ExitWidget)
	{
		// 패널 바깥은 클릭이 통과해야 하므로 SelfHitTestInvisible.
		ExitWidget->SetVisibility(bShown ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: UI %s (widget=%s)"), *GetName(), bShown ? TEXT("shown") : TEXT("hidden"), *GetNameSafe(ExitWidget));

	if (bShown)
	{
		RefreshWidget();
	}
	else
	{
		UpdateCursorMode(false);
	}
}

void AExitDoor::RefreshWidget()
{
	bool bHasInsertable = false;
	TArray<FExitSlotViewData> ViewSlots;
	ViewSlots.Reserve(RequiredSlots.Num());

	for (const FExitRequirementSlot& Slot : RequiredSlots)
	{
		FExitSlotViewData& View = ViewSlots.AddDefaulted_GetRef();
		View.DisplayName = Slot.DisplayName.IsEmpty() ? GetDefaultItemDisplayName(Slot.ItemID) : Slot.DisplayName;
		if (const TObjectPtr<UTexture2D>* Icon = ItemIcons.Find(Slot.ItemID))
		{
			View.Icon = *Icon;
		}

		if (Slot.bInserted)
		{
			View.State = EExitSlotState::Inserted;
		}
		else if (CountItems(NearbyPlayer, Slot.ItemID) > 0)
		{
			View.State = EExitSlotState::Owned;
			bHasInsertable = true;
		}
		else
		{
			View.State = EExitSlotState::Missing;
		}
	}

	// 넣을 수 있는 아이템이 있을 때만 마우스 커서를 켠다.
	UpdateCursorMode(bWidgetShown && !bUnlocked && bHasInsertable);

	if (!IsValid(ExitWidget)) { return; }

	const FText Title = ExitTitle.IsEmpty()
		? FText::Format(FText::FromString(TEXT("{0}번 탈출구")), FText::AsNumber(ExitNumber))
		: ExitTitle;

	const FText& Hint = bUnlocked ? UnlockedHintText : (bHasInsertable ? InsertHintText : NoItemHintText);

	ExitWidget->SetExitData(Title, ViewSlots, Hint, bUnlocked);
}

void AExitDoor::UpdateCursorMode(bool bWantCursor)
{
	if (bWantCursor)
	{
		APlayerController* PlayerController = IsValid(NearbyPlayer) ? Cast<APlayerController>(NearbyPlayer->GetController()) : nullptr;

		// 이미 커서가 켜져 있으면(인벤토리, 일시정지 메뉴 등) 건드리지 않는다.
		if (PlayerController && !PlayerController->bShowMouseCursor)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
			PlayerController->SetShowMouseCursor(true);
			CursorController = PlayerController;
		}
	}
	else if (APlayerController* PlayerController = CursorController.Get())
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->SetShowMouseCursor(false);
		CursorController = nullptr;
	}
}

bool AExitDoor::HasInsertableItem(const ACharacterPlayer* Player) const
{
	for (const FExitRequirementSlot& Slot : RequiredSlots)
	{
		if (!Slot.bInserted && CountItems(Player, Slot.ItemID) > 0)
		{
			return true;
		}
	}
	return false;
}

void AExitDoor::HandleSlotClicked(int32 SlotIndex)
{
	if (TryInsertSlot(SlotIndex, NearbyPlayer))
	{
		CheckUnlock();
	}
	RefreshWidget();
}

bool AExitDoor::TryInsertSlot(int32 SlotIndex, ACharacterPlayer* Player)
{
	if (bUnlocked || !RequiredSlots.IsValidIndex(SlotIndex)) { return false; }

	FExitRequirementSlot& Slot = RequiredSlots[SlotIndex];
	if (Slot.bInserted || CountItems(Player, Slot.ItemID) <= 0) { return false; }

	if (bConsumeItems && !ConsumeOneItem(Player, Slot.ItemID)) { return false; }

	Slot.bInserted = true;
	UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: slot %d inserted (%d/%d)"), *GetName(), SlotIndex, GetInsertedCount(), RequiredSlots.Num());
	return true;
}

void AExitDoor::CheckUnlock()
{
	if (!RequiredSlots.IsEmpty() && GetInsertedCount() == RequiredSlots.Num())
	{
		Unlock();
	}
}

void AExitDoor::Unlock()
{
	if (bUnlocked) { return; }
	bUnlocked = true;

	SetLinkedActorsLocked(false);

	OnExitUnlocked();
	OnExitDoorUnlocked.Broadcast(this);
}

// 상호작용 키로도 넣을 수 있다. (잠긴 동안 DoorVolume이 트레이스에 잡힌다)
void AExitDoor::Interact_Implementation(AActor* Interactor)
{
	if (bUnlocked) { return; }

	ACharacterPlayer* Player = Cast<ACharacterPlayer>(Interactor);

	bool bInsertedAny = false;
	for (int32 Index = 0; Index < RequiredSlots.Num(); ++Index)
	{
		bInsertedAny |= TryInsertSlot(Index, Player);
	}

	if (bInsertedAny)
	{
		CheckUnlock();
	}
	RefreshWidget();
}

bool AExitDoor::CanInteract_Implementation(AActor* Interactor) const
{
	return !bUnlocked && IsValid(Interactor);
}

FText AExitDoor::GetInteractionText_Implementation(AActor* Interactor) const
{
	return HasInsertableItem(Cast<ACharacterPlayer>(Interactor)) ? InteractionText : NoItemHintText;
}

void AExitDoor::BeginFocus_Implementation(AActor* Interactor)
{
}

void AExitDoor::EndFocus_Implementation(AActor* Interactor)
{
}
