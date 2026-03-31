// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_Loading.h"

void UUI_Loading::OnShow_Implementation()
{
	Super::OnShow_Implementation();

	_MinLoadingTime = FMath::RandRange(_MinLoadingTimeRange.X, _MinLoadingTimeRange.Y);
	_LoadingTimer = 0.0f;
}

void UUI_Loading::NativeTick(const FGeometry& _geo, float _delta)
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
