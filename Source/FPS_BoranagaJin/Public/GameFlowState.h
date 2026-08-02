#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFlowState.generated.h"

UENUM(BlueprintType)
enum class EGameFlowState : uint8
{
	None UMETA(DisplayName = "None"),
	Boot UMETA(DisplayName = "Boot"),           // 게임 최초 실행
	MainMenu UMETA(DisplayName = "MainMenu"),       // 메인 메뉴
	Loading UMETA(DisplayName = "Loading"),        // 레벨 로딩 중
	Playing UMETA(DisplayName = "Playing"),        // 정상 플레이
	Paused UMETA(DisplayName = "Paused"),         // 일시정지
	GameOver UMETA(DisplayName = "GameOver"),       // 플레이어 사망
	LevelCompleted UMETA(DisplayName = "LevelCompleted"), // 스테이지 클리어
	Ending UMETA(DisplayName = "Ending")          // 엔딩
};
ENUM_RANGE_BY_FIRST_AND_LAST(EGameFlowState, EGameFlowState::None, EGameFlowState::Ending);

UCLASS()
class FPS_BORANAGAJIN_API AGameFlowState : public AActor
{
	GENERATED_BODY()
public:
	AGameFlowState();
};