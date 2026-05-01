// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_Loading.h"
#include "RulesHorrorUtils.h"
#include "StoryFlowSubsystem.h"

void UUI_Loading::NativeConstruct()
{
	Super::NativeConstruct();

	_StoryFlowSubsystem = URulesHorrorUtils::GetGameInstanceSubsystem<UStoryFlowSubsystem>(this);
}

void UUI_Loading::NativeTick(const FGeometry& _geo, float _delta)
{
	Super::NativeTick(_geo, _delta);

	if (IsInvalid(_StoryFlowSubsystem))
		return;

	const float loading_progress_rate = _StoryFlowSubsystem->GetTargetLevelLoadingProgressRate();

	if (FMath::IsNearlyEqual(_ProgressRate, loading_progress_rate, 0.0001f))
		return;

	_ProgressRate = loading_progress_rate;

	OnUpdateProgressRate();
}
