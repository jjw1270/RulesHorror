// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Lobby/Page_Loading.h"
#include "UI/RulesHorrorWidgetHelper.h"
#include "UI/Lobby/Page_InitUser.h"
#include "RulesHorrorUtils.h"
#include "Lobby/LobbyPlayerController.h"

void UPage_Loading::NativeConstruct()
{
	Super::NativeConstruct();

	_MinLoadingTime = FMath::RandRange(_MinLoadingTimeRange.X, _MinLoadingTimeRange.Y);

	if (_LoadingStartDateTime.IsSet() == false)
	{
		_LoadingStartDateTime = FDateTime::Now();
		StartLoading();
	}
}

void UPage_Loading::NativeTick(const FGeometry& _geo, float _delta)
{
	Super::NativeTick(_geo, _delta);

	if (_IsOnLoading == false)
	{
		const FTimespan loading_time = FDateTime::Now() - _LoadingStartDateTime.GetValue();

		if (loading_time.GetTotalSeconds() > _MinLoadingTime)
		{
			// open next page
			UWidgetHelper::OpenPage(this, _LoadingFinishedPageClass);
		}
	}
}

void UPage_Loading::StartLoading()
{
	// 필요한 데이터 등을 미리 로딩함.
	// 비동기 로딩 위주
	

	_IsOnLoading = true;
	OnLoadingFinished();
}

void UPage_Loading::OnLoadingFinished()
{
	_IsOnLoading = false;
}
