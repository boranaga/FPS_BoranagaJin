


#include "UI/TargetingSkillWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"


void UTargetingSkillWidget::InitializeUI(float NewCurrCoolDown, float NewMaxSkillCoolDown, float NewCurrTargetingTime, float NewMaxTargetingTime)
{
	CachedMaxCoolDown = NewMaxSkillCoolDown;

	if (CurrentSkillCoolDown)
	{
		FNumberFormattingOptions FormatOptions;
		FormatOptions.SetMaximumFractionalDigits(1);
		CurrentSkillCoolDown->SetText(FText::AsNumber(NewCurrCoolDown, &FormatOptions));
	}
	if (MaxSkillCoolDown)
	{
		FNumberFormattingOptions FormatOptions;
		FormatOptions.SetMaximumFractionalDigits(1);
		MaxSkillCoolDown->SetText(FText::AsNumber(NewMaxSkillCoolDown, &FormatOptions));
	}
	if (MaxTargetingTime)
	{
		FNumberFormattingOptions FormatOptions;
		FormatOptions.SetMaximumFractionalDigits(1);
		MaxTargetingTime->SetText(FText::AsNumber(NewMaxTargetingTime, &FormatOptions));
	}
	if (ElapsedTimeAfterTargetingStarted)
	{
		FNumberFormattingOptions FormatOptions;
		FormatOptions.SetMaximumFractionalDigits(1);
		ElapsedTimeAfterTargetingStarted->SetText(FText::AsNumber(NewCurrTargetingTime, &FormatOptions));
	}
}
void UTargetingSkillWidget::SetTargetingSkillCoolDown(float NewCurrCoolDown)
{
	//CurrentSkillCoolDown->SetText(FText::FromString(FString::Printf(TEXT("%f"), NewCurrCoolDown)));
	if (CurrentSkillCoolDown)
	{
		FNumberFormattingOptions FormatOptions;
		FormatOptions.SetMaximumFractionalDigits(1);
		CurrentSkillCoolDown->SetText(FText::AsNumber(NewCurrCoolDown, &FormatOptions));
	}

	ApplyCooltimeBarImage(NewCurrCoolDown);
}
void UTargetingSkillWidget::SetDetectionTimeUIVisible(bool bVisible)
{
	if (bVisible)
	{
		ElapsedTimeAfterTargetingStarted->SetVisibility(ESlateVisibility::Visible);
		MaxTargetingTime->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		ElapsedTimeAfterTargetingStarted->SetVisibility(ESlateVisibility::Hidden);
		MaxTargetingTime->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UTargetingSkillWidget::SetTargetingTimeUI(float NewCurrTargetingTime)
{
	if (ElapsedTimeAfterTargetingStarted)
	{
		FNumberFormattingOptions FormatOptions;
		FormatOptions.SetMaximumFractionalDigits(1);
		ElapsedTimeAfterTargetingStarted->SetText(FText::AsNumber(NewCurrTargetingTime, &FormatOptions));
	}
}

void UTargetingSkillWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UTargetingSkillWidget::ApplyCooltimeBarImage(float CurrentCooltime)
{
	// 1. 비율(Ratio) 계산
	float CoolDownRatio = 0.f;

	// CachedMaxCoolDown은 InitializeUI에서 미리 저장해 둔 멤버 변수라고 가정
	if (CachedMaxCoolDown > 0.f)
	{
		CoolDownRatio = FMath::Clamp(CurrentCooltime / CachedMaxCoolDown, 0.f, 1.f);
	}

	// 2. 텍스처 배열 정보 가져오기
	int32 TextureNum = HPBarTextures.Num();
	if (TextureNum == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UTargetingSkillWidget::ApplyCooltimeBarImage - HPBarTextures array is empty."));
		return;
	}

	// 배열 인덱스 계산 (5단계 비율: 0.2, 0.4, 0.6, 0.8, 1.0)
	// 쿨타임 완료(1.0f)가 Index 0이 되도록 배열을 반전
	int32 TextureIndex = 0; // 쿨타임 완료 (Index 0)

	if (CoolDownRatio < 1.0f)
	{
		TextureIndex = 1;
	}
	if (CoolDownRatio < 0.8f)
	{
		TextureIndex = 2;
	}
	if (CoolDownRatio < 0.6f)
	{
		TextureIndex = 3;
	}
	if (CoolDownRatio < 0.4f)
	{
		TextureIndex = 4;
	}
	if (CoolDownRatio < 0.2f)
	{
		TextureIndex = 5; // 쿨타임 시작 (가장 비어있음, Index 4)
	}

	// 배열 크기에 맞게 인덱스를 Clamp 
	TextureIndex = FMath::Clamp(TextureIndex, 0, TextureNum - 1);


	// 3. 이미지(브러시) 적용
	if (CoolTimeBar && HPBarTextures.IsValidIndex(TextureIndex))
	{
		CoolTimeBar->SetBrushFromTexture(HPBarTextures[TextureIndex]);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UTargetingSkillWidget::ApplyCooltimeBarImage - CoolTimeBar is invalid or TextureIndex %d is out of bounds (Array Size: %d)."), TextureIndex, TextureNum);
	}
}