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

bool URulesHorrorUtils::IsInLobby(const UObject* _obj)
{
	auto lobby_pc = GetLocalPlayerController<ALobbyPlayerController>(_obj);
	return IsValid(lobby_pc);
}
