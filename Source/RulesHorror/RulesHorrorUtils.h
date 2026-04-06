// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUtils.h"
#include "RulesHorrorUtils.generated.h"

UENUM(BlueprintType)
enum class ERulesHorrorWidgetZOrder : uint8
{
	Zero = 0,
	InteractionIndicator = 3,
	Page = 10,
	Popup = 20,
};

using Utils = URulesHorrorUtils;

UCLASS()
class RULESHORROR_API URulesHorrorUtils : public UCommonUtils
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Utils", meta = (WorldContext = "_obj"))
	static class URulesHorrorGameInstance* GetGameInstance(const UObject* _obj);

	UFUNCTION(BlueprintPure, Category = "Utils|Lobby", meta = (WorldContext = "_obj"))
	static class ALobbyPlayerController* GetLobbyPlayerController(const UObject* _obj);

	UFUNCTION(BlueprintPure, Category = "Utils|Lobby", meta = (WorldContext = "_obj"))
	static bool IsInLobby(const UObject* _obj);

};
