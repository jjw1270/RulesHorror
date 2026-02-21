// Copyright Epic Games, Inc. All Rights Reserved.


#include "LobbyPlayerController.h"
#include "UI/Lobby/UI_MainLobby.h"

ALobbyPlayerController::ALobbyPlayerController()
{
}

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitMainLobbyUI();
}

#pragma region MainLobbyUI

void ALobbyPlayerController::InitMainLobbyUI()
{
	if (IsInvalid(_MainLobbyUIClass))
	{
		TRACE_WARNING(TEXT("MainLobbyUIClass is invalid"));
		return;
	}

	if (IsInvalid(_MainLobbyUI))
	{
		_MainLobbyUI = CreateWidget<UUI_MainLobby>(this, _MainLobbyUIClass);
		
		if (IsValid(_MainLobbyUI))
		{
			_MainLobbyUI->AddToViewport((int32)EWidgetZOrder::Zero); // page 보다 밑으로
		}
	}
}

void ALobbyPlayerController::ShowMainLobbyUI()
{
	if (IsInvalid(_MainLobbyUI))
	{
		TRACE_WARNING(TEXT("MainLobbyUI is invalid"));
		return;
	}

	_MainLobbyUI->Show(EWidgetShowType::SelfHitTestInvisible, false);
}

void ALobbyPlayerController::HideMainLobbyUI()
{
	if (IsInvalid(_MainLobbyUI))
	{
		TRACE_WARNING(TEXT("MainLobbyUI is invalid"));
		return;
	}

	_MainLobbyUI->Hide(EWidgetHideType::Collapsed, false);
}

#pragma endregion MainLobbyUI
