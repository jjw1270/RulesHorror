// Copyright (c) 2026 장윤제. All rights reserved.

#include "RulesHorrorUtils.h"
#include "RulesHorrorGameInstance.h"
#include "GameFramework/PlayerController/LobbyPlayerController.h"

URulesHorrorGameInstance* URulesHorrorUtils::GetGameInstance(const UObject* _obj)
{
	if (IsInvalid(_obj))
		return nullptr;

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
