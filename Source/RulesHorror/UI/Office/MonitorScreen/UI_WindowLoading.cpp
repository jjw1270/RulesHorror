// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_WindowLoading.h"

void UUI_WindowLoading::OnShow_Implementation()
{
	Super::OnShow_Implementation();

	_MinLoadingTime = FMath::RandRange(_MinLoadingTimeRange.X, _MinLoadingTimeRange.Y);
	_LoadingTimer = 0.0f;
}

void UUI_WindowLoading::NativeTick(const FGeometry& _geo, float _delta)
{
	Super::NativeTick(_geo, _delta);

	if (_LoadingTimer <= _MinLoadingTime)
	{
		_LoadingTimer += _delta;
	}
	else
	{
		OnLoadingFinished();
	}
}
