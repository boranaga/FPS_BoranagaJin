#pragma once

#include "CoreMinimal.h"
#include "Objects/DestructibleObject.h"
#include "Interface/InteractableInterface.h"
#include "DestructibleDoor.generated.h"

class USceneComponent;

UENUM(BlueprintType)
enum class EDoorOpenType : uint8
{
	Slide	UMETA(DisplayName = "Slide"),
	Rotate	UMETA(DisplayName = "Rotate")
};

UENUM(BlueprintType)
enum class EDoorRotationAxis : uint8
{
	X	UMETA(DisplayName = "X Axis"),
	Y	UMETA(DisplayName = "Y Axis"),
	Z	UMETA(DisplayName = "Z Axis")
};

UCLASS()
class FPS_BORANAGAJIN_API ADestructibleDoor : public ADestructibleObject, public IInteractableInterface
{
	GENERATED_BODY()
public:
	ADestructibleDoor();
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
public:
	UFUNCTION(BlueprintCallable, Category = "Door")
	void OpenDoor();
	UFUNCTION(BlueprintCallable, Category = "Door")
	void CloseDoor();
	UFUNCTION(BlueprintCallable, Category = "Door")
	void ToggleDoor();
	UFUNCTION(BlueprintCallable, Category = "Door")
	void SetDoorOpen(bool bOpen);
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsDoorOpen() const;
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsDoorMoving() const;
protected:
	virtual void BreakObject(const FVector& HitLocation, const FVector& HitDirection) override;

	void UpdateSlidingDoor(float DeltaTime);
	void UpdateRotatingDoor(float DeltaTime);

	FVector GetTargetLocation() const;
	FRotator GetTargetRotation() const;
	FRotator GetRotationOffset() const;
protected:
	/*
	 * 회전문의 중심축입니다.
	 *
	 * Rotate 방식에서는 이 컴포넌트를 회전시키므로
	 * Blueprint에서 DoorPivot의 위치를 문의 경첩 위치로 옮겨야 합니다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door|Component")
	TObjectPtr<USceneComponent> DoorPivot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	EDoorOpenType DoorOpenType = EDoorOpenType::Rotate;

	/*
	 * Slide 방식에서 닫힌 위치를 기준으로 이동할 상대 좌표입니다.
	 *
	 * 예:
	 * (0, 200, 0)   : 로컬 Y 방향으로 이동
	 * (0, 0, 250)   : 위쪽으로 이동
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Slide", meta = (EditCondition = "DoorOpenType == EDoorOpenType::Slide"))
	FVector SlideOffset = FVector(0.f, 200.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Rotate", meta = (EditCondition = "DoorOpenType == EDoorOpenType::Rotate"))
	EDoorRotationAxis RotationAxis = EDoorRotationAxis::Z;

	/*
	 * 음수를 입력하면 반대 방향으로 열립니다.
	 */
	UPROPERTY(EditAnywhere,
		BlueprintReadWrite,
		Category = "Door|Rotate",
		meta = (
			EditCondition = "DoorOpenType == EDoorOpenType::Rotate",
			ClampMin = "-360.0",
			ClampMax = "360.0"
			)
	)
	float OpenAngle = 90.f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Door|Movement",
		meta = (ClampMin = "0.0")
	)
	float OpenInterpSpeed = 4.f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Door|Movement",
		meta = (ClampMin = "0.0")
	)
	float CloseInterpSpeed = 4.f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Door|Movement",
		meta = (ClampMin = "0.001")
	)
	float LocationTolerance = 0.5f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Door|Movement",
		meta = (ClampMin = "0.001")
	)
	float RotationTolerance = 0.5f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Door"
	)
	bool bStartsOpen = false;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Door"
	)
	bool bIsOpen = false;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Door"
	)
	bool bIsMoving = false;

private:
	FVector ClosedRelativeLocation = FVector::ZeroVector;
	FRotator ClosedRelativeRotation = FRotator::ZeroRotator;

	FVector OpenRelativeLocation = FVector::ZeroVector;
	FRotator OpenRelativeRotation = FRotator::ZeroRotator;

#pragma region InteractableInterface
public:
	virtual void Interact_Implementation(AActor* Interactor) override;

	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	virtual FText GetInteractionText_Implementation(AActor* Interactor) const override;

	virtual void BeginFocus_Implementation(AActor* Interactor) override;

	virtual void EndFocus_Implementation(AActor* Interactor) override;

protected:
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Door|Interaction"
	)
	FText OpenInteractionText = NSLOCTEXT("Door", "OpenDoor", "OpenDoor");

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Door|Interaction"
	)
	FText CloseInteractionText = NSLOCTEXT("Door", "CloseDoor", "CloseDoor");

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Door|Interaction"
	)
	bool bHighlightWhenFocused = true;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Door|Interaction",
		meta = (
			EditCondition = "bHighlightWhenFocused",
			ClampMin = "0"
			)
	)
	int32 HighlightStencilValue = 1;

#pragma endregion
};