// Copyright Epic Games, Inc. All Rights Reserved.

#include "RulesHorrorUtils.h"
#include "RulesHorrorGameInstance.h"
#include "Lobby/LobbyPlayerController.h"

URulesHorrorGameInstance* URulesHorrorUtils::GetGameInstance(const UObject* _obj)
{
	auto world = _obj->GetWorld();
	if (IsValid(world))
	{
		return world->GetGameInstance<URulesHorrorGameInstance>();
	}

	return nullptr;
}

ALobbyPlayerController* URulesHorrorUtils::GetLobbyPlayerController(const UObject* _obj)
{
	return GetLocalPlayerController<ALobbyPlayerController>(_obj);
}

bool URulesHorrorUtils::IsInLobby(const UObject* _obj)
{
	auto lobby_pc = GetLobbyPlayerController(_obj);
	return IsValid(lobby_pc);
}
