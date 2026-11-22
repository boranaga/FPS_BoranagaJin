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
	// Each exit uses three consecutive items, shifted by one per exit. (Exit N starts at index N - 1.)
	// Names must match the ItemID values in the item Blueprints.
	const TCHAR* const GExitItemOrder[] =
	{
		TEXT("KeyCard"),
		TEXT("Battery"),
		TEXT("Fuse"),
		TEXT("Chip")
	};

	// Read and modify only the publicly exposed slot array; do not change the inventory class.
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

			// INDEX_NONE removes the last item. FInventorySlot resets itself when the slot becomes empty.
			AItem* RemovedItem = Slot.RemoveItem(INDEX_NONE);
			if (IsValid(RemovedItem))
			{
				RemovedItem->DeactivateItem();
			}

			// Refresh the inventory UI.
			Player->OnInventoryUpdatedDelegate.Broadcast(*Slots);
			return true;
		}
		return false;
	}

	const int32 GExitRequiredItemCount = 3;

	// Above other HUD elements, but below the pause and game-over menus.
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
	// While locked, block only interaction traces (Visibility), not movement or bullets.
	DoorVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DoorVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	DoorVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	DoorVolume->SetGenerateOverlapEvents(false);

	// Player proximity detection box. Uses only overlap events, without ticks or timers.
	ProximityVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ProximityVolume"));
	ProximityVolume->SetupAttachment(SceneRoot);
	// Must be movable because BeginPlay positions it at DoorVolume.
	ProximityVolume->SetMobility(EComponentMobility::Movable);
	ProximityVolume->SetCanEverAffectNavigation(false);

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

	// Reapply collision settings here to prevent Blueprint-saved values from overriding them.
	// The capsule (Pawn profile) blocks WorldDynamic, so overlap occurs if this box overlaps Pawn.
	ProximityVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProximityVolume->SetCollisionObjectType(ECC_WorldDynamic);
	ProximityVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProximityVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ProximityVolume->SetGenerateOverlapEvents(true);

	// Match DoorVolume expanded by ShowDistance. (Account for scale in the box extent.)
	FTransform VolumeTransform = DoorVolume->GetComponentTransform();
	VolumeTransform.SetScale3D(FVector::OneVector);
	ProximityVolume->SetWorldTransform(VolumeTransform);
	ProximityVolume->SetBoxExtent(DoorVolume->GetScaledBoxExtent() + FVector(ShowDistance));

	ProximityVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &AExitDoor::HandleProximityBeginOverlap);
	ProximityVolume->OnComponentEndOverlap.AddUniqueDynamic(this, &AExitDoor::HandleProximityEndOverlap);
	ProximityVolume->UpdateOverlaps();

	UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: ProximityVolume loc=%s extent=%s"),
		*GetName(), *ProximityVolume->GetComponentLocation().ToString(), *ProximityVolume->GetUnscaledBoxExtent().ToString());

	// Handle a player already inside the box at startup.
	TArray<AActor*> OverlappingActors;
	ProximityVolume->GetOverlappingActors(OverlappingActors, ACharacterPlayer::StaticClass());
	for (AActor* Actor : OverlappingActors)
	{
		ACharacterPlayer* Player = Cast<ACharacterPlayer>(Actor);
		if (IsValid(Player) && Player->IsLocallyControlled())
		{
			SetNearbyPlayer(Player);
			break;
		}
	}
}

void AExitDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearNearbyPlayer();
	if (ExitWidget)
	{
		ExitWidget->RemoveFromParent();
		ExitWidget = nullptr;
	}

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
	// While locked, DoorVolume receives interaction traces instead of the door.
	DoorVolume->SetCollisionEnabled(bLocked ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	if (LinkedEscapeTrigger)
	{
		// Disable trigger collision to prevent overlap events.
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
		// Disable the door Blueprint detection boxes so it cannot open automatically when approached.
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

	// Directly play the door Blueprint timelines (opening animations).
	TInlineComponentArray<UTimelineComponent*> Timelines(DoorActor);
	for (UTimelineComponent* Timeline : Timelines)
	{
		Timeline->Play();
	}

	// If the door should stay open, leave detection boxes disabled. (Re-enabling them could close it when the player leaves.)
	// If no timeline was found, restore detection boxes to allow the original behavior.
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

void AExitDoor::HandleProximityBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: proximity begin overlap with %s"), *GetName(), *GetNameSafe(OtherActor));

	ACharacterPlayer* Player = Cast<ACharacterPlayer>(OtherActor);
	if (IsValid(Player) && Player->IsLocallyControlled())
	{
		SetNearbyPlayer(Player);
	}
}

void AExitDoor::HandleProximityEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == NearbyPlayer)
	{
		UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: proximity end overlap"), *GetName());
		ClearNearbyPlayer();
	}
}

void AExitDoor::SetNearbyPlayer(ACharacterPlayer* Player)
{
	if (NearbyPlayer == Player) { return; }

	ClearNearbyPlayer();
	NearbyPlayer = Player;

	// If the door was opened through an area not covered by DoorVolume while locked, close it again.
	if (!bUnlocked)
	{
		ADestructibleDoor* DestructibleDoor = Cast<ADestructibleDoor>(LinkedDoor);
		if (DestructibleDoor && DestructibleDoor->IsDoorOpen())
		{
			DestructibleDoor->CloseDoor();
		}
	}

	// Reflect inventory changes in the UI while the player is nearby.
	InventoryUpdatedHandle = NearbyPlayer->OnInventoryUpdatedDelegate.AddUObject(this, &AExitDoor::HandleInventoryUpdated);

	SetWidgetShown(true);
}

void AExitDoor::ClearNearbyPlayer()
{
	if (IsValid(NearbyPlayer) && InventoryUpdatedHandle.IsValid())
	{
		NearbyPlayer->OnInventoryUpdatedDelegate.Remove(InventoryUpdatedHandle);
	}
	InventoryUpdatedHandle.Reset();

	const bool bWasShown = bWidgetShown;
	NearbyPlayer = nullptr;
	if (bWasShown)
	{
		SetWidgetShown(false);
	}
}

void AExitDoor::HandleInventoryUpdated(const TArray<FInventorySlot>& InventorySlots)
{
	if (bWidgetShown)
	{
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
		// Use SelfHitTestInvisible so clicks pass through outside the panel.
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

	// Show the mouse cursor only when the player has an item that can be inserted.
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

		// Do not interfere if another UI (inventory, pause menu, etc.) already shows the cursor.
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

// Items can also be inserted using the interaction key. (DoorVolume is hit by traces while locked.)
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
